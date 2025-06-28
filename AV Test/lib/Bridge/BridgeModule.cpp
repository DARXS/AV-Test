#include "BridgeModule.h"
#include <WiFi.h>

// Inicialização de membros estáticos
BridgeModule* BridgeModule::s_instance = nullptr;
const size_t BridgeModule::SEEN_MESSAGES_CACHE_SIZE; // Define o tamanho do cache

//================================================================================
// Construtor e Destrutor
//================================================================================

BridgeModule::BridgeModule() {
    s_instance = this;
}

BridgeModule::~BridgeModule() {
    s_instance = nullptr;
}


//================================================================================
// Métodos Públicos (setup e loop)
//================================================================================

void BridgeModule::setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    initializeEspNow();

    Serial.println("Módulo Bridge: Pronto para retransmitir pacotes.");
}

void BridgeModule::loop() {
    // O módulo bridge é puramente reativo, não há lógica no loop principal.
    // Apenas aguarda para economizar energia.
    vTaskDelay(100 / portTICK_PERIOD_MS);
}


//================================================================================
// Métodos Privados de Inicialização
//================================================================================

void BridgeModule::initializeEspNow() {
    WiFi.mode(WIFI_MODE_STA);
    printAddress();

    if (!init_esp_now() || !register_receive_callback(onDataRecv)) {
        digitalWrite(LED_BUILTIN, HIGH); // Sinaliza erro fatal
        Serial.println("Módulo Bridge: Falha ao inicializar ESP-NOW. Reiniciando...");
        delay(1500);
        esp_restart();
    }
}

void BridgeModule::printAddress() {
    Serial.print("Módulo Bridge MAC Address: ");
    Serial.println(WiFi.macAddress());
}


//================================================================================
// Handlers e Callbacks
//================================================================================

// Lógica principal do Bridge
void BridgeModule::onDataRecvHandler(const av_packet_t& packet) {
    // 1. Verifica se a mensagem já foi vista para evitar loops
    for (uint32_t seen_id : m_seenMessages) {
        if (seen_id == packet.message_id) {
            // Se a mensagem já foi retransmitida, ignora.
            return;
        }
    }

    // 2. Se for uma nova mensagem, adiciona ao cache
    // Para pacotes sem message_id, um valor padrão (como 0) pode ser usado,
    // mas a lógica é mais robusta se cada pacote tiver um ID único.
    if (packet.message_id != 0) {
        m_seenMessages.push_back(packet.message_id);
        // Mantém o cache com um tamanho fixo, removendo o mais antigo se necessário
        if (m_seenMessages.size() > SEEN_MESSAGES_CACHE_SIZE) {
            m_seenMessages.pop_front();
        }
    }
    
    // 3. Retransmite o pacote para todos na rede
    if (sent_to_all((void*)&packet, sizeof(av_packet_t))) {
        // Pisca o LED para indicar atividade
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.printf("Pacote #%u retransmitido com sucesso.\n", packet.message_id);
        delay(50);
        digitalWrite(LED_BUILTIN, LOW);
    } else {
        Serial.printf("Falha ao retransmitir pacote #%u.\n", packet.message_id);
    }
}

// Callback estático que chama o método da instância
void BridgeModule::onDataRecv(const uint8_t* macAddr, const uint8_t* data, int len) {
    if (s_instance && len == sizeof(av_packet_t)) {
        av_packet_t p;
        memcpy(&p, data, len);
        s_instance->onDataRecvHandler(p);
    }
}