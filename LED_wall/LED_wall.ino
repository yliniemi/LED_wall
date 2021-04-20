#define HOSTNAME "LEDwall"      // replace this with the name for this particular device. everyone deserves a unique name
#define OTA_ROUNDS 21             // this is how many seconds we waste waiting for the OTA during boot. sometimes people make mistakes in their code - not me - and the program freezes. this way you can still update your code over the air even if you have some dodgy code in your loop
#include <myCredentials.h>        // oh yeah. these is myCredentials.zip on the root of this repository. include it as a library and the edit the file with your onw ips and stuff

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
#define UNIVERSE_SIZE 170 //my setup is 170 leds per universe no matter if the last universe is not full.
CRGB leds[NUM_LEDS];

EspMQTTClient MQTTclient(
  //WIFI_SSID,
  //WIFI_PSK,
  MQTT_SERVER,      // MQTT Broker server ip
  1883,             // default unencrypted port is 1883
  MQTT_USERNAME,    // Can be omitted if not needed
  MQTT_PASSWORD,    // And this one too 
  HOSTNAME          // Client name that uniquely identify your device
);

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
    if (enabled) fill_solid(leds, NUM_LEDS, color);
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
  }
};

class Blinky: public MakeAll
{
  public:
  int numberOfDots = 10;

  explicit Blinky(int red, int green, int blue, bool amIenabled, int howManyDots) : MakeAll(red, green, blue, amIenabled)
  {
    // MakeAll(red, green, blue, true);
    numberOfDots = howManyDots;
  }
  
  void parse(String topic, String payload)
  {
    MakeAll::parse(topic, payload);
    if (topic.equals("dots"))
    {
      numberOfDots = payload.toInt();
      SerialOTA.println(String("The number of blinky dots is now ") + numberOfDots);
      Serial.println(String("The number of blinky dots is now ") + numberOfDots);
    }
  }
  
  void doYourThing()
  {
    if (enabled)
    {
      SerialOTA.println("blinky is alive!");
      for (int i; i < numberOfDots; i++)
      {
        int ledNumber = random(NUM_LEDS);
        leds[ledNumber].red += random(color.red);
        // SerialOTA.print(i);    // WTF. if I don't do this here, this isn't run. maybe a problem with the OTA partition?
        // SerialOTA.println(random(color.red));
        leds[ledNumber].green += random(color.green);
        // SerialOTA.println(random(color.red));
        leds[ledNumber].blue += random(color.blue);
        // SerialOTA.println(random(color.blue));
      }
      SerialOTA.println();
      SerialOTA.println("What the frell!");
    }
  }
  
};

MakeAll makeAll(1, 1, 1, false);

Blinky blinky(255, 255, 255, true, 10);

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
    leds[(int) location].red += color.red;
    leds[(int) location].green += color.green;
    leds[(int) location].blue += color.blue;
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

MovingDots movingDots;

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
    if (topic.equals(String(HOSTNAME) + "/make/fadeToBlack/multiplier"))
    {
      multiplier = payload.toInt();
      keepSane();
      SerialOTA.println(String("Changed fade multiplier to ") + multiplier);
      Serial.println(String("Changed fade multiplier to ") + multiplier);
    }
    if (topic.equals(String(HOSTNAME) + "/make/fadeToBlack/enabled"))
    {
      enabled = payload.toInt();
      SerialOTA.println(String("Changed fade to " + String((int)enabled)));
      Serial.println(String("Changed fade to " + String((int)enabled)));
    }
  }
};

FadeToBlack fadeToBlack(1, true);

void MQTTsubscriptions()
{
  MQTTclient.subscribe(String(HOSTNAME) + "/make/makeAll/+", [](String topic, String payload)
  {
    SerialOTA.println("From topic: " + topic + ", payload: " + payload);
    Serial.println("From topic: " + topic + ", payload: " + payload);
    makeAll.parse(topic.substring((String(HOSTNAME) + "/make/makeAll/").length()), payload);
    // i did this witchcraft to strip the topic of all useless stuff to help the poor parser
  });
  
  MQTTclient.subscribe(String(HOSTNAME) + "/make/blinky/+", [](String topic, String payload)
  {
    SerialOTA.println("From topic: " + topic + ", payload: " + payload);
    Serial.println("From topic: " + topic + ", payload: " + payload);
    blinky.parse(topic.substring((String(HOSTNAME) + "/make/blinky/").length()), payload);
    // i did this witchcraft to strip the topic of all useless stuff to help the poor parser
  });
  
  /*MQTTclient.subscribe(String(HOSTNAME) + "/make/dots/+", [](String topic, String payload)
  {
    SerialOTA.println("From topic: " + topic + ", payload: " + payload);
    Serial.println("From topic: " + topic + ", payload: " + payload);
    dots.parse(topic, payload);
  });*/
  
  // MQTTclient.subscribe(String(HOSTNAME) + "/make/rain/+", makeRainParse);
  
  MQTTclient.subscribe(String(HOSTNAME) + "/make/fadeToBlack/+", [](String topic, String payload)
  {
    SerialOTA.println("From topic: " + topic + ", payload: " + payload);
    Serial.println("From topic: " + topic + ", payload: " + payload);
    fadeToBlack.parse(topic, payload);
  });
  
  MQTTclient.subscribe(String(HOSTNAME) + "/make/brightness", [](String topic, String payload)
  {
    SerialOTA.println("From topic: " + topic + ", payload: " + payload);
    Serial.println("From topic: " + topic + ", payload: " + payload);
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
  set_max_power_in_volts_and_milliamps(5, 50000);   // in my current setup the maximum current is 50A
}

void loop()
{
  static unsigned long oldMillis = 0;
  unsigned long newMillis = millis();
  SerialOTA.println(String("The loop took ") + (newMillis - oldMillis) + " milliseconds");
  oldMillis = newMillis;
  randomSeed(esp_random());
  
  ArduinoOTA.handle();  
  SerialOTAhandle();
  MQTTclient.loop();

  fadeToBlack.doYourThing();
  makeAll.doYourThing();
  blinky.doYourThing();
  movingDots.doYourThing();
  
  delay(10);
  FastLED.show();
}
