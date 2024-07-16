WiFiClient MQTTWiFi;
PubSubClient pubsub(MQTT_SERVER, MQTT_PORT, MQTTWiFi);
void initMQTT () {
  String clientId = "ESP8266Client-";
  clientId.concat(ESP.getChipId());
  if (!pubsub.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWD)) {
    // If MQTT connection fails, go back to Deep Sleep and try again in 1 minute
    Serial.println("MQTT problem. Sleeping for 60s before retry");
    delay(1);
    ESP.deepSleepInstant(60e6, WAKE_RF_DISABLED);
    return;
  }
}

void publishMessage(const char* topic, String payload , boolean retained){
  if (pubsub.publish(topic, payload.c_str(), true))
      Serial.println("Message publised ["+String(topic)+"]: "+payload);
      delay(1);
}
