#include "AV_espnow.h"

esp_now_peer_info_t peerInfo;

bool init_esp_now()
{
    return esp_now_init() == ESP_OK;
}

bool register_peer(uint8_t *mac)
{
    // Register peer
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // add peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        return false;
    }
    return true;
}

bool register_receive_callback(esp_now_receiver_callback_t receive_callback)
{
    return esp_now_register_recv_cb(receive_callback) == ESP_OK;
}

bool register_transmitter_callback(esp_now_transmitter_callback_t transmitter_callback)
{
    return esp_now_register_send_cb(transmitter_callback) == ESP_OK;
}

bool sent_to_all(void *AnyMessage, int size)
{
    // Broadcast a message to every device in range
    uint8_t BroadcastAdress[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    esp_now_peer_info_t BroadInfo = {};

    memcpy(&BroadInfo.peer_addr, BroadcastAdress, 6);

    if (!esp_now_is_peer_exist(BroadcastAdress))
        esp_now_add_peer(&BroadInfo);

    // Send message
    esp_err_t result = esp_now_send(BroadcastAdress, (uint8_t*)AnyMessage, size);

    return result == ESP_OK;
}

bool sent_to_single(void *AnyMessage, int size, uint8_t *address_from_receive)
{
    esp_err_t result = esp_now_send(address_from_receive, (uint8_t*)AnyMessage, size);

    return result == ESP_OK;
}