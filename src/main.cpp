#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <vector>

/* ----------- constants ------------------------------------------- */
// hotspot‑config
const char*  AP_SSID       = "ESP32_Hotspot";
const char*  AP_PASSWORD   = "ESP32Wroom32";
const uint8_t AP_CHANNEL   = 6;
const IPAddress AP_IP      = IPAddress(192,168,4,1);
const IPAddress AP_GATEWAY = AP_IP;
const IPAddress AP_SUBNET  = IPAddress(255,255,255,0);

// WiFi config
const uint8_t wifiConnectTimeout = 30; // seconds

// LEDs
#define LED_ON  HIGH
#define LED_OFF LOW

#ifndef LED_BUILTIN
#define LED_BUILTIN 2   // GPIO2 for most ESP32‑boards
#endif

const uint8_t greenPin = 22;  // GPIO‑pin green LED
const uint8_t redPin   = 23;  // GPIO‑pin red LED

const uint16_t blinkIntervalMs = 500;

/* ----------- Structs ---------------------------------------------- */
struct Instance {       // Uptime-Kuma instance
  String id;            // UUID of instance
  String name;          // Instance name
  String endpoint;      // Uptime‑Kuma endpoint URL
  String apikey;        // API-Key
};

struct Config {
  String wifi_ssid;
  String wifi_pass;
  uint16_t pollInterval = 15;          // seconds
  std::vector<Instance> instances;
};

struct LedState {
  bool greenOn  = false;
  bool redBlink = false;
  uint32_t lastRedToggle = 0;     // millis since last blink toggle
  bool redOn    = false;
  bool builtinBlink = false;
  bool builtinOn    = false;
  uint32_t lastBuiltinToggle = 0; // millis since last blink toggle
};

/* ----------- Global variables ------------------------------------- */
Config cfg;
LedState ledState;
WebServer server(80);

/* ----------- function declarations -------------------------------- */
void startHotspot();
void setupWifi();
JsonDocument getWifiConfig();
JsonDocument setWifiConfig(String settingsJSON);
JsonDocument getInstancesConfig(JsonDocument doc);
JsonDocument setInstancesConfig(String settingsJSON);
bool writeConfig();
bool readConfig();
void setupLEDs();
void setLEDState(uint8_t pin, bool state);
void setupWebServer();

void ledTaskFunc(void * parameter);

/* ----------- setup ------------------------------------------------ */
void setup() {
  Serial.begin(115200);
  Serial.println();

  readConfig();

  setupLEDs();

  setupWifi();

  setupWebServer();

  // xTaskCreatePinnedToCore(pollTaskFunc, "PollTask", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(ledTaskFunc,  "LedTask",  2048, NULL, 1, NULL, 0);
}

/* ----------- loop ------------------------------------------------- */
void loop() {
  server.handleClient();
}

/* ----------- function definitions --------------------------------- */
// WiFi setup
void startHotspot() {
  Serial.print("[INFO] Hotspot config:   "); Serial.println(WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET) ? "Ready" : "Failed");
  WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, false, 6);
  IPAddress localIP = WiFi.softAPIP();
  Serial.print("[INFO] Hotspot IP:       "); Serial.println(localIP);
  Serial.print("[INFO] Hotspot SSID:     "); Serial.println(AP_SSID);
  Serial.print("[INFO] Hotspot Password: "); Serial.println(AP_PASSWORD);
  ledState.builtinBlink = true;
}

void setupWifi() {
  if (cfg.wifi_ssid == "" || cfg.wifi_pass == "") {
    Serial.println("[WARN] No WiFi credentials set, starting Hotspot");
    startHotspot();
    return;
  }

  int attempt = 0;
  WiFi.begin(cfg.wifi_ssid.c_str(), cfg.wifi_pass.c_str());
  Serial.print("[INFO] Connecting to WiFi network: " + cfg.wifi_ssid);
  while (WiFi.status() != WL_CONNECTED && attempt < wifiConnectTimeout * 2) {
    delay(500);
    Serial.print(".");
    attempt++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[ERROR] Could not connect to WIFI, starting Hotspot");
    startHotspot();
  } else {
    Serial.println("\n[INFO] WiFi connected, IP: " + WiFi.localIP().toString());
    ledState.builtinOn = true;
  }
}

