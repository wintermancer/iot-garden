#include <ESP8266WiFi.h>
#include <Redis.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include "secrets.h"

#define POWER_PIN D5
#define AO_PIN A0

// Redis prefix definitions
#define LOCATION_STRING "location_"
#define TOPIC_STRING "topic_"
#define MAX_STRING "maxmoist_"
#define MIN_STRING "minmoist_"
#define OTA_VERSION "OTAV_"

const String SOFTWARE_VERSION = "20240707001";

// data structure for saving WiFi information
// based on https://www.bakke.online/index.php/2017/05/21/reducing-wifi-power-consumption-on-esp8266-part-1/
struct {
  uint32_t crc32;   // 4 bytes
  uint8_t channel;  // 1 byte,   5 in total
  uint8_t bssid[6]; // 6 bytes, 11 in total
  uint8_t padding;  // 1 byte,  12 in total
} rtcData;

/** define global variables **/
const int deviceId = ESP.getChipId();
String redisValue;

/**** WiFi Connectivity Initialisation *****/
WiFiClient WiFiclient;
WiFiClient WiFiclient2;
Redis redis(WiFiclient);
PubSubClient pubsub(WiFiclient2);

void setup() {
  Serial.begin(115200);
  delay(1);
}

uint32_t calculateCRC32( const uint8_t *data, size_t length ) {
  uint32_t crc = 0xffffffff;
  while( length-- ) {
    uint8_t c = *data++;
    for( uint32_t i = 0x80; i > 0; i >>= 1 ) {
      bool bit = crc & 0x80000000;
      if( c & i ) {
        bit = !bit;
      }

      crc <<= 1;
      if( bit ) {
        crc ^= 0x04c11db7;
      }
    }
  }

  return crc;
}

void connectWiFi() {
  // Try to read WiFi settings from RTC memory
  bool rtcValid = false;
  if( ESP.rtcUserMemoryRead( 0, (uint32_t*)&rtcData, sizeof( rtcData ) ) ) {
    // Calculate the CRC of what we just read from RTC memory, but skip the first 4 bytes as that's the checksum itself.
    uint32_t crc = calculateCRC32( ((uint8_t*)&rtcData) + 4, sizeof( rtcData ) - 4 );
    if( crc == rtcData.crc32 ) {
      rtcValid = true;
    }
  }
  // connect wifi
  Serial.print("connecting to: ");
  Serial.println(WLAN_SSID);
  // wake WiFi from sleep
  WiFi.setOutputPower(11.5);
  WiFi.persistent( false );
  WiFi.forceSleepWake();
  delay(1);
  WiFi.mode(WIFI_STA);
  if( rtcValid ) {
    // The RTC data was good, make a quick connection
    WiFi.begin( WLAN_SSID, WLAN_PASSWD, rtcData.channel, rtcData.bssid, true );
  }
  else {
    // The RTC data was not valid, so make a regular connection
    WiFi.begin( WLAN_SSID, WLAN_PASSWD );
  }
  int WLAN_retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    WLAN_retries++;
    if( WLAN_retries == 400) {
      // Quick connect is not working, reset WiFi and try regular connection
      WiFi.disconnect();
      delay(1);
      WiFi.forceSleepBegin();
      delay(1);
      WiFi.forceSleepWake();
      delay(1);
      WiFi.begin( WLAN_SSID, WLAN_PASSWD );
    }
    if( WLAN_retries == 1200 ) {
      // Giving up after 12 seconds and restart
      WiFi.disconnect( true );
      delay(1);
      WiFi.mode( WIFI_OFF );
      delay(1);
      // Serial.println("WiFi problem. Sleeping for 60s before retry");
      ESP.deepSleepInstant(60e6, WAKE_RF_DISABLED);
      return;
  }
    delay(10);
  }
  // Write current connection info back to RTC
  rtcData.channel = WiFi.channel();
  memcpy( rtcData.bssid, WiFi.BSSID(), 6 ); // Copy 6 bytes of BSSID (AP's MAC address)
  rtcData.crc32 = calculateCRC32( ((uint8_t*)&rtcData) + 4, sizeof( rtcData ) - 4 );
  ESP.rtcUserMemoryWrite( 0, (uint32_t*)&rtcData, sizeof( rtcData ) );

}

