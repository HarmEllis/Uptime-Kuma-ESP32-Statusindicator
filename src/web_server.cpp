#include "web_server.h"
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

WebServerManager::WebServerManager(Config& cfg, ConfigManager& configManager)
  : _cfg(cfg), _configManager(configManager), _server(80) {}

void WebServerManager::setup() {
  setupRoutes();
  _server.begin();
  Serial.println("[INFO] Web server started");
}

void WebServerManager::handleClient() {
  _server.handleClient();
}

void WebServerManager::setupRoutes() {
  // GET / - configuratiepagina
  _server.on("/", HTTP_GET, [this]() {
    Serial.println("[INFO] GET / – serving config page");
    if (!SPIFFS.exists("/index.html")) {
      _server.send(404, "text/plain", "File not found");
      Serial.println("[ERROR] GET / – config page not found on fs");
      return;
    }
    File indexFile = SPIFFS.open("/index.html", "r");
    _server.streamFile(indexFile, "text/html", false);
    indexFile.close();
  });

  // GET /wifi - Return JSON object met wifi instellingen
  _server.on("/wifi", HTTP_GET, [this]() {
    Serial.println("[INFO] GET /wifi – returning wifi settings as JSON");
    JsonDocument doc = _configManager.getWifiConfig();
    String wifiJSON;
    serializeJson(doc, wifiJSON);
    _server.send(200, "application/json", wifiJSON);
  });

  // GET /instances - Return JSON array met alle instances
  _server.on("/instances", HTTP_GET, [this]() {
    Serial.println("[INFO] GET /instances – returning Uptime Kuma instances as JSON");
    JsonDocument doc;
    doc = _configManager.getInstancesConfig(doc);
    String instancesJSON;
    serializeJson(doc, instancesJSON);
    _server.send(200, "application/json", instancesJSON);
  });

  // POST /wifi - Verwerk wifi instellingen
  _server.on("/wifi", HTTP_POST, [this]() {
    Serial.println("[INFO] POST /wifi – processing wifi settings");

    String body = _server.arg("plain");
    Serial.println("[DEBUG] Body: " + body);
    _configManager.setWifiConfig(body.c_str());
    _configManager.writeConfig();

    _server.send(200, "text/plain", "Config saved. ESP32 will reboot in 3s...");
    delay(3000);
    Serial.println("[INFO] Rebooting to apply new wifi configuration");
    ESP.restart();
  });

  // POST /instances - Verwerk instances
  _server.on("/instances", HTTP_POST, [this]() {
    Serial.println("[INFO] POST /instances – processing instances");

    String body = _server.arg("plain");
    Serial.println("[DEBUG] Body: " + body);
    _configManager.setInstanceConfig(body.c_str());
    _configManager.writeConfig();

    _server.send(200, "text/plain", "Config saved.");
  });

  // POST /reboot - Handmatig herstarten
  _server.on("/reboot", HTTP_POST, [this]() {
    Serial.println("[INFO] POST /reboot - rebooting");
    _server.send(200, "text/plain", "Rebooting…");
    delay(2000);
    ESP.restart();
  });
}