#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <esp_now.h>

/* ================= VERSION ================= */

#define GW_VERSION "3.0.0"
#define API_VERSION "1.0"

/* ================= WIFI ================= */

const char* ssid = "COMPTEURS";
const char* password = "12345678";

WebServer server(80);

/* ================= CONFIG ================= */

#define MAX_NODES 10
#define NODE_TIMEOUT 10000   // 10 secondes

/* ================= STRUCTURES ================= */

typedef struct {
  uint8_t id;
  uint32_t compteur;
  char version[8];
} NodePacket;

struct NodeInfo {
  bool active;
  uint8_t id;
  uint32_t compteur;
  String version;
  int rssi;
  unsigned long lastSeen;
};

NodeInfo nodes[MAX_NODES];

/* ================= UTILS ================= */

int findNode(uint8_t id) {
  for (int i = 0; i < MAX_NODES; i++) {
    if (nodes[i].active && nodes[i].id == id)
      return i;
  }
  return -1;
}

int addNode(uint8_t id) {
  for (int i = 0; i < MAX_NODES; i++) {
    if (!nodes[i].active) {
      nodes[i].active = true;
      nodes[i].id = id;
      return i;
    }
  }
  return -1;
}

/* ================= ESP-NOW CALLBACK ================= */

void OnDataRecv(const esp_now_recv_info *info,
                const uint8_t *incomingData,
                int len) {

  NodePacket packet;
  memcpy(&packet, incomingData, sizeof(packet));

  int index = findNode(packet.id);
  if (index == -1) {
    index = addNode(packet.id);
  }

  if (index != -1) {
    nodes[index].compteur = packet.compteur;
    nodes[index].version = String(packet.version);
    nodes[index].rssi = info->rx_ctrl->rssi;
    nodes[index].lastSeen = millis();
  }
}

/* ================= API ================= */

void handleAPI() {

  String json = "{";
  json += "\"api\":\"" + String(API_VERSION) + "\",";
  json += "\"gateway\":\"" + String(GW_VERSION) + "\",";
  json += "\"nodes\":[";

  bool first = true;

  for (int i = 0; i < MAX_NODES; i++) {

    if (nodes[i].active) {

      if (!first) json += ",";
      first = false;

      bool online = (millis() - nodes[i].lastSeen < NODE_TIMEOUT);

      json += "{";
      json += "\"id\":" + String(nodes[i].id) + ",";
      json += "\"compteur\":" + String(nodes[i].compteur) + ",";
      json += "\"version\":\"" + nodes[i].version + "\",";
      json += "\"rssi\":" + String(nodes[i].rssi) + ",";
      json += "\"online\":" + String(online ? "true" : "false");
      json += "}";
    }
  }

  json += "]";
  json += "}";

  server.send(200, "application/json", json);
}

/* ================= SETUP ================= */

void setup() {

  Serial.begin(115200);

  SPIFFS.begin(true);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);

  Serial.print("IP AP : ");
  Serial.println(WiFi.softAPIP());

  if (esp_now_init() != ESP_OK) {
    Serial.println("Erreur ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  server.on("/api", handleAPI);

  server.serveStatic("/", SPIFFS, "/index.html");
  server.serveStatic("/index.html", SPIFFS, "/index.html");
  server.serveStatic("/manifest.json", SPIFFS, "/manifest.json");
  server.serveStatic("/sw.js", SPIFFS, "/sw.js");
  server.serveStatic("/icon-192.png", SPIFFS, "/icon-192.png");
  server.serveStatic("/icon-512.png", SPIFFS, "/icon-512.png");

  server.begin();

  Serial.println("Gateway terrain robuste prête.");
}

/* ================= LOOP ================= */

void loop() {
  server.handleClient();
}
