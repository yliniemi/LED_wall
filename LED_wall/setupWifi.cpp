#include "setupWifi.h"

char *ssid;
char *psk;

void reconnectWiFiOrDie()
{
  Serial.print("Trying to Reconnect for ");
  Serial.print(RECONNECT_TIME * 10);
  Serial.println(" seconds");
  int tryNumber = 0;
  // while ((WiFi.status() != WL_CONNECTED) && (tryNumber < RECONNECT_TIME))
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Lost WiFi. Trying to reconnect. Try number ");
    Serial.println(tryNumber);
    // WiFi.disconnect();
    delay(2000);
    // WiFi.reconnect();  // I had some stupid crashes. maybe changin to WiFi.disconnect WiFi.begin will solve it
    WiFi.mode(WIFI_STA);
    delay(2000);
    WiFi.begin(ssid, psk);
    delay(6000);
    tryNumber++;
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    ESP.restart();
    Serial.println("Giving up and killing myself");
  }
  Serial.print("Somehow survived after ");
  Serial.print(tryNumber);
  Serial.print(" of ");
  Serial.print(RECONNECT_TIME);
  Serial.println(" tries");
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  // Serial.println("Connection Lost! Rebooting...");
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.disconnected.reason);
  reconnectWiFiOrDie();
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupWifi(char *ssidTemp, char *pskTemp)
{
  ssid = ssidTemp;
  psk = pskTemp;
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, psk);
  delay(1000);    // let's be nice here and give core 1 some down time
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // these didn't seem to do the trick so i had to bring te big guns - events
  // they didn't help with crashing on WiFi.reconnect()
  // you can read more about the problem and the solution in the main loop
  WiFi.persistent(false);
  // WiFi.setAutoReconnect(true);
  
  /*
  WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  */
  
  // WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
}