JsonDocument deserializeInputJSON(String settingsJSON) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, settingsJSON);
  if (err) { Serial.print("[ERROR] ❌  JSON parse error: "); Serial.println(err.c_str()); return getWifiConfig(); }
  return doc;
}

JsonDocument getWifiConfig() {
  JsonDocument doc;
  doc["ssid"]      = cfg.wifi_ssid;
  doc["password"]  = cfg.wifi_pass;
  return doc;
}

JsonDocument setWifiConfig(String settingsJSON) {
  JsonDocument doc = deserializeInputJSON(settingsJSON);

  cfg.wifi_ssid     = doc["ssid"] | "";
  cfg.wifi_pass     = doc["password"] | "";

  Serial.println("[DEBUG] Wifi settings set: " + cfg.wifi_ssid + " / " + cfg.wifi_pass);

  return doc;
}

JsonDocument getInstancesConfig(JsonDocument doc) {
  JsonArray arr = doc["instances"].to<JsonArray>();
  for (const auto &inst : cfg.instances) {
    JsonObject obj = arr.add<JsonObject>();
    obj["id"]        = inst.id;
    obj["name"]      = inst.name;
    obj["endpoint"]  = inst.endpoint;
    obj["apikey"]    = inst.apikey;
  }
  return doc;
}

void setInstanceConfig(String settingsJSON) {
  JsonDocument doc = deserializeInputJSON(settingsJSON);
  cfg.instances.clear();
  JsonArray arr = doc["instances"].as<JsonArray>();
  for (JsonObject obj : arr) {
    Instance inst;
    inst.id          = obj["id"] | "";
    inst.name        = obj["name"] | "";
    inst.endpoint    = obj["endpoint"] | "";
    inst.apikey      = obj["apikey"] | "";
    cfg.instances.push_back(inst);
  }
}

// configuration read/write
bool writeConfig() {
  Serial.println("[INFO] Writing config to SPIFFS");
  JsonDocument doc = getWifiConfig();
  // Add the Uptime Kuma instances config
  doc = getInstancesConfig(doc);

  File f = SPIFFS.open("/config.json", "w");
  if (!f) { Serial.println("[ERROR] ❌  Cannot open config.json for writing"); return false; }
  serializeJson(doc, f);
  f.close();
  return true;
}

bool readConfig() {
  Serial.println("[INFO] Reading config from SPIFFS");
  if (!SPIFFS.begin(true)) { Serial.println("[ERROR] ❌  SPIFFS init failed!"); return false; }

  File f = SPIFFS.open("/config.json", "r");
  if (!f) { Serial.println("[WARN]⚠️  No config.json found – using defaults"); return false; }

  size_t size = f.size();
  std::unique_ptr<char[]> buf(new char[size]);
  f.readBytes(buf.get(), size);

  setWifiConfig(buf.get());
  setInstanceConfig(buf.get());

  return true;
}

// LED setup
void setupLEDs() {
  Serial.println("[INFO] Setting up LEDs");
  // initialize GPIO‑pins for green and red leds
  pinMode(greenPin, OUTPUT);
  pinMode(redPin,   OUTPUT);
  #ifdef LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT); // Wifi status
  #endif

  // Blink all LEDs to test functionality
  digitalWrite(greenPin, LED_ON);
  digitalWrite(redPin,   LED_ON);
  #ifdef LED_BUILTIN
  digitalWrite(LED_BUILTIN, LED_ON);
  #endif

  delay(1000);

  digitalWrite(greenPin, LED_OFF);
  digitalWrite(redPin,   LED_OFF);
  #ifdef LED_BUILTIN
  digitalWrite(LED_BUILTIN, LED_OFF);
  #endif
}

