#include <Arduino.h>
#include "AdiWiFiManager.h"

AdiWiFiManager WiFiManager;

void wifiDebugLog(const char *message) {
  Serial.println(message);
}

void setup() {

  Serial.begin(115200);

  if(!SD.begin()) {
    Serial.println("Card Mount Failed");
  }

  WiFiManager.setDebugCallback(wifiDebugLog);

  WiFiManager.setAP_ssid_pass("ESP32", "12345678");

  WiFiManager.setHostname("ESP32");

  //If true, Webserver stays active after wifi connection is established
  WiFiManager.WB_StaysActive(true);

  WiFiManager.connectToWiFi(true, "", "");

  //WiFiManager.StartWebserver(); Don't need this since webserver will already be active
  
  if(WiFiManager.getWiFiStatus() == WL_CONNECTED)
  {
    Serial.println("Connected to WiFi");
  }
}

void loop() {
  WiFiManager.loop();
}