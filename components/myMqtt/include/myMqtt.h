#ifndef MYMQTT_H
#define MYMQTT_H

#include "esp_err.h"

namespace MYMQTT
{
    // Inicializa o cliente MQTT    
    esp_err_t init_mqtt_with_no_cert(const char *broker_uri, const char *device_id);    
    
    // Publica mensagens no tópico MQTT
    void publish_message(const char *topic, const char *message);
    
    // Publica status do dispositivo (online/offline)
    void publish_status(const char *device_id, const char *status);

    // Definição de um ponteiro de função para o callback de mensagens MQTT
    // é um ponteiro para uma função que recebe dois parâmetros e não retorna nada.
    typedef void (*mqtt_message_callback_t)(const char *topic, const char *message);

    // Definição de um ponteiro de função para o callback chamado em main.cpp quando a conexão MQTT é estabelecida.
    // é um ponteiro para uma função que recebe 1 parâmetros e não retorna nada.
    typedef void (*mqtt_connected_callback_t)(const char *broker_uri);

    // Registra o callback de mensagens MQTT
    void register_mqtt_callback(mqtt_message_callback_t callback);

    // Registra o callback chamado em main.cpp quando a conexão MQTT é estabelecida.
    void register_mqtt_connected_callback(mqtt_connected_callback_t callback);

    // Inscreve-se em um tópico MQTT
    esp_err_t mqtt_subscribe_topic(const char *topic, int qos);
    
    // Retorna o estado da conexão MQTT
    bool is_mqtt_connected();

}

#endif // MYMQTT_H