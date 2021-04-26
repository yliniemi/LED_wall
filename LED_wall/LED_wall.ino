
#define HOSTNAME "LEDwall"        // replace this with the name for this particular device. everyone deserves a unique name
#define OTA_ROUNDS 21             // this is how many seconds we waste waiting for the OTA during boot. sometimes people make mistakes in their code - not me - and the program freezes. this way you can still update your code over the air even if you have some dodgy code in your loop
#include <myCredentials.h>        // oh yeah. there is myCredentials.zip on the root of this repository. include it as a library and then edit the file with your onw ips and stuff

// #include <WiFi.h>
// #include <ESPmDNS.h>
// #include <WiFiUdp.h>
#include <EspMQTTClient.h>
#include "setupWifi.h"
#include "OTA.h"
#include "SerialOTA.h"
#include "FastLED.h"
FASTLED_USING_NAMESPACE

//The following has to be adapted to your specifications
#define LED_WIDTH 8
#define LED_HEIGHT 300
#define NUM_LEDS LED_WIDTH*LED_HEIGHT
CRGB leds[NUM_LEDS];
int maxCurrent = 50000;         // in milliwatts. can be changed later on with mqtt commands. be careful with this one. it might be best to disable this funvtionality altogether
#define TRY_DISCONNECTING 15     // this is how many times we try WiFi.reconnect() before trying WiFi.disconnect()
#define TIME_TO_REBOOT 30        // this one has to be bigger that TRY_DISCONNECTING. this is how many times we try to get WiFi back before giving up and rebooting

unsigned long expectedTime = LED_HEIGHT * 24 * 2 / 800;  // this gives us double of what it should take to update one led strip. double bbecause sensing the rest pulse takes a little time

/* // This one let it control MQTT and WiFi
EspMQTTClient MQTTclient(
  //WIFI_SSID,
  //WIFI_PSK,
  MQTT_SERVER,      // MQTT Broker server ip
  MQTT_USERNAME,    // Can be omitted if not needed
  MQTT_PASSWORD,    // And this one too 
  HOSTNAME,         // Client name that uniquely identify your device
  1883              // default unencrypted port is 1883
);
*/

// This one is for letting it only handle MQTT and not WiFi
EspMQTTClient MQTTclient(
  MQTT_SERVER,        // MQTT Broker server ip
  1883,               // default unencrypted port is 1883
  // MQTT_USERNAME,   // Can be omitted if not needed
  // MQTT_PASSWORD,   // And this one too 
  HOSTNAME            // Client name that uniquely identify your device
);

