#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "credentials.h"
#include <Adafruit_NeoPixel.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define PIN D2
#define HOSTNAME "PergolaLighting"
#define ONE_WIRE_BUS D5
#define USER_MQTT_CLIENT_NAME "kolcun/outdoor/pergola/led"
#define MODE_WHITE 1
#define MODE_RGB 0
#define STRIP_ON 1
#define STRIP_OFF 0

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWD;

const char* clientId = "kolcun/outdoor/pergola/led";
const char* overwatchTopic = "kolcun/outdoor/pergola/led/overwatch";
const char* stateTopic = "kolcun/outdoor/pool/temperature/state";
const char onlineMessage[50] = "Pergola LED Online";

char charPayload[50];

int ledPowerState = STRIP_OFF;
int ledMode = MODE_WHITE;
int ledBrightness = 25;
int red = 0;
int green = 0;
int blue = 0;


WiFiClient wifiClient;
PubSubClient pubSubClient(wifiClient);

Adafruit_NeoPixel strip = Adafruit_NeoPixel(300, PIN, NEO_GRBW + NEO_KHZ800);

// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  pinMode(ONE_WIRE_BUS, INPUT);

  setupOTA();
  setupMqtt();
  setupStrip();

}

void loop() {

  ArduinoOTA.handle();
  //  readTemperature();

  if (!pubSubClient.connected()) {
    reconnect();
  }
  pubSubClient.loop();

  configureLedStripState();
  strip.show();

  //do things with LED strip

  //  theaterChase(strip.Color(127, 127, 127, 127), 50); // "White"
  //  theaterChase(strip.Color(255, 255, 255, 255), 50); // "White"
  //  theaterChase(strip.Color(0, 0, 0, 127), 50); // Warm White
  //  theaterChase(strip.Color(0, 0, 0, 255), 50); // Warm White
  //  theaterChase(strip.Color(127, 0, 0), 50); // Red
  //  theaterChase(strip.Color(0, 0, 127), 50); // Blue
}


void configureLedStripState() {
  if (ledPowerState == STRIP_OFF) {
    strip.setBrightness(0);
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0, 0));
    }
  } else if (ledPowerState == STRIP_ON) {
    strip.setBrightness(ledBrightness);
    if (ledMode == MODE_RGB) {
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(red, green, blue, 0));
      }
    } else if (ledMode == MODE_WHITE) {
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0, 255));
      }
      strip.setPixelColor(2, strip.Color(255, 0, 0, 0));
      strip.setPixelColor(3, strip.Color(255, 0, 0, 0));
      strip.setPixelColor(4, strip.Color(255, 0, 0, 0));
      strip.setPixelColor(5, strip.Color(255, 0, 0, 0));
      strip.setPixelColor(6, strip.Color(255, 0, 0, 0));
    }

  }

}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  String newTopic = topic;
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0';
  String newPayload = String((char *)payload);
  int intPayload = newPayload.toInt();
  Serial.println(newPayload);
  Serial.println();
  newPayload.toCharArray(charPayload, newPayload.length() + 1);

