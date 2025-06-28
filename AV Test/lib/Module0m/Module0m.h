#ifndef MODULE_0M_H
#define MODULE_0M_H

#include <Arduino.h>
#include <Ticker.h>
#include "IModule.h"
#include "AV_espnow.h"
#include "LCD.h"
#include "sd_device.h"
#include "hardware_defs.h"
#include "packets.h"

class Module0m : public IModule {
public:
    Module0m();
    virtual ~Module0m();

    void setup() override;
    void loop() override;

private:
    //== MÉTODOS DE LÓGICA INTERNA ==//
    void initializeDependencies();
    void printAddress();
    void checkPeerModules();
    void runMainLoop();
    
    // Máquinas de estado
    void handleMenuState();
    void handleRunState();
    void handleSaveState();

    // Sub-estados da corrida
    void handleWaitToStart();
    void handleLcdDisplay();
    void handleEndRun();

    // Ações de hardware
    int8_t readPotentiometer();
    
    //== CALLBACKS E INTERRUPÇÕES ==//
    // Métodos da instância chamados pelos callbacks estáticos
    void onDataSentHandler(esp_now_send_status_t status);
    void onDataRecvHandler(const av_packet_t& packet);
    
    // Callbacks estáticos para o ESP-NOW e Ticker
    static void onDataSent(const uint8_t* macAddr, esp_now_send_status_t status);
    static void onDataRecv(const uint8_t* macAddr, const uint8_t* data, int len);
    static void enableResetFlag();

    //== VARIÁVEIS DE ESTADO (Membros da Classe) ==//
    av_ecu_t m_ecuState;        // Estado principal da ECU (menu, run, save)
    av_ecu_t m_runState;        // Sub-estado da corrida (wait, display, end)
    int8_t m_potSel;            // Posição atual do potenciômetro para seleção
    int8_t m_oldPotSel;         // Posição anterior para detectar mudança
    bool m_needsLcdUpdate;      // Flag para evitar redesenhar a tela desnecessariamente
    
    // Flags de status da corrida
    bool m_runFinished30m;
    bool m_runFinished100m;

    // Tempos da corrida
    unsigned long m_time30;
    unsigned long m_time100;
    unsigned long m_time101;

    // Status da comunicação e hardware
    bool m_espNowOk;
    bool m_conf30m;
    bool m_conf100m;
    bool m_sdInitialized;
    bool m_canReset;            // Flag para controle do botão de reset

    // Pacote de dados para comunicação
    av_packet_t m_packet;
    
    // Objeto para controle de tempo (reset)
    Ticker m_resetTicker;

    // Ponteiro estático para a instância, usado pelos callbacks estáticos
    static Module0m* s_instance;
};

#endif // MODULE_0M_H