#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <vector>

/********************************************************************
 *  Webpage template – stored in flash (PROGMEM)
 ********************************************************************/
const char PROGMEM pageTemplate[] = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Uptime‑Kuma Monitor</title>
</head>
<body>
  <h1>Configure</h1>
  <form method="POST" action="/config">
    <!-- Wi‑Fi fields -->
    <label>SSID: <input type="text" name="ssid" value="%s" /></label><br/>
    <label>Password: <input type="password" name="pass" value="%s" /></label><br/>
    <label>Poll interval (s): <input type="number" name="interval" value="%d" /></label><br/><br/>

    <!-- Instance fields – generated in the handler loop -->
  
  <!-- Button to add a new instance (JavaScript) -->
  <button type="button" onclick="addInstance()">Add instance</button><br/><br/>

  <input type="submit" value="Save" /></form>
  <form method="POST" action="/reboot"><input type="submit" value="Reboot" /></form>

  <!-- JavaScript to add a new instance dynamically -->
  <script>
    function addInstance() {
      var form = document.querySelector('form');
      var i = form.querySelectorAll('h3').length;
      var h = document.createElement('h3');
      h.textContent = 'Instance ' + (i + 1);
      form.appendChild(h);

      var labels = ['Name','URL'];
      var names  = ['name','url'];
      for (var j = 0; j < labels.length; j++) {
        var label = document.createElement('label');
        label.textContent = labels[j] + ': ';
        var input = document.createElement('input');
        input.type = 'text';
        input.name = names[j] + i;
        label.appendChild(input);
        form.appendChild(label);
        form.appendChild(document.createElement('br'));
      }
    }
  </script>
</body>
</html>
)=====";

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

const uint8_t blinkIntervalMs = 500;

/* ----------- Structs ---------------------------------------------- */
struct Instance {       // Uptime-Kuma instance
  String name;          // Instance name
  String url;           // Uptime‑Kuma endpoint URL
  String apiKey;        // API-Key
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

// configuration read/write
bool writeConfig() {
  Serial.println("[INFO] Writing config to SPIFFS");
  StaticJsonDocument<1024> doc;
  doc["wifi_ssid"]     = cfg.wifi_ssid;
  doc["wifi_pass"]     = cfg.wifi_pass;
  doc["poll_interval"] = cfg.pollInterval;

  JsonArray arr = doc.createNestedArray("instances");
  for (const auto &inst : cfg.instances) {
    JsonObject obj = arr.createNestedObject();
    obj["name"]      = inst.name;
    obj["url"]       = inst.url;
  }

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

  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, buf.get());
  if (err) { Serial.print("[ERROR] ❌  JSON parse error: "); Serial.println(err.c_str()); return false; }

  cfg.wifi_ssid     = doc["wifi_ssid"] | "";
  cfg.wifi_pass     = doc["wifi_pass"] | "";
  cfg.pollInterval  = doc["poll_interval"] | 15;

  cfg.instances.clear();
  JsonArray arr = doc["instances"].as<JsonArray>();
  for (JsonObject obj : arr) {
    Instance inst;
    inst.name        = obj["name"] | "";
    inst.url         = obj["url"] | "";
    cfg.instances.push_back(inst);
  }

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
    // Temporary buffer for the final HTML (size depends on your use‑case)
    // 4 kB is usually enough for a small configuration page
    char html[4096];

    // Build the static part and insert Wi‑Fi values
    snprintf_P(html, sizeof(html), pageTemplate,
              cfg.wifi_ssid.c_str(),          // %s  – SSID
              cfg.wifi_pass.c_str(),          // %s  – password
              cfg.pollInterval);              // %d  – poll interval

    // Append instance blocks – we need a buffer that can hold one block
    char block[512];
    size_t used = strlen(html);              // current length of html

    for (size_t i = 0; i < cfg.instances.size(); ++i) {
      const auto &inst = cfg.instances[i];
      // %d is used for the instance number (i+1)
      snprintf(block, sizeof(block),
              "<h3>Instance %d</h3>\n<label>Name: <input type=\"text\" name=\"name%d\" value=\"%s\" /></label><br/>\n<label>URL: <input type=\"text\" name=\"url%d\" value=\"%s\" /></label><br/>",
              (int)i + 1, // instance header
              (int)i, inst.name.c_str(),
              (int)i, inst.url.c_str());

      // Ensure we don't overflow the final buffer
      if (used + strlen(block) < sizeof(html)) {
        strcpy(html + used, block);
        used += strlen(block);
      } else {
        // In a real project you should handle the overflow case
        break;
      }
    }

    // Finally send the page
    server.send(200, "text/html", html);
  });

  /* 2. POST /config – verwerking */
  server.on("/config", HTTP_POST, []() {
    Serial.println("[INFO] POST /config – processing form data");
    cfg.wifi_ssid   = server.arg("ssid");
    cfg.wifi_pass   = server.arg("pass");
    cfg.pollInterval = server.arg("interval").toInt();

    // Bepaal hoeveel instanties er zijn (aantal “nameX”‑velden)
    int instanceCount = 0;
    for (int i = 0; i < server.args(); ++i) {
      String key = server.argName(i);
      if (key.startsWith("name") && key.length() > 4) {
        int idx = key.substring(4).toInt();
        instanceCount = max(instanceCount, idx + 1);
      }
    }

    cfg.instances.clear();
    for (int i = 0; i < instanceCount; ++i) {
      Instance inst;
      inst.name       = server.arg("name"  + String(i));
      inst.url        = server.arg("url"   + String(i));
      cfg.instances.push_back(inst);
    }

    writeConfig();   // opslaan naar SPIFFS
    server.send(200, "text/plain",
                "Config saved. ESP32 will reboot in 3s…");
    delay(3000);
    ESP.restart();
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
