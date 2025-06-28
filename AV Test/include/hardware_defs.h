#ifndef HARDWARE_DEFS_H
#define HARDWARE_DEFS_H

#include <driver/gpio.h>

//==================================================================
// Módulo 0m (ECU com LCD/SD)
//==================================================================

/* Pinos do Módulo SD Card */
#define SD_CS       GPIO_NUM_2  // Pino Chip Select do módulo SD

/* Pinos de Controle da Interface do Usuário */
#define POT         GPIO_NUM_35 // Pino do potenciômetro para seleção no menu
#define B_SEL       GPIO_NUM_14 // Pino do botão de Seleção/Confirmação
#define B_CANCEL    GPIO_NUM_13 // Pino do botão para Cancelar/Resetar

/* Pino do Sensor de Início/Fim */
#define SENSOR_0m   GPIO_NUM_15 // Pino do sensor na linha de partida (0 metros)


//==================================================================
// Módulo 30m
//==================================================================
#define SENSOR_30m  GPIO_NUM_5  // Pino do sensor de 30 metros


//==================================================================
// Módulo 100m
//==================================================================
#define SENSOR_100m GPIO_NUM_5  // Pino do sensor de 100 metros
#define SENSOR_101m GPIO_NUM_18 // Pino do sensor de 101 metros


#endif // HARDWARE_DEFS_H