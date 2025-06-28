#ifndef MODULE_100M_H
#define MODULE_100M_H

#include <Arduino.h>
#include "IModule.h"
#include "AV_espnow.h"
#include "hardware_defs.h"
#include "packets.h"

class Module100m : public IModule {
public:
    Module100m();
    virtual ~Module100m();

    void setup() override;
    void loop() override;

private:
    //== MÉTODOS DE LÓGICA INTERNA ==//
    void initializeEspNow();
    void printAddress();
    void runMainLoop();
    void handleState();

    //== CALLBACKS E INTERRUPÇÕES ==//
    // Método da instância
    void onDataRecvHandler(const av_packet_t& packet);

    // Callback estático
    static void onDataRecv(const uint8_t* macAddr, const uint8_t* data, int len);
    
    //== VARIÁVEIS DE ESTADO (Membros da Classe) ==//
    state_t m_currentState; // Estado atual do módulo (wait, setup, run)

    // -- Dados da Corrida --
    unsigned long m_startTime; // Tempo de início da corrida (equivalente a 'curr')
    
    // -- Comunicação --
    av_packet_t m_packet; // Pacote de dados para comunicação

    // Ponteiro estático para a instância da classe, usado pelos callbacks
    static Module100m* s_instance;
};

#endif // MODULE_100M_H