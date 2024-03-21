#include "secrets.h"
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <cstdio>
#include <cstring>
#include <stdint.h>

AsyncWebServer webserver(80);
AsyncWebSocket websocket("/ws");

HardwareSerial hs(1);

void wsEventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client,
                    AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(),
                  client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

typedef enum { Message, TargetDirection, CurrentDirection, Servo } DebugHeader;

#define PACKET_MSG_HEADER "{\"msg\": \""
#define PACKET_MSG_FOOTER "\"}"

void packet_msg() {
  Serial.println("Packet receveived: Message");
  uint8_t msg_len = 0;
  hs.readBytes(&msg_len, 1);
  uint8_t msg[128 + strlen(PACKET_MSG_HEADER) + strlen(PACKET_MSG_FOOTER)] = {
      0};
  memcpy(msg, PACKET_MSG_HEADER, strlen(PACKET_MSG_HEADER));
  hs.readBytes(msg + strlen(PACKET_MSG_HEADER), (size_t)msg_len);
  memcpy(msg + strlen(PACKET_MSG_HEADER) + msg_len, PACKET_MSG_FOOTER,
         strlen(PACKET_MSG_FOOTER));
  websocket.textAll(msg, msg_len + strlen(PACKET_MSG_HEADER) +
                             strlen(PACKET_MSG_FOOTER));
}

void packet_target_direction() {
  Serial.println("Packet receveived: Message");
  uint8_t msg_len = 0;
  hs.readBytes(&msg_len, 1);
  uint8_t msg[128 + strlen(PACKET_MSG_HEADER) + strlen(PACKET_MSG_FOOTER)] = {
      0};
  memcpy(msg, PACKET_MSG_HEADER, strlen(PACKET_MSG_HEADER));
  hs.readBytes(msg + strlen(PACKET_MSG_HEADER), (size_t)msg_len);
  memcpy(msg + strlen(PACKET_MSG_HEADER) + msg_len, PACKET_MSG_FOOTER,
         strlen(PACKET_MSG_FOOTER));
  websocket.textAll(msg, msg_len + strlen(PACKET_MSG_HEADER) +
                             strlen(PACKET_MSG_FOOTER));
}

bool recv_packet() {
  if (hs.available() > 0) {
    uint8_t header = -1;
    hs.readBytes(&header, 1);
    switch (header) {
    case Message:
      packet_msg();
      return true;
    case TargetDirection:
      packet_target_direction();
      return true;
    }
  }
  return false;
}

void packet_targetdir();
void packet_curdir();
void packet_servo();

void setup() {
  Serial.begin(115200);
  hs.begin(115200, SERIAL_8N1, 5, 6);

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());

  // Web server
  websocket.onEvent(wsEventHandler);
  webserver.addHandler(&websocket);
  webserver.begin();
}

void loop() { recv_packet(); }
