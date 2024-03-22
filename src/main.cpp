#include "secrets.h"
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <stdint.h>
#include "vector.h"
#include <ArduinoJson.h>

#define HEARTBEAT 100
unsigned long last_heartbeat = 0;

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

typedef enum { Message,
               TargetDirection,
               CurrentDirection,
               Servo } PacketHeader;

struct packet_state {
  const char *messages[1000];
  int messages_length;
  float *target_direction;
  float *current_direction;
  vector2_t *position;
};

struct packet_state state = { .messages = { NULL }, .messages_length = 0, .target_direction = NULL, .current_direction = NULL, .position = NULL };

// This parser doesn't account for endianness!
void recv_serial_packet() {
  if (hs.available() > 0) {
    // read() is non-blocking, may cause problems later!
    int header = hs.read();
    switch (header) {
      case Message:
        {
          int len = hs.read();
          char *message = (char *)malloc(len);
          hs.readBytes(message, len);
          state.messages[state.messages_length] = message;
          state.messages_length++;
          break;
        }
      case TargetDirection:
        {
          float *dir = (float *)malloc(sizeof(float));
          hs.readBytes((uint8_t *)dir, sizeof(float));
          state.target_direction = dir;
          break;
        }
      case CurrentDirection:
        {
          float *dir = (float *)malloc(sizeof(float));
          hs.readBytes((uint8_t *)dir, sizeof(float));
          state.current_direction = dir;
          break;
        }
    }
  }
}

void send_ws_packet() {
  JsonDocument doc;
  // Message
  for (int i = 0; i < state.messages_length; i++) {
    doc["msgs"][i] = state.messages[i];
  }
  // TargetDirection
  if (state.target_direction != NULL) {
    doc["target"] = *state.target_direction;
    free(state.target_direction);
    state.target_direction = NULL;
  }
  // CurrentDirection
  if (state.current_direction != NULL) {
    doc["dir"] = *state.current_direction;
    free(state.current_direction);
    state.current_direction = NULL;
  }
  String output = doc.as<String>();
  websocket.textAll(output);

  // Free messages afterwards as they don't get deep copied
  for (int i = 0; i < state.messages_length; i++) {
    free((void *)state.messages[i]);  // Shouldn't have to cast here but won't compile otherwise
    state.messages[i] = NULL;
  }
  state.messages_length = 0;
}

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

void loop() {
  recv_serial_packet();
  unsigned long time = millis();
  if ((time - last_heartbeat) > HEARTBEAT) {
    last_heartbeat = time;
    send_ws_packet();
  }
}
