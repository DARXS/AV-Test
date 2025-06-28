#ifndef SDDEVICE_H
#define SDDEVICE_H

#include <Arduino.h>
#include <SD.h>

/**
 * @brief Inicializa o módulo SD Card e cria um novo arquivo .csv para os dados da corrida.
 * * A função verifica o número de arquivos existentes para criar um nome único
 * (ex: AV_data0.csv, AV_data1.csv, etc.), garantindo que nenhum dado anterior
 * seja sobrescrito.
 * * @param pin O pino Chip Select (CS) ao qual o módulo SD está conectado.
 * @return `true` se o SD foi inicializado e o arquivo foi criado com sucesso, 
 * `false` caso contrário.
 */
bool init_sd(uint8_t pin);

/**
 * @brief Salva uma linha de dados da corrida no arquivo CSV previamente aberto.
 * * @param _t30 O tempo final da passagem em 30 metros.
 * @param _t100 O tempo final da passagem em 100 metros.
 * @param _v A velocidade final calculada em km/h.
 * @return `true` se os dados foram escritos com sucesso no arquivo, 
 * `false` caso contrário.
 */
bool save_AV_Data(unsigned long _t30, unsigned long _t100, float _v);

#endif // SDDEVICE_H