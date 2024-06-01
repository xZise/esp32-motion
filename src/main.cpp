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

constexpr int output = LED_MOVEMENT;
constexpr int onTimeSeconds = 10;
constexpr int inPin = 4;

// When it detects movements it checks every "checkTimeout" whether the light
// should be switched on. When the light is switched on, it'll wait for
// "enableTimeout" before it'll switch it on again. As long as "enableTimeout"
// is less than "onTimeSeconds", it shouldn't cause it to "skip an update".
constexpr unsigned long checkTimeout = 1 * 1000;
// This value prevents retriggering, while the motion sensor is in "hold time"
constexpr unsigned long enableTimeout = 5 * 1000;

unsigned long nextCheck;
unsigned long nextEnable;

void setup() {

  Serial.begin(115200);
  Serial.printf("Connecting to %s", Secrets::SSID);

  wifiMulti.addAP(Secrets::SSID, Secrets::password);

  pinMode(output, OUTPUT);
  pinMode(inPin, INPUT);

  nextCheck = millis();
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
  sprintf(url, "http://%s/%s", Secrets::hostname, endpoint);

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

SwitchResult setSwitch(uint8_t switchId, bool on, int toggle_after) {
  char endpoint[100];
  if (toggle_after > 0) {
    sprintf(endpoint, "rpc/Switch.Set?id=%d&on=%s&toggle_after=%d", switchId, on ? "true" : "false", toggle_after);
  } else {
    sprintf(endpoint, "rpc/Switch.Set?id=%d&on=%s", switchId, on ? "true" : "false");
  }

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
  digitalWrite(output, movement ? HIGH : LOW);
  Serial.printf("Movement: %s\n", movement ? "yes" : "no");

  unsigned long currentTime = millis();
  if (movement && currentTime > nextCheck) {
    JsonDocument status;
    if (queryShelly(status, "rpc/Switch.GetStatus?id=0")) {
      bool switchedOn = status["output"];
      bool switchedOnTimer = status["timer_started_at"] > 0;

      unsigned long timeout = checkTimeout;
      if (!switchedOn || switchedOnTimer) {
        if (setSwitch(0, true, onTimeSeconds) != SwitchResult::Error) {
          timeout = enableTimeout;
        }
      }
      nextCheck = millis() + timeout;
    }
  }

  delay(100);
}
