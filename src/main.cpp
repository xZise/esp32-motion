/**
 * BasicHTTPClient.ino
 *
 *  Created on: 24.05.2015
 *
 */
#include "secrets.h"

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <HTTPClient.h>

#include <ArduinoJson.h>


WiFiMulti wifiMulti;

constexpr int inPin = 23;
constexpr int onTime = 1000 * 10;

unsigned long nextSwitchOff;
bool on;


void setup() {

  Serial.begin(115200);
  Serial.printf("Connecting to %s", SSID);

  wifiMulti.addAP(SSID, password);

  pinMode(inPin, INPUT);

  nextSwitchOff = millis();
  on = false;
}

/*

http://HOSTNAME/rpc/Switch.GetStatus?id=0
http://HOSTNAME/rpc/Switch.Set?id=0&on=true

*/

enum class SwitchResult {
  Error,
  WasOn,
  WasOff
};

bool queryShelly(JsonDocument& doc, const char endpoint[]) {
  // wait for WiFi connection
  if ((wifiMulti.run() != WL_CONNECTED)) {
    return false;
  }

  HTTPClient http;

  char url[100];
  sprintf(url, "http://%s/%s", hostname, endpoint);

  Serial.print("[HTTP] begin...\n");
  // configure traged server and url
  http.begin(url);

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);

      deserializeJson(doc, payload);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();

  return httpCode == HTTP_CODE_OK;
}

SwitchResult setSwitch(uint8_t switchId, bool on) {
  char endpoint[100];
  sprintf(endpoint, "rpc/Switch.Set?id=%d&on=%s", switchId, on ? "true" : "false");

  JsonDocument doc;
  if (queryShelly(doc, endpoint)) {
    bool was_on = doc["was_on"];

    Serial.printf("Was on: %s\n", was_on ? "on" : "off");
    return was_on ? SwitchResult::WasOn : SwitchResult::WasOff;
  } else {
    return SwitchResult::Error;
  }
}

void loop() {
  bool movement = digitalRead(inPin);
  Serial.printf("Movement: %s\n", movement ? "yes" : "no");

  unsigned long currentTime = millis();
  if (movement) {
    nextSwitchOff = currentTime + onTime;
    if (!on) {
      Serial.println("Switched on");
      on = true;
      setSwitch(0, on);
    }
  }

  if (on && currentTime > nextSwitchOff) {
    Serial.println("Switched off");
    on = false;
    setSwitch(0, on);
  }

  delay(100);
}