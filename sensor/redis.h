WiFiClient RedisWiFi;
Redis redis(RedisWiFi);
void initRedis () {
  if (!RedisWiFi.connect(REDIS_ADDR, REDIS_PORT)) {
    // If Redis connection fails, go back to Deep Sleep and try again in 1 minute
    Serial.println("Redis problem. Sleeping for 60s before retry");
    delay(1);
    ESP.deepSleepInstant(60e6, WAKE_RF_DISABLED);
    return;
  }
  auto connRet = redis.authenticate(REDIS_PASSWORD);
  if (connRet != RedisSuccess) {
  Serial.println("Redis auth failed. Sleeping for 60s before retry");
  ESP.deepSleepInstant(60e6, WAKE_RF_DISABLED);
  }
}

String getRedisValue (String redisKey) {
  redisKey.concat(ESP.getChipId());
  String result = redis.get(redisKey.c_str());
  return result;
}

void setRedisValue (String redisKey, String redisValue) {
  redisKey.concat(ESP.getChipId());
  redis.set(redisKey.c_str(), redisValue.c_str());
  return;
}
