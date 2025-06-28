#ifndef AV_ESPNOW_H
#define AV_ESPNOW_H

#include <Arduino.h>
#include <esp_now.h>

/**
 * @brief Define o tipo de uma função de callback para recebimento de dados via ESP-NOW.
 * @param const uint8_t* Endereço MAC do remetente.
 * @param const uint8_t* Ponteiro para os dados recebidos.
 * @param int Tamanho (em bytes) dos dados recebidos.
 */
typedef void (*esp_now_receiver_callback_t)(const uint8_t *, const uint8_t *, int);

/**
 * @brief Define o tipo de uma função de callback para status de envio de dados via ESP-NOW.
 * @param const uint8_t* Endereço MAC do destinatário.
 * @param esp_now_send_status_t Status do envio (ESP_NOW_SEND_SUCCESS ou ESP_NOW_SEND_FAIL).
 */
typedef void (*esp_now_transmitter_callback_t)(const uint8_t *, esp_now_send_status_t);

/**
 * @brief Inicializa o sistema ESP-NOW.
 * @return `true` se a inicialização for bem-sucedida, `false` caso contrário.
 */
bool init_esp_now(void);

/**
 * @brief Registra um novo dispositivo (peer) na rede ESP-NOW para comunicação direta.
 * @param mac Endereço MAC de 6 bytes do peer a ser registrado.
 * @return `true` se o peer for adicionado com sucesso, `false` caso contrário.
 */
bool register_peer(uint8_t *mac);

/**
 * @brief Registra a função de callback que será chamada quando um pacote for recebido.
 * @param receive_callback A função a ser registrada.
 * @return `true` se o registro for bem-sucedido, `false` caso contrário.
 */
bool register_receive_callback(esp_now_receiver_callback_t receive_callback);

/**
 * @brief Registra a função de callback que será chamada após uma tentativa de envio de pacote.
 * @param transmitter_callback A função a ser registrada.
 * @return `true` se o registro for bem-sucedido, `false` caso contrário.
 */
bool register_transmitter_callback(esp_now_transmitter_callback_t transmitter_callback);

/**
 * @brief Envia uma mensagem para todos os dispositivos ESP-NOW ao alcance (broadcast).
 * @param AnyMessage Ponteiro para a estrutura de dados a ser enviada.
 * @param size Tamanho (em bytes) da estrutura de dados.
 * @return `true` se o envio for enfileirado com sucesso, `false` caso contrário.
 */
bool sent_to_all(void *AnyMessage, int size);

/**
 * @brief Envia uma mensagem para um único dispositivo (peer) específico.
 * @param AnyMessage Ponteiro para a estrutura de dados a ser enviada.
 * @param size Tamanho (em bytes) da estrutura de dados.
 * @param address_from_receive Endereço MAC do destinatário.
 * @return `true` se o envio for enfileirado com sucesso, `false` caso contrário.
 */
bool sent_to_single(void *AnyMessage, int size, uint8_t *address_from_receive);

#endif // AV_ESPNOW_H