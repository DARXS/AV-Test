#ifndef MODULE_30M_H
#define MODULE_30M_H

#include <Arduino.h>
#include "IModule.h"
#include "AV_espnow.h"
#include "hardware_defs.h"
#include "packets.h"

class Module30m : public IModule {
public:
    Module30m();
    virtual ~Module30m();

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

    // Callbacks estáticos
    static void onDataRecv(const uint8_t* macAddr, const uint8_t* data, int len);
    static void IRAM_ATTR onSensorTrigger(); // Rotina de interrupção

    //== VARIÁVEIS DE ESTADO (Membros da Classe) ==//
    state_t m_currentState; // Estado atual do módulo (wait, setup, run)
    
    // -- Dados da Corrida --
    unsigned long m_startTime;           // Tempo de início da corrida (equivalente a 'curr')
    volatile bool m_interruptTriggered;  // Flag sinalizada pela ISR
    bool m_interruptAttached;            // Controle para anexar/desanexar a ISR

    // -- Comunicação --
    bool m_peerRegistered;              // Flag para saber se a ECU já foi registrada
    av_packet_t m_packet;               // Pacote de dados para comunicação
    uint8_t m_ecuMacAddress[6];         // Armazena o MAC da ECU para envio direto

    // Ponteiro estático para a instância da classe, usado pelos callbacks
    static Module30m* s_instance;
};

#endif // MODULE_30M_H