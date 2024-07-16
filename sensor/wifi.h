// data struct for saving WiFi information
struct {
  uint32_t crc32;   // 4 bytes
  uint8_t channel;  // 1 byte,   5 in total
  uint8_t bssid[6]; // 6 bytes, 11 in total
  uint8_t padding;  // 1 byte,  12 in total
} rtcData;

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
  Serial.print("connecting to: ");
  Serial.println(WLAN_SSID);
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
    // The RTC data was not valid, make a regular connection
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
      // Giving up after 12 seconds
      WiFi.disconnect( true );
      delay(1);
      WiFi.mode( WIFI_OFF );
      delay(1);
      ESP.deepSleepInstant(60e6, WAKE_RF_DISABLED);
      return;
  }
    delay(10);
  }
  // Write connection info to RTC
  rtcData.channel = WiFi.channel();
  memcpy( rtcData.bssid, WiFi.BSSID(), 6 ); // Copy 6 bytes of BSSID (AP's MAC address)
  rtcData.crc32 = calculateCRC32( ((uint8_t*)&rtcData) + 4, sizeof( rtcData ) - 4 );
  ESP.rtcUserMemoryWrite( 0, (uint32_t*)&rtcData, sizeof( rtcData ) );

}
