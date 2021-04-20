#ifndef OTA_H
#define OTA_H

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

void setupOTA(const char *hostname, const char *password);
void setupOTA(const char *hostname, const char *password, int rounds);

#endif