// we need this function so the led values don't overflow. if we try to make a color on an led more than 255, it will overflow and the value will be small
// with this function we can limit out values to be 0 if they are negative and to 255 if they are higher than 255
// i made the function more broad so it could be used for other things
int limitInt(int value, int low, int high)
{
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

class MakeAll
{
  public:
  bool enabled = true;
  CRGB color;
  
  int interval = 100;
  int type = 0;   // 0 = static, 1 = strobe, 2 = random time
  int frameCounter = 0;
  
  MakeAll(byte red, byte green, byte blue, bool ON)
  {
    setColor(red, green, blue);
    setEnabled(ON);
  }
  
  void setColor(byte red, byte green, byte blue)
  {
    color.red = red;
    color.green = green;
    color.blue = blue;
    SerialOTA.println(String("Set everything in color ") + red + ", " + green + ", " + blue);
    Serial.println(String("Set everything in color ") + red + ", " + green + ", " + blue);
  }
  
  void setEnabled(bool ON)
  {
    enabled = ON;
  }
  
  void doYourThing()
  {
    if (enabled)
    {
      if (type == 0) fill_solid(leds, NUM_LEDS, color);
      else if (type == 1)
      {
        if (frameCounter > interval)
        {
          fill_solid(leds, NUM_LEDS, color);
          frameCounter = 0;
        }
        else frameCounter++;
      }
      else if ((type == 2) && random(interval) == 0) fill_solid(leds, NUM_LEDS, color);
    }
  }
  
  void parse(String topic, String payload)
  {
    if (topic.equals("red"))
    {
      color.red = payload.toInt();
      SerialOTA.println(String("Changed all red to ") + color.red);
      Serial.println(String("Changed all red to ") + color.red);
    }
    if (topic.equals("green"))
    {
      color.green = payload.toInt();
      SerialOTA.println(String("Changed all green to ") + color.green);
      Serial.println(String("Changed all green to ") + color.green);
    }
    if (topic.equals("blue"))
    {
      color.blue = payload.toInt();
      SerialOTA.println(String("Changed all blue to ") + color.blue);
      Serial.println(String("Changed all blue to ") + color.blue);
    }
    if (topic.equals("enabled"))
    {
      enabled = payload.toInt();
      SerialOTA.println(String("Changed to ") + enabled);
      Serial.println(String("Changed to ") + enabled);
    }
    else if (topic.equals("type"))
    {
      type = payload.toInt();
      SerialOTA.println(String("Changed to ") + type);
      Serial.println(String("Changed to ") + type);
    }
    else if (topic.equals("interval"))
    {
      interval = payload.toInt();
      SerialOTA.println(String("Changed to ") + interval);
      Serial.println(String("Changed to ") + interval);
    }
  }
};

class Blinky: public MakeAll
{
  public:
  int numberOfDots = 10;

  explicit Blinky(int red, int green, int blue, bool amIenabled, int howManyDots) : MakeAll(red, green, blue, amIenabled)
  {
    // MakeAll::MakeAll(red, green, blue, true);
    numberOfDots = howManyDots;
  }
  
  void parse(String topic, String payload)
  {
    if (topic.equals("dots"))
    {
      numberOfDots = payload.toInt();
      SerialOTA.println(String("The number of blinky dots is now ") + numberOfDots);
      Serial.println(String("The number of blinky dots is now ") + numberOfDots);
    }
    else MakeAll::parse(topic, payload);
  }
  
  void doYourThing()
  {
    if (enabled)
    {
      // SerialOTA.println("blinky is alive!");
      for (int i; i < numberOfDots; i++)
      {
        int ledNumber = random(NUM_LEDS);
        leds[ledNumber].red = limitInt(random(color.red) + leds[ledNumber].red, 0, 255);
        // SerialOTA.print(i);    // WTF. if I don't do this here, this isn't run. maybe a problem with the OTA partition?
        // SerialOTA.println(random(color.red));
        leds[ledNumber].green = limitInt(random(color.green) + leds[ledNumber].green, 0, 255);
        // SerialOTA.println(random(color.red));
        leds[ledNumber].blue = limitInt(random(color.blue) + leds[ledNumber].blue, 0, 255);
        // SerialOTA.println(random(color.blue));
      }
      // SerialOTA.println();
      // SerialOTA.println("What the frell!");
    }
  }
};

class Blur
{
  public:
  int blurAmount = 128;
  bool enabled = true;

  Blur(int amount, bool amIon)
  {
    blurAmount = amount;
    enabled = amIon;
  }

  void doYourThing()
  {
    if (enabled) blur1d(leds, NUM_LEDS, blurAmount);  
  }
  
  void parse(String topic, String payload)
  {
    if (topic.equals("amount"))
    {
      blurAmount = payload.toInt();
      SerialOTA.println(String("Changed blur amount to ") + blurAmount);
      Serial.println(String("Changed blur amount to ") + blurAmount);
    }
    if (topic.equals("enabled"))
    {
      enabled = payload.toInt();
      SerialOTA.println(String("Changed blur to " + String((int)enabled)));
      Serial.println(String("Changed blur to " + String((int)enabled)));
    }
  }
};

class Dot
{
  public:

  float location = random(NUM_LEDS);
  float speed = (float) random(1000) / 1000;
  float acceleration = 0;
  CRGB color;
  
  
  Dot()
  {
    runFirst();
  }
  void runFirst()
  {
      color.red = random(10);
      // color.red = random(255);
      color.green = random(20);
      color.blue = random(20);
  }
  
  void draw()
  {
    updateLocation();
    
    int locationInt = location;
    // this is to do bilinear filering for the dots to make them just the tiniest bit more fluid
    float multiplier = 1 - (location - locationInt);
    leds[locationInt].red = limitInt(multiplier * color.red + leds[locationInt].red, 0, 255);
    leds[locationInt].green = limitInt(multiplier * color.green + leds[locationInt].green, 0, 255);
    leds[locationInt].blue = limitInt(multiplier * color.blue + leds[locationInt].blue, 0, 255);
    
    locationInt++;
    if (locationInt >= NUM_LEDS) locationInt -= NUM_LEDS;
    multiplier = 1 - multiplier;
    leds[locationInt].red = limitInt(multiplier * color.red + leds[locationInt].red, 0, 255);
    leds[locationInt].green = limitInt(multiplier * color.green + leds[locationInt].green, 0, 255);
    leds[locationInt].blue = limitInt(multiplier * color.blue + leds[locationInt].blue, 0, 255);
  }
  
  void updateLocation()
  {
    location += speed;
    if (location < 0) location += NUM_LEDS;
    if (location > NUM_LEDS) location -= NUM_LEDS;
  }
};

class MovingDots
{
  public:
  Dot dot[100];
  
  MovingDots()
  {
    for (int i = 0; i < 100; i++)
    {
      dot[i].runFirst();
    }
  }
  
  void doYourThing()
  {
    for (int i = 0; i < 100; i++)
    {
      dot[i].draw();
    }
  }
};

class FadeToBlack
{
  public:
  bool enabled = true;
  int multiplier = 1;     // ( 255 - multiplier ) / 255 = actual multiplier
  
  FadeToBlack(int newMultiplier, bool newEnabled)
  {
    setMultiplier(newMultiplier);
    setEnabled(newEnabled);
  }
  
  void keepSane()
  {
    if (multiplier < 0) multiplier = 0;
    if (multiplier > 255) multiplier = 255;
  }

  void setMultiplier(int newMultiplier)
  {
    multiplier = newMultiplier;
    keepSane();
  }

  void setEnabled(bool newEnabled)
  {
    enabled = newEnabled;
  }
  
  void doYourThing()
  {
    if (enabled) fadeToBlackBy(leds, NUM_LEDS, multiplier);
  }

  void parse(String topic, String payload)
  {
    if (topic.equals("multiplier"))
    {
      multiplier = payload.toInt();
      keepSane();
      SerialOTA.println(String("Changed fade multiplier to ") + multiplier);
      Serial.println(String("Changed fade multiplier to ") + multiplier);
    }
    if (topic.equals("enabled"))
    {
      enabled = payload.toInt();
      SerialOTA.println(String("Changed fade to " + String((int)enabled)));
      Serial.println(String("Changed fade to " + String((int)enabled)));
    }
  }
};

MovingDots movingDots;

Blur blur(128, false);

MakeAll makeAll(1, 1, 1, false);

Blinky blinky(255, 255, 255, true, 10);

FadeToBlack fadeToBlack(20, true);

void MQTTsubscriptions()
{
  MQTTclient.subscribe(String(HOSTNAME) + "/command/makeAll/+", [](String topic, String payload)
  {
    SerialOTA.println(String("From topic: ") + topic + ", payload: " + payload);
    Serial.println(String("From topic: ") + topic + ", payload: " + payload);
    makeAll.parse(topic.substring((String(HOSTNAME) + "/command/makeAll/").length()), payload);
    // i did this witchcraft to strip the topic of all useless stuff to help the poor parser
  });
  
  MQTTclient.subscribe(String(HOSTNAME) + "/command/blinky/+", [](String topic, String payload)
  {
    SerialOTA.println(String("From topic: ") + topic + ", payload: " + payload);
    Serial.println(String("From topic: ") + topic + ", payload: " + payload);
    blinky.parse(topic.substring((String(HOSTNAME) + "/command/blinky/").length()), payload);
    // i did this witchcraft to strip the topic of all useless stuff to help the poor parser
  });
  
  MQTTclient.subscribe(String(HOSTNAME) + "/command/current", [](String topic, String payload)
  {
    SerialOTA.println(String("From topic: ") + topic + ", payload: " + payload);
    Serial.println(String("From topic: ") + topic + ", payload: " + payload);
    maxCurrent = payload.toInt();    // no safety in place. don't give this insane values so you don't burn your house down. this is here purely for debugging reasons
    set_max_power_in_volts_and_milliamps(5, maxCurrent);
  });
  
  /*MQTTclient.subscribe(String(HOSTNAME) + "/command/dots/+", [](String topic, String payload)
  {
    SerialOTA.println("From topic: " + topic + ", payload: " + payload);
    Serial.println("From topic: " + topic + ", payload: " + payload);
    dots.parse(topic, payload);
  });*/
  
  // MQTTclient.subscribe(String(HOSTNAME) + "/command/rain/+", makeRainParse);
  
  MQTTclient.subscribe(String(HOSTNAME) + "/command/fadeToBlack/+", [](String topic, String payload)
  {
    SerialOTA.println(String("From topic: ") + topic + ", payload: " + payload);
    Serial.println(String("From topic: ") + topic + ", payload: " + payload);
    fadeToBlack.parse(topic.substring((String(HOSTNAME) + "/command/fadeToBlack/").length()), payload);
  });
  
  MQTTclient.subscribe(String(HOSTNAME) + "/command/blur/+", [](String topic, String payload)
  {
    SerialOTA.println(String("From topic: ") + topic + ", payload: " + payload);
    Serial.println(String("From topic: ") + topic + ", payload: " + payload);
    blur.parse(topic.substring((String(HOSTNAME) + "/command/blur/").length()), payload);
  });
  
  MQTTclient.subscribe(String(HOSTNAME) + "/command/brightness", [](String topic, String payload)
  {
    SerialOTA.println(String("From topic: ") + topic + ", payload: " + payload);
    Serial.println(String("From topic: ") + topic + ", payload: " + payload);
    FastLED.setBrightness(limitInt(payload.toInt(), 0, 255));
  });
  
}

void onConnectionEstablished()
{
  //MQTTclient.subscribe("feed/" + OTAhostname + "/#", [](const String & topic, const String & payload)
  MQTTclient.subscribe(String("feed/") + HOSTNAME + "/#", [](const String & topic, const String & payload)
  {
    Serial.println("From wildcard topic: " + topic + ", payload: " + payload);
    SerialOTA.println("From wildcard topic: " + topic + ", payload: " + payload);
  });
  MQTTclient.publish(String(HOSTNAME) + "/alive", "I just woke up");
  MQTTsubscriptions();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  
  // comment the next line when you get the chance
  // Serial.println("Doodling around for 10 seconds for debugging reasons");
  // delay(10000);
  setupWifi(WIFI_SSID, WIFI_PSK);
  
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  setupOTA(HOSTNAME, OTApassword, OTA_ROUNDS);

  setupSerialOTA(HOSTNAME);

  // setupMQTT(HOSTNAME, MQTT_SERVER);
  MQTTclient.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  String lastWillTopic = String(HOSTNAME) + "/lastwill";
  char lastWillTopicChar[lastWillTopic.length() + 1];
  lastWillTopic.toCharArray(lastWillTopicChar, lastWillTopic.length() + 1);
  SerialOTA.println(lastWillTopicChar);
  Serial.println(lastWillTopicChar);
  // MQTTclient.enableLastWillMessage(lastWillTopicChar, "What a world, what a world!");  // For some reason this line prevents MQTT connection altogether
  // MQTTsubscriptions();     // we call this elsewhere
  
  FastLED.addLeds<NEOPIXEL, 4>(leds, 0*LED_HEIGHT, LED_HEIGHT);
  FastLED.addLeds<NEOPIXEL, 5>(leds, 1*LED_HEIGHT, LED_HEIGHT);
  FastLED.addLeds<NEOPIXEL, 27>(leds, 2*LED_HEIGHT, LED_HEIGHT);
  FastLED.addLeds<NEOPIXEL, 18>(leds, 3*LED_HEIGHT, LED_HEIGHT);
  FastLED.addLeds<NEOPIXEL, 19>(leds, 4*LED_HEIGHT, LED_HEIGHT);
  FastLED.addLeds<NEOPIXEL, 23>(leds, 5*LED_HEIGHT, LED_HEIGHT);
  FastLED.addLeds<NEOPIXEL, 32>(leds, 6*LED_HEIGHT, LED_HEIGHT);
  FastLED.addLeds<NEOPIXEL, 33>(leds, 7*LED_HEIGHT, LED_HEIGHT);
  
  randomSeed(esp_random());
  set_max_power_in_volts_and_milliamps(5, maxCurrent);   // in my current setup the maximum current is 50A
  if (expectedTime < 10) expectedTime = 10;
}

void loop()
{
  randomSeed(esp_random());
  
  SerialOTAhandle();
  if (WiFi.status() == WL_CONNECTED)
  {
    ArduinoOTA.handle();    // This is so that the MQTT or OTA doesn't go crazy and try to reconnect to a server when there is no WiFi
    MQTTclient.loop();
  }

  fadeToBlack.doYourThing();

  // The last ones are additive so it doesn't matter what order you put them
  makeAll.doYourThing();
  blinky.doYourThing();
  movingDots.doYourThing();
  blur.doYourThing();
  
  static unsigned long oldMillis = 0;
  unsigned long newMillis = millis();
  unsigned long frameTime = newMillis - oldMillis;
  static unsigned long previousTime = 0;
  if ((millis() - previousTime > 10000) || (millis() < previousTime))
  {
    static String reconnectedAt = "";
    static bool beenDisconnected = false;
    Serial.println();
    SerialOTA.println();
    Serial.println(reconnectedAt);
    SerialOTA.println(reconnectedAt);
    Serial.println(String("The loop took ") + frameTime + " milliseconds");
    SerialOTA.println(String("The loop took ") + frameTime + " milliseconds");
    
    Serial.print("WiFi connections status: ");
    Serial.println(WiFi.status());
    SerialOTA.print("WiFi connections status: ");
    SerialOTA.println(WiFi.status());
    previousTime = millis();
    Serial.print(String("I have been on for ") + previousTime / (1000 * 60 * 60) + " hours, ");
    Serial.print(String((previousTime / (1000 * 60)) % 60) + " minutes and ");
    Serial.println(String((previousTime / 1000) % 60) + " seconds");
    SerialOTA.print(String("I have been on for ") + previousTime / (1000 * 60 * 60) + " hours, ");
    SerialOTA.print(String((previousTime / (1000 * 60)) % 60) + " minutes and ");
    SerialOTA.println(String((previousTime / 1000) % 60) + " seconds");
    static int tryNumber = 0;
    if (WiFi.status() != WL_CONNECTED)
    {
      reconnectedAt += WiFi.status();   // im doing this to differentiate regular temporary WiFi outage and a long one
      if (tryNumber > TRY_DISCONNECTING)
      {
        if (tryNumber > TIME_TO_REBOOT)
        {
          Serial.println("Couldn't reconnect to WiFi no matter what. Giving up and rebooting");
          delay(1000);
          ESP.restart();
        }
        else
        {
          Serial.println("WiFi has been down for too long. Trying to force disconnect.");
          WiFi.disconnect();
          delay(1000);
          WiFi.mode(WIFI_STA);
          WiFi.begin(WIFI_SSID, WIFI_PSK);
          delay(1000);
        }
      }
      else
      {
        Serial.println("God damn it! WiFi is lost. Trying to reconnect.");
        
        WiFi.reconnect();
        // WiFi.reconnect() resulted in a crash half the time
        // using events or WiFi.setAutoReconnect(true) didn't help. everything just kept crashing anyways
        // i finally cracked the problem
        // the solution is to have core 1 do nothing for some time. there is delay(1000) right now but i will reduce it when i do more testing
        // the problem was FastLED.show() starting right after WiFi.reconnect(). core 1 didn't like that
        // even though WiFi is run on core 0, core 1 does something really important right after connection is established
        delay(1000);   // maybe this will make reconnecting more reliable. who knows???   IT DID!!!!!   False alarm. It didn't
        
        beenDisconnected = true;        
      }
      tryNumber++;
    }
    else if (beenDisconnected)
    {
      reconnectedAt += String(" WiFi reconnected at: ") + previousTime / (1000 * 60 * 60) + ":" + previousTime / (1000 * 60) % 60 + ":" + previousTime / 1000 % 60;      
      reconnectedAt += String(" after ") + tryNumber + " tries\r\n";
      beenDisconnected = false;
      tryNumber = 0;
    }
  }
  if (frameTime < expectedTime) delay(expectedTime - frameTime);
  oldMillis = millis();
  FastLED.show();
}
