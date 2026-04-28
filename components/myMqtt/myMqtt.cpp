#include "myMqtt.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "esp_random.h"
#include "esp_mac.h"
#include "esp_tls.h"
#include <cstring>
#include <cstdio>
#include "settings.h"

namespace MYMQTT
{
    static const char *TAG = "MY_MQTT";
    static esp_mqtt_client_handle_t client;
    static mqtt_message_callback_t user_callback = NULL;
    static mqtt_connected_callback_t connected_callback = NULL;
    static bool g_mqtt_is_connected = false;

    void register_mqtt_callback(mqtt_message_callback_t callback)
    {
        user_callback = callback;
    }

    void register_mqtt_connected_callback(mqtt_connected_callback_t callback)
    {
        connected_callback = callback;
    }

    esp_err_t mqtt_subscribe_topic(const char *topic, int qos)
    {
        if (client)
        {
            return esp_mqtt_client_subscribe(client, topic, qos);
        }
        return ESP_FAIL;
    }

    static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
    {
        esp_mqtt_event_handle_t event = reinterpret_cast<esp_mqtt_event_handle_t>(event_data);

        switch (event_id)
        {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Conectado ao broker MQTT.");
            if (connected_callback)
            {
                connected_callback(MQTT_BROKER_URI);
            }
            g_mqtt_is_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGE(TAG, "Desconectado do broker MQTT.");
            // esp_mqtt_client_start(client);
            g_mqtt_is_connected = false;
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Recebido no tópico: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Dados: %.*s", event->data_len, event->data);
            if (user_callback)
            {
                char topic[event->topic_len + 1];
                char message[event->data_len + 1];

                strncpy(topic, event->topic, event->topic_len);
                topic[event->topic_len] = '\0';

                strncpy(message, event->data, event->data_len);
                message[event->data_len] = '\0';

                user_callback(topic, message);
            }
            break;
        default:
            ESP_LOGI(TAG, "Evento não tratado: %ld", (long)event_id);
            break;
        }
    }

    esp_err_t init_mqtt_with_aws_iot_certs(const char *broker_uri, const char *device_id, const uint8_t *CA_certificate, const size_t CA_certificate_len, const uint8_t *device_cert, const size_t device_cert_len, const uint8_t *device_private_key, const size_t device_private_key_len)
    {
        ESP_LOGI(TAG, "Inicializando MQTT para AWS IoT Core...%s", broker_uri);

        // Gera um client_id baseado no MAC do dispositivo
        // generate_mac_client_id();

        ESP_ERROR_CHECK(esp_tls_init_global_ca_store());
        ESP_ERROR_CHECK(esp_tls_set_global_ca_store((const unsigned char *)CA_certificate,
        CA_certificate_len));
        
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        esp_mqtt_client_config_t mqtt5_cfg = {};

        // Configure broker connection
        mqtt5_cfg.broker.address.hostname = broker_uri;
        mqtt5_cfg.broker.address.port = 8883;
        mqtt5_cfg.broker.address.transport = MQTT_TRANSPORT_OVER_SSL;
        mqtt5_cfg.broker.verification.use_global_ca_store = true;
        // mqtt5_cfg.credentials.client_id = mqtt_client_id;
        mqtt5_cfg.credentials.client_id = device_id;

        mqtt5_cfg.credentials.authentication.certificate = (const char *)device_cert;
        mqtt5_cfg.credentials.authentication.certificate_len = device_cert_len;
        mqtt5_cfg.credentials.authentication.key = (const char *)device_private_key;  
        mqtt5_cfg.credentials.authentication.key_len = device_private_key_len;
        mqtt5_cfg.task.priority = 5;
        mqtt5_cfg.task.stack_size = 1024 * 8;

        mqtt5_cfg.session.protocol_ver = MQTT_PROTOCOL_V_5;
        mqtt5_cfg.network.disable_auto_reconnect = false;
        mqtt5_cfg.session.disable_clean_session = 1;
        mqtt5_cfg.network.timeout_ms = MQTT_TIMEOUT_MS;
        mqtt5_cfg.session.keepalive = MQTT_KEEPALIVE_S;

        // Configure Last Will and Testament (LWT)
        // The Last Will message will be published automatically by the broker when the device
        // disconnects unexpectedly (e.g., power loss, network failure, crash).
        // Estimated trigger time: ~90-120 seconds after unexpected disconnection.
        // This is based on the keepalive period (MQTT_KEEPALIVE_S = 60s) multiplied by 1.5x,
        // which is the typical broker timeout before considering a client disconnected.
        if (device_id && strlen(device_id) > 0) {
            static char lwt_topic[128];
            snprintf(lwt_topic, sizeof(lwt_topic), "/techday/%s/reports/", device_id);
            mqtt5_cfg.session.last_will.topic = lwt_topic;
            mqtt5_cfg.session.last_will.msg = "offline";
            mqtt5_cfg.session.last_will.msg_len = 7;
            mqtt5_cfg.session.last_will.qos = 1;
            mqtt5_cfg.session.last_will.retain = true;            
        }
   
        client = esp_mqtt_client_init(&mqtt5_cfg);
        if (!client)
        {
            ESP_LOGE(TAG, "Falha ao inicializar o cliente MQTT AWS IoT");
            return ESP_FAIL;
        }

        esp_mqtt_client_register_event(client, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), mqtt_event_handler, NULL);
        return esp_mqtt_client_start(client);
    }


    void publish_message(const char *topic, const char *message)
    {
        if (client)
        {
            esp_mqtt_client_publish(client, topic, message, 0, 0, 0);
            ESP_LOGI(TAG, "Mensagem publicada no tópico: %s", topic);
        }
    }

    void publish_status(const char *device_id, const char *status)
    {
        if (client && device_id && strlen(device_id) > 0)
        {
            char status_topic[128];
            snprintf(status_topic, sizeof(status_topic), "%s/status/", device_id);
            esp_mqtt_client_publish(client, status_topic, status, strlen(status), 1, 1); // QoS 1, retain true
            ESP_LOGI(TAG, "Status publicado no tópico: %s", status_topic);
        }
    }

    bool is_mqtt_connected()
    {
        return g_mqtt_is_connected;
    }
}