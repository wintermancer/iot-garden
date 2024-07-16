#include <ESP8266WiFi.h>
#include <Redis.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include "secrets.h"
#include "redis.h"
#include "mqtt.h"
#include "wifi.h"
#include "OTAupdate.h"

// Globals
#define POWER_PIN D5
#define AO_PIN A0
const int deviceId = ESP.getChipId();
// Redis prefix definitions
const String LOCATION_STRING = "location_";
const String TOPIC_STRING = "topic_";
const String MAX_STRING = "maxmoist_";
const String MIN_STRING = "minmoist_";

void setup() {
  Serial.begin(115200);
  delay(1);
}

void loop() {
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);
  // connect WiFi while we wait for sensor to stabilize
  connectWiFi();  
  int sensorValue = analogRead(AO_PIN);
  digitalWrite(POWER_PIN, LOW);
  delay(1);
  initRedis();
  initOTA();
  String topic = getRedisValue(TOPIC_STRING);
  String location = getRedisValue(LOCATION_STRING);
  String maxMoist = getRedisValue(MAX_STRING);
  String minMoist = getRedisValue(MIN_STRING);
  int percentageSoilMoisture = map(sensorValue, maxMoist.toInt(), minMoist.toInt(), 100, 0);
  // build JSON doc for MQTT sending
  DynamicJsonDocument doc(1024);
  doc["deviceId"] = deviceId;
  doc["siteId"] = location;
  doc["moisture"] = percentageSoilMoisture;
  doc["calibration_moisture"] = sensorValue;
  doc["ms"] = ESP.getCycleCount() / 160000;
  char MQTT_message[128];
  serializeJson(doc, MQTT_message);
  initMQTT();
  publishMessage(topic.c_str(), MQTT_message, true);
  delay(10);
  WiFi.disconnect( true );
  delay(1);
  ESP.deepSleepInstant(2700e6, WAKE_RF_DISABLED);
  delay(10);
}