void initRedis () {
  if (!WiFiclient.connect(REDIS_ADDR, REDIS_PORT)) {
    // If Redis connection fails, go back to Deep Sleep and try again in 5 minutes
    Serial.println("Redis problem. Sleeping for 60s before retry");
    ESP.deepSleepInstant(60e6, WAKE_RF_DISABLED);
    return;
  }
}

String getRedisValue (String redisKey) {
  // get Redis Values
  auto connRet = redis.authenticate(REDIS_PASSWORD);
  if (connRet == RedisSuccess) {
    redisKey.concat(deviceId);
    String result = redis.get(redisKey.c_str());
    return result;
    }
  else {
    Serial.println("Redis read failed.");
    String result = "ReadError";
    return result;
  }
}

void setRedisValue (String redisKey, String redisValue) {
  // get Redis Values
  auto connRet = redis.authenticate(REDIS_PASSWORD);
  if (connRet == RedisSuccess) {
    redisKey.concat(deviceId);
    redis.set(redisKey.c_str(), redisValue.c_str());
    return;
    }
  else {
    // Serial.println("Redis write failed.");
    return;
  }
}

void initOTA () {
  String otaVersion = getRedisValue(OTA_VERSION);
  if ( SOFTWARE_VERSION != otaVersion ) {
    ESPhttpUpdate.setClientTimeout(2000);
    String updateURL = "/";
    updateURL.concat(deviceId);
    updateURL.concat(otaVersion);
    t_httpUpdate_return ret = ESPhttpUpdate.update(WiFiclient, OTA_SERVER, OTA_PORT, updateURL);
    switch (ret) {
      case HTTP_UPDATE_FAILED: Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str()); break;
      case HTTP_UPDATE_NO_UPDATES: Serial.println("HTTP_UPDATE_NO_UPDATES"); break;
      case HTTP_UPDATE_OK: Serial.println("HTTP_UPDATE_OK"); break;
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!pubsub.connected()) {
    // Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";   // Create client ID
    clientId.concat(deviceId);
    // Attempt to connect
    if (pubsub.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWD)) {
      // Serial.println("connected");
    } else {
    Serial.println("MQTT problem. Sleeping for 60s before retry");
    ESP.deepSleepInstant(60e6, WAKE_RF_DISABLED);
    return;
    }
  }
}

void publishMessage(const char* topic, String payload , boolean retained){
  if (pubsub.publish(topic, payload.c_str(), true))
      Serial.println("Message publised ["+String(topic)+"]: "+payload);
      delay(1);
}

void loop() {
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);
  connectWiFi(); // connect WiFi while we wait for sensor to stabilize 
  int sensorValue = analogRead(AO_PIN);
  digitalWrite(POWER_PIN, LOW);
  delay(1);
  initRedis();
  // check for OTA updates
  initOTA();
  // prepare payload for MQTT
  String topic = getRedisValue(TOPIC_STRING);
  String location = getRedisValue(LOCATION_STRING);
  String maxMoist = getRedisValue(MAX_STRING);
  String minMoist = getRedisValue(MIN_STRING);
  int percentageSoilMoisture = map(sensorValue, maxMoist.toInt(), minMoist.toInt(), 100, 0);
  DynamicJsonDocument doc(1024);
  doc["deviceId"] = deviceId;
  doc["siteId"] = location;
  doc["moisture"] = percentageSoilMoisture;
  doc["calibration_moisture"] = sensorValue;
  doc["ms"] = ESP.getCycleCount() / 160000;
  char MQTT_message[128];
  serializeJson(doc, MQTT_message);
  // MQTT is go
  pubsub.setServer(MQTT_SERVER, MQTT_PORT);
  if (!pubsub.connected()) reconnect();
  pubsub.loop();
  // publish message
  publishMessage(topic.c_str(), MQTT_message, true);
  delay(10);
  WiFi.disconnect( true );
  delay(1);
  ESP.deepSleepInstant(2700e6, WAKE_RF_DISABLED);
  delay(10);
}
