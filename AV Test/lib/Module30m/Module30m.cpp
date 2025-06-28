#include "Module30m.h"
#include <WiFi.h>

// Inicialização do ponteiro estático para a instância da classe
Module30m* Module30m::s_instance = nullptr;

//================================================================================
// Construtor e Destrutor
//================================================================================

Module30m::Module30m() {
    s_instance = this; // Aponta para a instância atual

    // Inicializa os membros da classe
    m_currentState = state_t::wait;
    m_startTime = 0;
    m_interruptTriggered = false;
    m_interruptAttached = false;
    m_peerRegistered = false;
    memset(m_ecuMacAddress, 0, 6); // Zera o array do endereço MAC
}

Module30m::~Module30m() {
    s_instance = nullptr;
}

//================================================================================
// Métodos Públicos (setup e loop)
//================================================================================

void Module30m::setup() {
    // Configura o hardware para o módulo 30m
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(SENSOR_30m, INPUT_PULLUP);

    m_packet.id = module_t::metros_30; // Identifica este módulo

    initializeEspNow(); // Inicia a comunicação e o loop principal
}

void Module30m::loop() {
    // A lógica está toda contida dentro de runMainLoop.
}

//================================================================================
// Métodos Privados de Lógica Principal
//================================================================================

void Module30m::initializeEspNow() {
    WiFi.mode(WIFI_MODE_STA);
    printAddress();

    if (!init_esp_now() || !register_receive_callback(onDataRecv)) {
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.println("Módulo 30m: Falha ao inicializar ESP-NOW. Reiniciando...");
        delay(1500);
        esp_restart();
    }
    Serial.println("30 metros OK!");

    runMainLoop();
}

void Module30m::runMainLoop() {
    while(1) {
        handleState();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void Module30m::handleState() {
    switch (m_currentState) {
        case state_t::wait:
            // Garante que a interrupção esteja desativada no estado de espera
            if (m_interruptAttached) {
                detachInterrupt(digitalPinToInterrupt(SENSOR_30m));
                m_interruptAttached = false;
            }
            break;

        case state_t::__setup__:
            m_startTime = millis(); // Salva o tempo de início da corrida
            m_interruptTriggered = false;
            m_currentState = state_t::run;
            break;

        case state_t::run:
            // Ativa a interrupção apenas uma vez ao entrar no estado de corrida
            if (!m_interruptAttached) {
                attachInterrupt(digitalPinToInterrupt(SENSOR_30m), onSensorTrigger, FALLING);
                m_interruptAttached = true;
            }

            // Aguarda a interrupção sinalizar que o sensor foi acionado
            while (!m_interruptTriggered) {
                vTaskDelay(1); // Libera o processador enquanto espera
            }

            // Quando a interrupção ocorre...
            if (m_interruptTriggered) {
                m_packet.command_for_state_machine = state_machine_command_t::end_run_30m;
                // O tempo (m_packet.time) foi gravado de forma segura pela ISR
                
                // Envia o pacote com o tempo para a ECU
                sent_to_single(&m_packet, sizeof(av_packet_t), m_ecuMacAddress);
                
                m_currentState = state_t::wait; // Retorna ao estado de espera
                m_packet.command_for_state_machine = state_machine_command_t::do_nothing;
            }
            break;
    }
}

void Module30m::printAddress() {
    Serial.print("Mac address: ");
    Serial.println(WiFi.macAddress());
}

//================================================================================
// Handlers e Callbacks
//================================================================================

void Module30m::onDataRecvHandler(const av_packet_t& packet) {
    av_packet_t received_packet = packet; // Cria uma cópia para modificar

    if (received_packet.id == module_t::metros_0) { // Se o comando veio da ECU
        switch (received_packet.command_for_state_machine) {
            case state_machine_command_t::check_module:
                // Registra o MAC da ECU para comunicação direta
                if (!m_peerRegistered) {
                    if (register_peer(received_packet.mac_address)) {
                        memcpy(m_ecuMacAddress, received_packet.mac_address, 6);
                        m_peerRegistered = true;
                        Serial.println("Peer (ECU) registrado.");
                    }
                }
                // Responde à ECU confirmando sua presença
                m_packet.command_for_state_machine = state_machine_command_t::flag_30m;
                sent_to_single(&m_packet, sizeof(av_packet_t), m_ecuMacAddress);
                
                // Atua como ponte: retransmite o check para outros módulos (100m)
                received_packet.id = module_t::metros_30;
                sent_to_all(&received_packet, sizeof(av_packet_t));
                break;

            case state_machine_command_t::start_run:
                m_currentState = state_t::__setup__; // Muda para o estado de preparação
                // Retransmite o comando de início para o módulo 100m
                received_packet.id = module_t::metros_30;
                sent_to_all(&received_packet, sizeof(av_packet_t));
                break;

            case state_machine_command_t::cancel:
            case state_machine_command_t::reset_:
                m_currentState = state_t::wait; // Volta ao estado de espera
                // Retransmite os comandos de cancelar/resetar
                received_packet.id = module_t::metros_30;
                sent_to_all(&received_packet, sizeof(av_packet_t));
                
                if (received_packet.command_for_state_machine == state_machine_command_t::reset_) {
                    delay(100);
                    esp_restart();
                }
                break;
        }
    } else if (received_packet.id == module_t::metros_100) { // Se recebeu do 100m
        // Atua como ponte: repassa a mensagem do 100m diretamente para a ECU
        received_packet.id = module_t::metros_30; // Identifica que a retransmissão veio daqui
        sent_to_single(&received_packet, sizeof(av_packet_t), m_ecuMacAddress);
    }
}

// Callback estático que chama o método da instância
void Module30m::onDataRecv(const uint8_t* macAddr, const uint8_t* data, int len) {
    if (s_instance && len == sizeof(av_packet_t)) {
        av_packet_t p;
        memcpy(&p, data, len);
        s_instance->onDataRecvHandler(p);
    }
}

// Rotina de Serviço da Interrupção (ISR)
void IRAM_ATTR Module30m::onSensorTrigger() {
    if (s_instance) {
        // Grava o tempo exato da passagem. Esta é a única linha que deve fazer isso.
        s_instance->m_packet.time = millis() - s_instance->m_startTime;
        s_instance->m_interruptTriggered = true;
        
        // Desanexa a interrupção para evitar múltiplos acionamentos na mesma corrida
        if (s_instance->m_interruptAttached) {
            detachInterrupt(digitalPinToInterrupt(SENSOR_30m));
            s_instance->m_interruptAttached = false;
        }
    }
}