void checkCalibration (int sensorValue, String CALIBRATION_STRING, String MAX_STRING, String MIN_STRING) {
  String calibration = getRedisValue(CALIBRATION_STRING);
  if (calibration == "") {
    // First time for sensor. Signup for initial calibration
    // Make sure we wait for sensor being placed in soil
    setRedisValue(CALIBRATION_STRING, "new_sensor");
    ESP.deepSleepInstant(60e6, WAKE_RF_DISABLED);
  }
  else if (calibration == "new_sensor") {
    // Make sure we wait for sensor being placed in soil
    ESP.deepSleepInstant(60e6, WAKE_RF_DISABLED);
  }
  else if (calibration == "initiate_sensor") {
    // Sensor got placed. Proceed with initial reading
    // Setting narrow corridor for initial calibration
    // Automatic calibration taking over after 100+ datapoints
    setRedisValue(MAX_STRING, String(sensorValue - 20));
    setRedisValue(MIN_STRING, String(sensorValue + 20));
    setRedisValue(CALIBRATION_STRING, "calibration_ongoing");
    delay(1);
  }
}