#include "Module0m.h"
#include <WiFi.h>

// Inicialização do ponteiro estático para a instância da classe
Module0m* Module0m::s_instance = nullptr;

//================================================================================
// Construtor e Destrutor
//================================================================================
Module0m::Module0m() {
    s_instance = this; // Aponta para a instância atual

    // Inicialização dos membros da classe
    m_ecuState = av_ecu_t::initializing;
    m_runState = av_ecu_t::wait_to_start;
    m_potSel = 0;
    m_oldPotSel = -1;
    m_needsLcdUpdate = true;
    m_runFinished30m = false;
    m_runFinished100m = false;
    m_time30 = 0;
    m_time100 = 0;
    m_time101 = 0;
    m_espNowOk = false;
    m_conf30m = false;
    m_conf100m = false;
    m_sdInitialized = false;
    m_canReset = true;
}

Module0m::~Module0m() {
    s_instance = nullptr;
}

//================================================================================
// Métodos Públicos (setup e loop)
//================================================================================
void Module0m::setup() {
    // Configuração dos pinos de hardware para a ECU
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(POT, INPUT);
    pinMode(B_SEL, INPUT_PULLUP);
    pinMode(B_CANCEL, INPUT_PULLUP);
    pinMode(SENSOR_0m, INPUT_PULLUP);

    m_packet.id = module_t::metros_0;
    
    initializeDependencies();
}

void Module0m::loop() {
    // A lógica está toda contida dentro de runMainLoop.
}

//================================================================================
// Lógica Principal e Inicialização
//================================================================================
void Module0m::initializeDependencies() {
    init_lcd();
    WiFi.mode(WIFI_MODE_STA);
    printAddress();

    if (!init_esp_now() ||
        !register_receive_callback(onDataRecv) ||
        !register_transmitter_callback(onDataSent)) {
        digitalWrite(LED_BUILTIN, HIGH);
        error_message();
        delay(500);
        esp_restart();
    }
    Serial.println("AV ECU OK!");

    m_ecuState = av_ecu_t::initializing;
    runMainLoop();
}

