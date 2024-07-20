#include <ESP8266WiFi.h>
#include <Redis.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "secrets.h"
#include "redis.h"
#include "mqtt.h"
#include "wifi.h"
#include "OTAupdate.h"
// Globals
#define POWER_PIN_RAIN D5  // The ESP8266 pin that provides the power to the rain sensor
#define POWER_PIN_TEMP D3  // The ESP8266 pin that provides the power to the BME sensor
#define AO_PIN    A0  // The ESP8266 pin connected to AO pin of the rain sensor
const int deviceId = ESP.getChipId();
Adafruit_BME280 bme = Adafruit_BME280();
// Redis prefix definitions
const String LOCATION_STRING = "location_";
const String TOPIC_STRING = "topic_";
const String MAX_STRING = "maxrain_";
const String MIN_STRING = "minrain_";

void setup() {
  Serial.begin(115200);
  delay(1);
}

void loop() {
  bme.begin(0x76);  
  delay(1);
  pinMode(POWER_PIN_RAIN, OUTPUT);
  pinMode(POWER_PIN_TEMP, OUTPUT);
  digitalWrite(POWER_PIN_TEMP, HIGH);
  digitalWrite(POWER_PIN_RAIN, HIGH);
  delay(10);
  int rain = analogRead(AO_PIN);
  delay(1);
  digitalWrite(POWER_PIN_RAIN, LOW);
  // connect WiFi while we wait for DHT sensor to stabilize
  connectWiFi();
  initRedis();
  initOTA();
  // BME sensor should be done. Read data and shut it down
	float fh = bme.readHumidity();
	float ft = bme.readTemperature();
  float fp = bme.readPressure() / 100.0F;
  int h = round(fh);
  int t = round(ft);
  int p = round(fp);
  delay(1);
  digitalWrite(POWER_PIN_TEMP, LOW);
  delay(1);
  String topic = getRedisValue(TOPIC_STRING);
  String location = getRedisValue(LOCATION_STRING);
  String maxRain = getRedisValue(MAX_STRING);
  String minRain = getRedisValue(MIN_STRING);
  int percentageRain = map(rain, maxRain.toInt(), minRain.toInt(), 100, 0);
  // build JSON doc for MQTT sending
  DynamicJsonDocument doc(1024);
  doc["deviceId"] = deviceId;
  doc["siteId"] = location;
  doc["temperature"] = t;
  doc["humidity"] = h;
  doc["pressure"] = p;
  doc["rain"] = percentageRain;
  doc["calibration_rain"] = rain;
  doc["ms"] = ESP.getCycleCount() / 160000;
  char MQTT_message[256];
  serializeJson(doc, MQTT_message);
  initMQTT();
  publishMessage(topic.c_str(), MQTT_message, true);
  delay(10);
  WiFi.disconnect( true );
  delay(1);
  ESP.deepSleepInstant(300e6, WAKE_RF_DISABLED);
  delay(10);
}
