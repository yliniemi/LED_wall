#ifndef OTA_H
#define OTA_H

#include <ArduinoOTA.h>

void setupOTA(const char *hostname, const char *password);
void setupOTA(const char *hostname, const char *password, int rounds);

#endif
