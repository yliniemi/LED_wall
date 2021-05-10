#include "settings.h"
#include <myCredentials.h>        // oh yeah. there is myCredentials.zip on the root of this repository. include it as a library and then edit the file with your onw ips and stuff

#include <vector>

#include "setupWifi.h"
#include "OTA.h"

#ifdef USING_SERIALOTA
#include "SerialOTA.h"
#endif

#include "FastLED.h"
#include <EspMQTTClient.h>
FASTLED_USING_NAMESPACE

//The following has to be adapted to your specifications
#define NUM_LEDS LED_WIDTH*LED_HEIGHT
CRGB leds[NUM_LEDS];
int maxCurrent = MAX_CURRENT;         // in milliwatts. can be changed later on with mqtt commands. be careful with this one. it might be best to disable this funvtionality altogether

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

float randomF()
{
  return (float) random(RANDOM_RESOLUTION) * 2 / RANDOM_RESOLUTION - 1;
}

float random01()
{
  return (float) random(RANDOM_RESOLUTION) / RANDOM_RESOLUTION;
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
    else if (topic.equals("green"))
    {
      color.green = payload.toInt();
      SerialOTA.println(String("Changed all green to ") + color.green);
      Serial.println(String("Changed all green to ") + color.green);
    }
    else if (topic.equals("blue"))
    {
      color.blue = payload.toInt();
      SerialOTA.println(String("Changed all blue to ") + color.blue);
      Serial.println(String("Changed all blue to ") + color.blue);
    }
    else if (topic.equals("enabled"))
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
      for (int i = 0; i < numberOfDots; i++)
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
  float speed = 5;
  float acceleration = 0;
  float hue = 128;  // can be bigger than 255 because we will take a modulus of this
  float hueChange = 0;
  float lightness = 0;
  float lightnessChange = 0;
  CRGB color = 0xFFFFFF;
  
  Dot(float location_, float speed_, float acceleration_, float hue_, int hueChange_, int lightness_,  float lightnessChange_)
  {
    location = location_;
    speed = speed_;
    acceleration = acceleration_;
    hue = hue_;
    hueChange = hueChange_;
    lightness = lightness_;
    lightnessChange = lightnessChange_;
  }
  
  void updateSpeed(float speedMin, float speedMax, float accelerationMax)
  {
    speed = speed + acceleration;
    
    if (speed < speedMin)
    {
      acceleration = (float) random01() * accelerationMax;
      speed = speedMin + acceleration;
    }
    
    else if (speed > speedMax)
    {
      acceleration = (float) -1 * random01() * accelerationMax;
      speed = speedMax + acceleration;
    }
  }
  
  void updateHue(int hueMin, int hueMax, float hueChangeMax)
  {
    hue = (float) hue + hueChange;
    
    if (hue <= hueMin)
    {
      hueChange = (float) random01() * hueChangeMax;
      hue = hueMin + hueChange;
    }
    
    else if (hue >= hueMax)
    {
      hueChange = (float) -1 * random01() * hueChangeMax;
      hue = hueMax + hueChange;
    }
    
    hsv2rgb_rainbow(CHSV(((int) hue) % 256, 255, (int) lightness), color);
  }
  
  void updateLightness(int lightnessMin, int lightnessMax, float lightnessChangeMax)
  {
    lightness = (float) lightness + lightnessChange;
    
    if (lightness <= lightnessMin)
    {
      lightnessChange = (float) random01() * lightnessChangeMax;
      lightness = lightnessMin + lightnessChange;
    }
    
    else if (lightness >= lightnessMax)
    {
      lightnessChange = (float) -1 * random01() * lightnessChangeMax;
      lightness = lightnessMax + lightnessChange;
    }
  }
  
  void updateLocation(int locationMin, int locationMax)
  {
    location = location + speed;
    if (location < locationMin) location = (float) location + locationMax - locationMin;
    else if (location >= locationMax) location = (float) location - locationMax + locationMin;
  }
  
  void draw(int locationMin, int locationMax)
  {
    int locationInt = location;
    // this is to do bilinear filering for the dots to make them just the tiniest bit more fluid
    float multiplier = 1 - (location - locationInt);
    leds[locationInt].red = limitInt(multiplier * color.red + leds[locationInt].red, 0, 255);
    leds[locationInt].green = limitInt(multiplier * color.green + leds[locationInt].green, 0, 255);
    leds[locationInt].blue = limitInt(multiplier * color.blue + leds[locationInt].blue, 0, 255);
    
    locationInt++;
    if (locationInt >= locationMax) locationInt = locationInt - locationMax + locationMin;
    multiplier = 1 - multiplier;
    leds[locationInt].red = limitInt(multiplier * color.red + leds[locationInt].red, 0, 255);
    leds[locationInt].green = limitInt(multiplier * color.green + leds[locationInt].green, 0, 255);
    leds[locationInt].blue = limitInt(multiplier * color.blue + leds[locationInt].blue, 0, 255);
  }  
};