void Module0m::runMainLoop() {
    while(1) {
        switch (m_ecuState) {
            case av_ecu_t::initializing:
                checkPeerModules();
                break;
            case av_ecu_t::menu:
                handleMenuState();
                break;
            case av_ecu_t::__start_run__:
                handleRunState();
                break;
            case av_ecu_t::save_run:
                handleSaveState();
                break;
            default:
                m_ecuState = av_ecu_t::menu;
                break;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void Module0m::checkPeerModules() {
    Serial.println("Procurando módulos...");
    WiFi.macAddress(m_packet.mac_address);
    m_packet.command_for_state_machine = state_machine_command_t::check_module;
    
    while (!m_conf30m || !m_conf100m) {
        sent_to_all(&m_packet, sizeof(av_packet_t));
        delay(200);
    }
    
    ok_message();
    m_sdInitialized = init_sd((uint8_t)SD_CS);
    SD_status(m_sdInitialized);

    m_ecuState = av_ecu_t::menu;
    m_needsLcdUpdate = true;
}

//================================================================================
// Máquinas de Estado
//================================================================================

void Module0m::handleMenuState() {
    if (m_needsLcdUpdate) {
        intro_text();
        m_needsLcdUpdate = false;
    }

    if (!digitalRead(B_SEL)) {
        vTaskDelay(120);
        while(!digitalRead(B_SEL));
        m_ecuState = av_ecu_t::__start_run__;
        m_runState = av_ecu_t::wait_to_start;
        m_runFinished30m = false;
        m_runFinished100m = false;
    }

    if (!digitalRead(B_CANCEL) && m_canReset) {
        m_packet.command_for_state_machine = state_machine_command_t::reset_;
        sent_to_all(&m_packet, sizeof(av_packet_t));
        delay(500);
        esp_restart();
    }
}

void Module0m::handleRunState() {
    switch (m_runState) {
        case av_ecu_t::wait_to_start:
            handleWaitToStart();
            break;
        case av_ecu_t::lcd_display:
            handleLcdDisplay();
            break;
        case av_ecu_t::end_run:
            handleEndRun();
            break;
        default:
             m_runState = av_ecu_t::wait_to_start;
             break;
    }
}

void Module0m::handleSaveState() {
    if (m_sdInitialized) {
        m_potSel = readPotentiometer();

        if (m_potSel != m_oldPotSel) {
            select_sd(m_potSel);
            m_oldPotSel = m_potSel;
        }

        if (!digitalRead(B_SEL)) {
            vTaskDelay(120);
            while(!digitalRead(B_SEL));

            if (m_potSel == 0) { // Opção "SIM"
                float speed = 0.0f;
                if (m_time101 > m_time100 && m_time100 > 0) {
                     float delta_t_s = (float)(m_time101 - m_time100) / 1000.0f;
                     speed = (1.0f / delta_t_s) * 3.6f;
                }
                
                // --- CHAMADA CORRIGIDA ---
                display_save_result(save_AV_Data, m_time30, m_time100, speed);

            } else { // Opção "NÃO"
                // --- CHAMADA CORRIGIDA ---
                display_returning_message();
            }

            m_ecuState = av_ecu_t::menu;
            m_needsLcdUpdate = true;
            m_oldPotSel = -1;
        }
    } else {
        SD_status(false);
        m_ecuState = av_ecu_t::menu;
        m_needsLcdUpdate = true;
    }

    if (!digitalRead(B_CANCEL)) {
         m_ecuState = av_ecu_t::menu;
         m_needsLcdUpdate = true;
    }
}


//================================================================================
// Sub-estados da Corrida
//================================================================================

void Module0m::handleWaitToStart() {
    m_time30 = m_time100 = m_time101 = 0;
    start_the_av_run();

    if (!digitalRead(B_CANCEL)) {
        m_packet.command_for_state_machine = state_machine_command_t::cancel;
        sent_to_all(&m_packet, sizeof(av_packet_t));
        m_ecuState = av_ecu_t::menu;
        m_needsLcdUpdate = true;
        m_canReset = false;
        m_resetTicker.once(2.0f, enableResetFlag);
        return;
    }

    if (digitalRead(SENSOR_0m)) {
        m_packet.command_for_state_machine = state_machine_command_t::start_run;
        sent_to_all(&m_packet, sizeof(av_packet_t));
        save_tcurr_time(millis());
        m_runState = av_ecu_t::lcd_display;
    }
}

void Module0m::handleLcdDisplay() {
    bool cancelled = false;

    while (!m_runFinished30m && !cancelled) {
        printRun(millis(), millis());
        if (!digitalRead(B_CANCEL)) { cancelled = true; }
    }
    
    if (!cancelled) save_t30(m_time30);

    while (!m_runFinished100m && !cancelled) {
        printRun(millis());
        if (!digitalRead(B_CANCEL)) { cancelled = true; }
    }

    if (cancelled) {
        m_packet.command_for_state_machine = state_machine_command_t::cancel;
        sent_to_all(&m_packet, sizeof(av_packet_t));
        m_ecuState = av_ecu_t::menu;
        m_needsLcdUpdate = true;
        return;
    }
    
    save_t100(m_time100);
    m_runState = av_ecu_t::end_run;
}

void Module0m::handleEndRun() {
    save_speed(m_time100, m_time101);

    if (!digitalRead(B_SEL)) {
        vTaskDelay(120);
        while (!digitalRead(B_SEL));
        m_ecuState = av_ecu_t::save_run;
    }
    
    if (!digitalRead(B_CANCEL)) {
        m_ecuState = av_ecu_t::menu;
        m_needsLcdUpdate = true;
    }
}


//================================================================================
// Ações de Hardware e Callbacks
//================================================================================

void Module0m::printAddress() {
    Serial.print("Mac address: ");
    Serial.println(WiFi.macAddress());
}

int8_t Module0m::readPotentiometer() {
    uint16_t read_val = analogRead(POT);
    return (map(read_val, 0, 4095, 0, 2) == 0) ? 0 : 1;
}

void Module0m::onDataSentHandler(esp_now_send_status_t status) {
    m_espNowOk = (status == ESP_NOW_SEND_SUCCESS);
}

void Module0m::onDataRecvHandler(const av_packet_t& packet) {
    if (packet.id == module_t::metros_30) {
        if (packet.command_for_state_machine == state_machine_command_t::flag_30m) {
            m_conf30m = true;
            Serial.println("Módulo 30m confirmado.");
        }
        if (packet.command_for_state_machine == state_machine_command_t::flag_100m) {
            m_conf100m = true;
            Serial.println("Módulo 100m confirmado.");
        }
        if (packet.command_for_state_machine == state_machine_command_t::end_run_30m) {
            m_runFinished30m = true;
            m_time30 = packet.time;
        }
        if (packet.command_for_state_machine == state_machine_command_t::end_run_100m) {
            m_runFinished100m = true;
            m_time100 = packet.time;
            m_time101 = packet.timer2;
        }
    }
}

void Module0m::onDataSent(const uint8_t* macAddr, esp_now_send_status_t status) {
    if (s_instance) s_instance->onDataSentHandler(status);
}

void Module0m::onDataRecv(const uint8_t* macAddr, const uint8_t* data, int len) {
    if (s_instance && len == sizeof(av_packet_t)) {
        av_packet_t p;
        memcpy(&p, data, len);
        s_instance->onDataRecvHandler(p);
    }
}

void Module0m::enableResetFlag() {
    if (s_instance) s_instance->m_canReset = true;
}