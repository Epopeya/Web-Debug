#include "secrets.h"
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <stdint.h>
#include "vector.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

#define HEARTBEAT 50
#define MAX_MSGS 5
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

typedef enum {
  Message,
  TargetDirection,
  CurrentDirection,
  Battery,
  Position,
  Route,
  Lidar,
  MapFlip
} PacketHeader;

struct packet_state {
  char *messages[MAX_MSGS];
  int messages_length;
  float *target_direction;
  float *current_direction;
  float *battery;
  vector2_t *position;
  vector2_t *route;
  int route_length;
  vector2_t *lidar;
  bool *map_flip;
};

struct packet_state state = { .messages = { NULL }, .messages_length = 0, .target_direction = NULL, .current_direction = NULL, .battery = NULL, .position = NULL, .route = NULL, .route_length = 0, .lidar = NULL, .map_flip = NULL };

// This parser doesn't account for endianness!
void recv_serial_packet() {
  if (hs.available() > 0) {
    int header = hs.read();
    switch (header) {
      case Message:
        {
          uint8_t len = 0;
          hs.readBytes(&len, 1);
          char *message = (char *)malloc(len + 1);
          message[len] = '\0';
          hs.readBytes(message, len);
          if (state.messages_length < MAX_MSGS) {
            state.messages[state.messages_length] = message;
            state.messages_length++;
          } else {
            free(message);
          }
          break;
        }
      case TargetDirection:
        {
          float *dir = (float *)realloc(state.target_direction, sizeof(float));
          hs.readBytes((uint8_t *)dir, sizeof(float));
          state.target_direction = dir;
          break;
        }
      case CurrentDirection:
        {
          float *dir = (float *)realloc(state.current_direction, sizeof(float));
          hs.readBytes((uint8_t *)dir, sizeof(float));
          state.current_direction = dir;
          break;
        }
      case Battery:
        {
          float *bat = (float *)realloc(state.battery, sizeof(float));
          hs.readBytes((uint8_t *)bat, sizeof(float));
          state.battery = bat;
          break;
        }
      case Position:
        {
          vector2_t *pos = (vector2_t *)realloc(state.position, sizeof(vector2_t));
          hs.readBytes((uint8_t *)&pos->x, sizeof(float));
          hs.readBytes((uint8_t *)&pos->y, sizeof(float));
          state.position = pos;
          break;
        }
      case Route:
        {
          uint8_t len = 0;
          hs.readBytes(&len, 1);
          vector2_t *route = (vector2_t *)realloc(state.route, sizeof(vector2_t) * len);
          for (int i = 0; i < len; i++) {
            hs.readBytes((uint8_t *)&route[i].x, sizeof(float));
            hs.readBytes((uint8_t *)&route[i].y, sizeof(float));
          }
          state.route = route;
          state.route_length = len;
          break;
        }
      case Lidar:
        {
          vector2_t *lidar = (vector2_t *)realloc(state.lidar, sizeof(vector2_t));
          hs.readBytes((uint8_t *)&lidar->x, sizeof(float));
          hs.readBytes((uint8_t *)&lidar->y, sizeof(float));
          state.lidar = lidar;
          break;
        }
      case MapFlip:
        {
          bool *flipped = (bool *)realloc(state.map_flip, sizeof(bool));
          hs.readBytes((uint8_t *)flipped, sizeof(bool));
          state.map_flip = flipped;
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
    doc["trot"] = *state.target_direction;
    free(state.target_direction);
    state.target_direction = NULL;
  }
  // CurrentDirection
  if (state.current_direction != NULL) {
    doc["rot"] = *state.current_direction;
    free(state.current_direction);
    state.current_direction = NULL;
  }
  // Battery
  if (state.battery != NULL) {
    doc["bat"] = *state.battery;
    free(state.battery);
    state.battery = NULL;
  }
  // Position
  if (state.position != NULL) {
    doc["pos"][0] = state.position->x;
    doc["pos"][1] = state.position->y;
    free(state.position);
    state.position = NULL;
  }
  // Route
  if (state.route != NULL) {
    for (int i = 0; i < state.route_length; i++) {
      doc["route"][i][0] = state.route[i].x;
      doc["route"][i][1] = state.route[i].y;
    }
    free(state.route);
    state.route = NULL;
    state.route_length = 0;
  }
  // Lidar
  if (state.lidar != NULL) {
    doc["lidar"][0] = state.lidar->x;
    doc["lidar"][1] = state.lidar->y;
    free(state.lidar);
    state.lidar = NULL;
  }
  // Flip
  if (state.map_flip != NULL) {
    doc["flip"] = *state.map_flip;
    free(state.map_flip);
    state.map_flip = NULL;
  }

  if (!doc.isNull()) {
    String output = doc.as<String>();
    websocket.textAll(output);
  }

  // Free messages afterwards as they don't get deep copied
  for (int i = 0; i < state.messages_length; i++) {
    free(state.messages[i]);
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

  LittleFS.begin();
  webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", LittleFS.open("/index.html").readString());
  });
  webserver.on("/index.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/javascript", LittleFS.open("/index.js").readString());
  });
  webserver.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/css", LittleFS.open("/style.css").readString());
  });
  webserver.on("/map.svg", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "image/svg+xml", LittleFS.open("/map.svg").readString());
    response->addHeader("Cache-Control", "max-age=86400");
    request->send(response);
  });

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