void setLEDState(uint8_t pin, bool state) {
  if (state) {
    digitalWrite(pin, LED_ON);
  } else {
    digitalWrite(pin, LED_OFF);
  }
}

// webserver setup
void setupWebServer() {
  /* 1. GET / – configuratieform  */
  server.on("/", HTTP_GET, []() {
    Serial.println("[INFO] GET / – serving config page");
    if (!SPIFFS.exists("/index.html")) {
      server.send(404, "text/plain", "File not found");
      Serial.println("[ERROR] GET / – config page not found on fs");
      return;
    }
    File indexFile = SPIFFS.open("/index.html", "r");
    server.streamFile(indexFile, "text/html", false);
    indexFile.close();
  });

  /* 2. GET /wifi - Return JSON object with wifi settings */
  server.on("/wifi", HTTP_GET, []() {
    Serial.println("[INFO] GET /wifi – returning wifi settings as JSON");
    JsonDocument doc = getWifiConfig();
    String wifiJSON;
    serializeJson(doc, wifiJSON);
    server.send(200, "application/json", wifiJSON);
  });

  /* 2. GET /instances - Return JSON array with all instances */
  server.on("/instances", HTTP_GET, []() {
    Serial.println("[INFO] GET /instances – returning Uptime Kuma instances as JSON");
    JsonDocument doc;
    doc = getInstancesConfig(doc);
    String instancesJSON;
    serializeJson(doc, instancesJSON);
    server.send(200, "application/json", instancesJSON);
  });

  /* 3. POST /wifi – Process incoming JSON array with instances */
  server.on("/wifi", HTTP_POST, []() {
    Serial.println("[INFO] POST /wifi – processing wifi settings");

    String body = server.arg("plain");
    Serial.println("[DEBUG] Body: " + body);
    setWifiConfig(body.c_str()); // Set to memory
    writeConfig(); // Write to SPIFFS

    server.send(200, "text/plain",
                "Config saved. ESP32 will reboot in 3s...");
    delay(3000);
    Serial.println("[INFO] Rebooting to apply new wifi configuration");
    ESP.restart();
  });

  /* 3. POST /instances – Process incoming JSON array with instances */
  server.on("/instances", HTTP_POST, []() {
    Serial.println("[INFO] POST /instances – processing instances");

    String body = server.arg("plain");
    Serial.println("[DEBUG] Body: " + body);
    setInstanceConfig(body.c_str()); // Set to memory
    writeConfig(); // Write to SPIFFS

    server.send(200, "text/plain",
                "Config saved.");
  });

  /* 3. POST /reboot – handmatig herstarten */
  server.on("/reboot", HTTP_POST, []() {
    Serial.println("[INFO] POST /reboot - rebooting");
    server.send(200, "text/plain", "Rebooting…");
    delay(2000);
    ESP.restart();
  });

  server.begin();
  Serial.println("[INFO] Web server started");
}

/* ----------- tasks (free‑RTOS) -------------------------------- */
void ledTaskFunc(void * parameter) {
  for (;;) {
    setLEDState(greenPin, ledState.greenOn);
    
    if (ledState.redBlink) {
      /* 500 ms interval for toggling */
      if (millis() - ledState.lastRedToggle >= blinkIntervalMs) {
        ledState.redOn = !ledState.redOn;
        ledState.lastRedToggle = millis();
      }
    }
    setLEDState(redPin, ledState.redOn);

    #ifdef LED_BUILTIN
    if (ledState.builtinBlink) {
      if (millis() - ledState.lastBuiltinToggle >= blinkIntervalMs) {
        ledState.builtinOn = !ledState.builtinOn;
        ledState.lastBuiltinToggle = millis();
      }
    }
    setLEDState(LED_BUILTIN, ledState.builtinOn);
    #endif

    delay(50);  // korte pauze om CPU‑belasting te verminderen
  }
}
