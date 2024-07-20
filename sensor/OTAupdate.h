const String OTA_VERSION = "OTAV_";
const String SOFTWARE_VERSION = "20240720001";
WiFiClient OTAWiFi;
void initOTA () {
  String otaVersion = getRedisValue(OTA_VERSION);
  if ( SOFTWARE_VERSION != otaVersion ) {
    ESPhttpUpdate.setClientTimeout(2000);
    String updateURL = "/";
    updateURL.concat(ESP.getChipId());
    updateURL.concat(otaVersion);
    t_httpUpdate_return ret = ESPhttpUpdate.update(OTAWiFi, OTA_SERVER, OTA_PORT, updateURL);
    switch (ret) {
      case HTTP_UPDATE_FAILED: Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str()); break;
      case HTTP_UPDATE_NO_UPDATES: Serial.println("HTTP_UPDATE_NO_UPDATES"); break;
      case HTTP_UPDATE_OK: Serial.println("HTTP_UPDATE_OK"); break;
    }
  }
}