class MovingDots
{
  public:
  
  float speedMin = -1;
  float speedMax = 1;
  float accelerationMax = 0.1;
  int accelerationInterval = 1000;
  int hueMin = 0;
  int hueMax = 255;
  float hueChangeMax = 1.1;
  int hueInterval = 1000;
  int lightnessMin = 0;
  int lightnessMax = 255;
  float lightnessChangeMax;
  int lightnessInterval;
  int locationMin = 0;
  int locationMax = NUM_LEDS;
  int numDots = 10;
  
  std::vector <Dot> dots;
  
  MovingDots(float speedMin_, float speedMax_, float accelerationMax_, int accelerationInterval_, int hueMin_, int hueMax_, float hueChangeMax_, int hueInterval_, int lightnessMin_, int lightnessMax_,  float lightnessChangeMax_, int lightnessInterval_, int locationMin_, int locationMax_, int numDots_)
  {
    speedMin = speedMin_;
    speedMax = speedMax_;
    accelerationMax = accelerationMax_;
    accelerationInterval = accelerationInterval_;
    hueMin = hueMin_;
    hueMax = hueMax_;
    hueChangeMax = hueChangeMax_;
    hueInterval = hueInterval_;
    lightnessMin = lightnessMin_;
    lightnessMax = lightnessMax_;
    lightnessChangeMax = lightnessChangeMax_;
    lightnessInterval = lightnessInterval_;
    locationMin = locationMin_;
    locationMax = locationMax_;
    numDots = numDots_;
    
    for (int i = 0; i < numDots; i++)
    {
      // Dot(float location_, float speed_, float acceleration_, float hue_, hueChange_, int lightness_,  float lightnessChange_)
      dots.push_back(Dot(random(NUM_LEDS), randomF(), 2.1, 128, 0, lightnessMax, 0));
    }
  }
  
  void doYourThing()
  {
    for(int i=0; i < dots.size(); i++)
    {
      randomEncounter(i);
      dots[i].updateSpeed(speedMin, speedMax, accelerationMax);
      dots[i].updateLocation(locationMin, locationMax);
      dots[i].updateHue(hueMin, hueMax, hueChangeMax);
      dots[i].updateLightness(lightnessMin, lightnessMax, lightnessChangeMax);
      dots[i].draw(locationMin, locationMax);             // i had to give this function min and max so it doesn't draw on wrong pixels when doing bilinear filtering right next to the max
      // debug60seconds();
    }
  }
  
  void debug60seconds()
  {
      static unsigned long previousTime = 0;
      if ((millis() - previousTime > 60000) || (millis() < previousTime))
      {
        previousTime = millis();
        SerialOTA.println(String("speedMin : ") + speedMin);
        SerialOTA.println(String("speedMax : ") + speedMax);
        SerialOTA.println(String("accelerationMax : ") + accelerationMax);
        SerialOTA.println(String("hueMin : ") + hueMin);
        SerialOTA.println(String("hueMax : ") + hueMax);
        SerialOTA.println(String("hueChangeMax : ") + hueChangeMax);
        SerialOTA.println(String("hueInterval : ") + hueInterval);
        SerialOTA.println(String("lightnessMin : ") + lightnessMin);
        SerialOTA.println(String("lightnessMax : ") + lightnessMax);
        SerialOTA.println(String("lightnessChangeMax : ") + lightnessChangeMax);
        SerialOTA.println(String("lightnessInterval : ") + lightnessInterval);
        SerialOTA.println(String("locationMin : ") + locationMin);
        SerialOTA.println(String("locationMax : ") + locationMax);
        SerialOTA.println(String("numDots : ") + numDots);
        SerialOTA.println();
        SerialOTA.println(String("location : ") + dots[0].location);
        SerialOTA.println(String("speed : ") + dots[0].speed);
        SerialOTA.println(String("acceleration : ") + dots[0].acceleration);
        SerialOTA.println(String("hue : ") + dots[0].hue);
        SerialOTA.println(String("hueChange : ") + dots[0].hueChange);
        SerialOTA.println(String("lightness : ") + dots[0].lightness);
        SerialOTA.println(String("lightnessChange : ") + dots[0].lightnessChange);
      }
  }
  
