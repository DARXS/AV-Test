#include "Module100m.h"
#include <WiFi.h>

// Inicialização do ponteiro estático para a instância da classe
Module100m* Module100m::s_instance = nullptr;

//================================================================================
// Construtor e Destrutor
//================================================================================

Module100m::Module100m() {
    s_instance = this; // Aponta para a instância atual

    // Inicializa os membros da classe
    m_currentState = state_t::wait;
    m_startTime = 0;
}

Module100m::~Module100m() {
    s_instance = nullptr;
}


//================================================================================
// Métodos Públicos (setup e loop)
//================================================================================

void Module100m::setup() {
    // Configura o hardware para o módulo 100m
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(SENSOR_100m, INPUT_PULLUP);
    pinMode(SENSOR_101m, INPUT_PULLUP);
    
    m_packet.id = module_t::metros_100; // Identifica este módulo

    initializeEspNow();
}

void Module100m::loop() {
    // A lógica está toda contida dentro de runMainLoop.
}


//================================================================================
// Métodos Privados de Lógica Principal
//================================================================================

void Module100m::initializeEspNow() {
    WiFi.mode(WIFI_MODE_STA);
    printAddress();

    if (!init_esp_now() || !register_receive_callback(onDataRecv)) {
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.println("Módulo 100m: Falha ao inicializar ESP-NOW. Reiniciando...");
        delay(1500);
        esp_restart();
    }
    Serial.println("100 metros OK!");

    runMainLoop();
}

void Module100m::runMainLoop() {
    while(1) {
        handleState();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void Module100m::handleState() {
    switch (m_currentState) {
        case state_t::wait:
            // Estado ocioso, aguardando comandos
            break;

        case state_t::__setup__:
            m_startTime = millis(); // Salva o tempo de início da corrida
            m_currentState = state_t::run;
            break;

        case state_t::run:
            // Aguarda o acionamento do sensor de 100m (polling)
            while (digitalRead(SENSOR_100m)) {
                vTaskDelay(1);
            }
            m_packet.time = millis() - m_startTime; // Salva o tempo dos 100m

            // Aguarda o acionamento do sensor de 101m (polling)
            while (digitalRead(SENSOR_101m)) {
                vTaskDelay(1);
            }
            m_packet.timer2 = millis() - m_startTime; // Salva o tempo dos 101m
            
            // Envia os dados para a rede
            m_packet.command_for_state_machine = state_machine_command_t::end_run_100m;
            sent_to_all(&m_packet, sizeof(av_packet_t)); // Envia em broadcast
            
            m_currentState = state_t::wait; // Retorna ao estado de espera
            m_packet.command_for_state_machine = state_machine_command_t::do_nothing;
            break;
    }
}

void Module100m::printAddress() {
    Serial.print("Mac address: ");
    Serial.println(WiFi.macAddress());
}

//================================================================================
// Handlers e Callbacks
//================================================================================

void Module100m::onDataRecvHandler(const av_packet_t& packet) {
    // Este módulo só reage a pacotes retransmitidos pelo módulo 30m
    if (packet.id == module_t::metros_30) {
        switch (packet.command_for_state_machine) {
            case state_machine_command_t::check_module:
                // Responde ao check, confirmando sua presença na rede
                m_packet.command_for_state_machine = state_machine_command_t::flag_100m;
                sent_to_all(&m_packet, sizeof(av_packet_t));
                break;
            
            case state_machine_command_t::start_run:
                m_currentState = state_t::__setup__; // Inicia a preparação para a corrida
                break;

            case state_machine_command_t::cancel:
            case state_machine_command_t::reset_:
                m_currentState = state_t::wait; // Retorna ao estado de espera
                if (packet.command_for_state_machine == state_machine_command_t::reset_) {
                    delay(100);
                    esp_restart();
                }
                break;
        }
    }
}

void Module100m::onDataRecv(const uint8_t* macAddr, const uint8_t* data, int len) {
    if (s_instance && len == sizeof(av_packet_t)) {
        av_packet_t p;
        memcpy(&p, data, len);
        s_instance->onDataRecvHandler(p);
    }
}