//  Serial.print("Topic: ");
//  Serial.println(newTopic);
//  Serial.print("Payload: ");
//  Serial.println(newPayload);
//  Serial.print("CharPayload: ");
//  Serial.println(charPayload);
//  Serial.print("IntPayload: " );
//  Serial.println(intPayload);

  if (newTopic == USER_MQTT_CLIENT_NAME"/power/set") {
    pubSubClient.publish(USER_MQTT_CLIENT_NAME"/power/state", charPayload);
    ledPowerState = intPayload;
  }
  if (newTopic == USER_MQTT_CLIENT_NAME"/mode/set") {
    pubSubClient.publish(USER_MQTT_CLIENT_NAME"/mode/state", charPayload);
    ledMode = intPayload;
  }
  if (newTopic == USER_MQTT_CLIENT_NAME"/brightness/set") {
    pubSubClient.publish(USER_MQTT_CLIENT_NAME"/brightness/state", charPayload);
    ledBrightness = intPayload;
  }

  if (newTopic == USER_MQTT_CLIENT_NAME"/colour/set") {
    pubSubClient.publish(USER_MQTT_CLIENT_NAME"/colour/state", charPayload);

    uint8_t firstIndex = newPayload.indexOf(',');
    uint8_t lastIndex = newPayload.lastIndexOf(',');

    uint8_t rgb_red = newPayload.substring(0, firstIndex).toInt();
    if (rgb_red < 0 || rgb_red > 255) {
      return;
    } else {
      red = rgb_red;
    }

    uint8_t rgb_green = newPayload.substring(firstIndex + 1, lastIndex).toInt();
    if (rgb_green < 0 || rgb_green > 255) {
      return;
    } else {
      green = rgb_green;
      
    }

    uint8_t rgb_blue = newPayload.substring(lastIndex + 1).toInt();
    if (rgb_blue < 0 || rgb_blue > 255) {
      return;
    } else {
      blue = rgb_blue;
    }
  }
}

void readTemperature() {
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  Serial.print(" Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  float currentTempC = sensors.getTempCByIndex(0);
  float currentTempF = sensors.getTempFByIndex(0);
  Serial.print("Temperature C is: ");
  Serial.println(currentTempC);
  Serial.print("Temperature F is: ");
  Serial.println(currentTempF);

  //  String tempC;
  //  tempC += currentTempC;
  //  String tempF;
  //  tempF += currentTempF;
  //  pubSubClient.publish(stateTopicC, (char *) tempC.c_str());
  //  pubSubClient.publish(stateTopicF, (char *) tempF.c_str());
  // You can have more than one IC on the same bus.
  // 0 refers to the first IC on the wire
  //  delay(1000);
}

void setupMqtt() {
  pubSubClient.setServer(MQTT_SERVER, 1883);
  pubSubClient.setCallback(mqttCallback);
  if (!pubSubClient.connected()) {
    reconnect();
  }
}

void setupStrip() {
  strip.setBrightness(ledBrightness);
  strip.begin();
  configureLedStripState();
  strip.show();

  pubSubClient.publish(USER_MQTT_CLIENT_NAME"/power/state", String(ledPowerState).c_str());
  pubSubClient.publish(USER_MQTT_CLIENT_NAME"/brightness/state", String(ledBrightness).c_str());
  pubSubClient.publish(USER_MQTT_CLIENT_NAME"/mode/state", String(ledMode).c_str());
  String colour = String(red) + "," + String(green) + "," + String(blue);
  pubSubClient.publish(USER_MQTT_CLIENT_NAME"/colour/state", String(colour).c_str());

}

void setupOTA() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Wifi Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.setHostname(HOSTNAME);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j = 0; j < 10; j++) { //do 10 cycles of chasing
    for (int q = 0; q < 3; q++) {
      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, c);  //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, 0);      //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j = 0; j < 256; j++) {   // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, Wheel( (i + j) % 255)); //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, 0);      //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void reconnect() {
  bool boot = true;
  int retries = 0;
  while (!pubSubClient.connected()) {
    if (retries < 10) {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (pubSubClient.connect(clientId, MQTT_USER, MQTT_PASSWD)) {
        Serial.println("connected");
        // Once connected, publish an announcement...
        if (boot == true) {
          pubSubClient.publish(overwatchTopic, "Rebooted");
          boot = false;
        } else {
          pubSubClient.publish(overwatchTopic, "Reconnected");
        }
        pubSubClient.subscribe(USER_MQTT_CLIENT_NAME"/power/set");
        pubSubClient.subscribe(USER_MQTT_CLIENT_NAME"/mode/set");
        pubSubClient.subscribe(USER_MQTT_CLIENT_NAME"/brightness/set");
        pubSubClient.subscribe(USER_MQTT_CLIENT_NAME"/colour/set");
      } else {
        Serial.print("failed, rc=");
        Serial.print(pubSubClient.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    else {
      ESP.restart();
    }
  }
}
