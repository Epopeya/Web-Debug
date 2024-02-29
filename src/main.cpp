#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "secrets.h"

void setup() {
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
}

void loop() {
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
}
