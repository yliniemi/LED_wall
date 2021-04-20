#include "setupWifi.h"

/*
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  // Serial.println("Connection Lost! Rebooting...");
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.reconnect();
  delay(5000);
  // ESP.restart();
}
*/

void setupWifi(char *ssid, char *psk)
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, psk);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  // WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
}