  void randomEncounter(int i)
  {
    if (random(accelerationInterval) == 0)
    {
      dots[i].acceleration = randomF() * accelerationMax;
    }
    if (random(hueInterval) == 0)
    {
      dots[i].hueChange = randomF() * hueChangeMax;
    }
    if (random(lightnessInterval) == 0)
    {
      dots[i].lightnessChange = randomF() * lightnessChangeMax;
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

// MovingDots(speedMin, speedMax, accelerationMax, accelerationInterval, hueMin, hueMax, hueChangeMax, hueInterval, lightnessMin, lightnessMax,  lightnessChangeMax, lightnessInterval, locationMin, locationMax, numDots)
MovingDots movingDots(-0.3, 0.3, 0.005, 1200, 100, 200, 1.3, 300, 130, 200, 1.6, 300, 0, 5 * LED_HEIGHT, 10);
MovingDots movingDots1(-0.3, 0.3, 0.005, 1200, 240, 270, 1.3, 300, 230, 250, 1.6, 300, 3 * LED_HEIGHT, NUM_LEDS, 10);
MovingDots movingDots2(-0.3, 0.3, 0.005, 1200, 100, 20000, 1.3, 300, 130, 200, 1.6, 300, 0, NUM_LEDS, 10);
MovingDots movingDots3(-1.0, -0.5, 0.005, 200, 20, 40000, 1.3, 300, 20, 20, 1.6, 300, 0, NUM_LEDS, 10);

Blur blur(128, false);

MakeAll makeAll(1, 1, 1, false);

Blinky blinky(255, 255, 255, true, 10);

FadeToBlack fadeToBlack(255, true);

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
  setupWifi();
  
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  setupOTA();

  setupSerialOTA();

  // setupMQTT(HOSTNAME, MQTT_SERVER);
  MQTTclient.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  String lastWillTopic = String(HOSTNAME) + "/lastwill";
  static char lastWillTopicChar[65];
  lastWillTopic.toCharArray(lastWillTopicChar, lastWillTopic.length() + 1);
  SerialOTA.println(lastWillTopicChar);
  Serial.println(lastWillTopicChar);
  MQTTclient.enableLastWillMessage(lastWillTopicChar, "What a world, what a world!");  // For some reason this line prevents MQTT connection altogether
  // MQTTsubscriptions();     // we call this elsewhere

  FastLED.addLeds<NEOPIXEL, PIN_0>(leds, 0*LED_HEIGHT, LED_HEIGHT);
  
  #if LED_WIDTH > 1
  FastLED.addLeds<NEOPIXEL, PIN_1>(leds, 1*LED_HEIGHT, LED_HEIGHT);
  #endif
  #if LED_WIDTH > 2
  FastLED.addLeds<NEOPIXEL, PIN_2>(leds, 2*LED_HEIGHT, LED_HEIGHT);
  #endif
  #if LED_WIDTH > 3
  FastLED.addLeds<NEOPIXEL, PIN_3>(leds, 3*LED_HEIGHT, LED_HEIGHT);
  #endif
  #if LED_WIDTH > 4
  FastLED.addLeds<NEOPIXEL, PIN_4>(leds, 4*LED_HEIGHT, LED_HEIGHT);
  #endif
  #if LED_WIDTH > 5
  FastLED.addLeds<NEOPIXEL, PIN_5>(leds, 5*LED_HEIGHT, LED_HEIGHT);
  #endif
  #if LED_WIDTH > 6
  FastLED.addLeds<NEOPIXEL, PIN_6>(leds, 6*LED_HEIGHT, LED_HEIGHT);
  #endif
  #if LED_WIDTH > 7
  FastLED.addLeds<NEOPIXEL, PIN_7>(leds, 7*LED_HEIGHT, LED_HEIGHT);
  #endif
  
  randomSeed(esp_random());
  set_max_power_in_volts_and_milliamps(5, maxCurrent);   // in my current setup the maximum current is 50A
}

void loop()
{
  randomSeed(esp_random());
  
  reconnectToWifiIfNecessary();
  SerialOTAhandle();
  ArduinoOTA.handle();    // This is so that the MQTT or OTA doesn't go crazy and try to reconnect to a server when there is no WiFi
  MQTTclient.loop();
  

  fadeToBlack.doYourThing();

  // The last ones are additive so it doesn't matter what order you put them
  makeAll.doYourThing();
  blinky.doYourThing();
  movingDots.doYourThing();
  movingDots1.doYourThing();
  // movingDots2.doYourThing();
  movingDots3.doYourThing();
  blur.doYourThing();
  
  // this is here so that we don't call Fastled.show() too fast. things froze if we did that
  // perhaps I should use microseconds here. I could shave off a couple of milliseconds
  // unsigned long expectedTime = LED_HEIGHT * 24 * 11 / (800 * 10) + 2;     // 1 ms for the reset pulse and (takes 50 us. better safe than sorry) 1 ms rounding 11/10 added 10 % extra just to be on the safe side
  static unsigned long expectedTime = LED_HEIGHT * 24 * 11 / 8 + 500;     // 500 us for the reset pulse and (takes 50 us. better safe than sorry) also added 10 % extra just to be on the safe side
  
  static unsigned long oldMicros = 0;
  unsigned long frameTime = micros() - oldMicros;
  if (frameTime < expectedTime) delayMicroseconds(expectedTime - frameTime);
  oldMicros = micros();
  FastLED.show();
}
