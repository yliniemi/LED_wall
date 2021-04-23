#ifndef SETUPWIFI_H
#define SETUPWIFI_H
#define RECONNECT_TIME 60  // in multiples of 10 seconds. this is how many times we will try to reconnect for once every 10 seconds

#include <WiFi.h>


void setupWifi(char *ssid, char *psk);
void reconnectWiFiOrDie();

#endif
