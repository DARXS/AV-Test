#ifndef BRIDGE_MODULE_H
#define BRIDGE_MODULE_H

#include <Arduino.h>
#include <list> // Necessário para o cache de mensagens
#include "IModule.h"
#include "AV_espnow.h"
#include "packets.h"

class BridgeModule : public IModule {
public:
    BridgeModule();
    virtual ~BridgeModule();

    void setup() override;
    void loop() override;

private:
    //== MÉTODOS DE LÓGICA INTERNA ==//
    void initializeEspNow();
    void printAddress();

    //== CALLBACKS E INTERRUPÇÕES ==//
    // Método da instância
    void onDataRecvHandler(const av_packet_t& packet);

    // Callback estático
    static void onDataRecv(const uint8_t* macAddr, const uint8_t* data, int len);

    //== VARIÁVEIS DE ESTADO (Membros da Classe) ==//
    // Cache para armazenar os IDs das mensagens já vistas e retransmitidas
    std::list<uint32_t> m_seenMessages; 
    
    // -- Constantes --
    // Define o número máximo de IDs de mensagens a serem lembrados
    static const size_t SEEN_MESSAGES_CACHE_SIZE = 20;

    // Ponteiro estático para a instância da classe, usado pelo callback
    static BridgeModule* s_instance;
};

#endif // BRIDGE_MODULE_H