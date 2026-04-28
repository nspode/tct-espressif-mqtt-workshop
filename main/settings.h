#ifndef SETTINGS_H
#define SETTINGS_H

#define PRE_CONFIGURED_WIFI_SSID                ""
#define PRE_CONFIGURED_WIFI_PASSWORD            ""

#define MQTT_BROKER_URI                         "" 
#define DUMMY_DEVICE_ID                         "DEV_123"

#define MQTT_TIMEOUT_MS 10000                   // Timeout da conexão MQTT em milissegundos
#define MQTT_KEEPALIVE_S 15                     // Keep-alive do MQTT em segundos

// WiFi initialization timeout (in milliseconds)
// System will continue initialization even if WiFi doesn't connect within this timeout
// WiFi will continue trying to reconnect in background automatically
#define WIFI_INIT_TIMEOUT_MS                           10000   // 10 seconds timeout for initial WiFi connection
#endif // SETTINGS_H