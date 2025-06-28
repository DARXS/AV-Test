#ifndef PACKETS_H
#define PACKETS_H

#include <sys/_stdint.h>

/**
 * @enum module_t
 * @brief Identifica o tipo de módulo que está enviando uma mensagem.
 */
typedef enum
{
    metros_0 = 0,   // Módulo ECU (0 metros) com LCD e SD Card
    metros_30 = 1,  // Módulo sensor em 30 metros
    metros_100 = 2, // Módulo sensor em 100/101 metros
    bridge = 3      // Módulo ponte/repetidor
} module_t;

/**
 * @enum state_machine_command_t
 * @brief Define os comandos que controlam as máquinas de estado dos módulos.
 */
typedef enum
{
    do_nothing = 0xff,      // Comando nulo, para preenchimento
    check_module = 0x00,    // Comando inicial para verificar quais módulos estão online
    cancel = 0x03,          // Comando para cancelar uma corrida em andamento
    start_run = 0x04,       // Comando da ECU para iniciar a cronometragem em todos os módulos
    flag_30m = 0x05,        // Flag de confirmação enviada pelo módulo de 30m
    end_run_30m = uint8_t(~0x05), // Comando do módulo 30m com o tempo da passagem
    flag_100m = 0x06,       // Flag de confirmação enviada pelo módulo de 100m
    end_run_100m = uint8_t(~0x06),// Comando do módulo 100m com os tempos de passagem
    reset_ = 0x07           // Comando para reiniciar todos os módulos
} state_machine_command_t;

/**
 * @struct av_packet_t
 * @brief Estrutura de dados principal para toda a comunicação ESP-NOW.
 */
typedef struct
{
    // --- CAMPOS PARA REDE ADAPTATIVA ---
    /// @brief ID único para cada mensagem, para evitar loops de retransmissão.
    uint32_t message_id;
    /// @brief Endereço MAC do remetente que originou a mensagem.
    uint8_t original_sender_mac[6];
    // ------------------------------------

    /// @brief ID do módulo que está enviando/retransmitindo, conforme `module_t`.
    uint8_t id;

    /// @brief Comando para a máquina de estados, conforme `state_machine_command_t`.
    uint8_t command_for_state_machine = state_machine_command_t::do_nothing;

    /// @brief Endereço MAC do remetente original (usado pela ECU para registrar peers).
    uint8_t mac_address[6];

    /// @brief Timestamp principal (ex: tempo em 30m ou tempo em 100m).
    unsigned long time;

    /// @brief Timestamp secundário (usado para o tempo em 101m).
    unsigned long timer2 = 0;
} av_packet_t;

/**
 * @enum state_t
 * @brief Define os estados operacionais para os módulos sensores (30m e 100m).
 */
typedef enum
{
    wait = 0,       // Estado ocioso, aguardando comando de início
    __setup__ = 1,  // Estado de preparação para a corrida
    run = 2         // Estado de execução, aguardando o sensor ser acionado
} state_t;

/**
 * @enum av_ecu_t
 * @brief Define os estados da máquina de estados principal da ECU (módulo 0m).
 */
typedef enum
{
    initializing = -1,  // Estado de inicialização e verificação de peers.
    menu = 0,           // Menu principal
    __start_run__ = 1,  // Estado que gerencia a corrida
    wait_to_start = 2,  // Sub-estado: aguardando o carro passar pelo sensor de 0m
    lcd_display = 3,    // Sub-estado: corrida em andamento, exibindo tempos no LCD
    end_run = 4,        // Sub-estado: corrida finalizada, exibindo resultados
    save_run = 5        // Estado para salvar os dados no cartão SD
} av_ecu_t;

#endif // PACKETS_H