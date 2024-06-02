/**
 * esp32-motion
 *
 * Detects motion and switches a Shelly on.
 */
#include "config.hpp"

#include <Arduino.h>

#include <WiFi.h>

#include <HTTPClient.h>

#include <ArduinoJson.h>


unsigned long nextCheck;
unsigned long nextEnable;

void setup() {
  pinMode(Config::movementLed, OUTPUT);
  digitalWrite(Config::movementLed, true);

  Serial.begin(115200);
  Serial.printf("Connecting to %s", Config::SSID);

  WiFi.begin(Config::SSID, Config::password);
  bool flash = false;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
    digitalWrite(Config::movementLed, flash);
    flash = !flash;
  }

  Serial.print("\nWiFi Connected: ");
  Serial.println(WiFi.localIP());
  digitalWrite(Config::movementLed, false);
  delay(100);
  digitalWrite(Config::movementLed, true);
  delay(100);
  digitalWrite(Config::movementLed, false);
  delay(1000);

  pinMode(Config::movementInput, INPUT);

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
  if ((WiFi.status() != WL_CONNECTED)) {
    return false;
  }

  HTTPClient http;

  char url[100];
  sprintf(url, "http://%s/%s", Config::hostname, endpoint);

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
  bool movement = digitalRead(Config::movementInput);
  digitalWrite(Config::movementLed, movement ? HIGH : LOW);
  Serial.printf("Movement: %s\n", movement ? "yes" : "no");

  unsigned long currentTime = millis();
  if (movement && currentTime > nextCheck) {
    JsonDocument status;
    char endpoint[100];
    sprintf(endpoint, "rpc/Switch.GetStatus?id=%d", Config::switchId);
    if (queryShelly(status, endpoint)) {
      bool switchedOn = status["output"];
      bool switchedOnTimer = status["timer_started_at"] > 0;

      unsigned long timeout = Config::checkTimeout.milliseconds();
      if (!switchedOn || switchedOnTimer) {
        if (setSwitch(Config::switchId, true, Config::toggleAfter.seconds()) != SwitchResult::Error) {
          timeout = Config::enableTimeout.milliseconds();
        }
      }
      nextCheck = millis() + timeout;
    }
  }

  delay(100);
}
