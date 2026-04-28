#ifndef MAIN_H
#define MAIN_H

#include <string>

void publishCurrentState(void);
std::string get_device_id(void);
void on_wifi_connected(void);
void on_wifi_disconnected(void);
void mqtt_message_handler(const char *topic, const char *message);
void mqtt_connected_handler(const char *broker_uri);
#endif // MAIN_H