#ifndef SETTINGS_H
#define SETTINGS_H

#define PRE_CONFIGURED_WIFI_SSID                "Mezzomo e Spode"
#define PRE_CONFIGURED_WIFI_PASSWORD            "****"

// WiFi initialization timeout (in milliseconds)
// System will continue initialization even if WiFi doesn't connect within this timeout
// WiFi will continue trying to reconnect in background automatically
#define WIFI_INIT_TIMEOUT_MS                           10000   // 10 seconds timeout for initial WiFi connection
#endif // SETTINGS_H