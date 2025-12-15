#include "web_server.h"
#include "config.h"
#include <FS.h>
#include <LittleFS.h>
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

bool WebServerManager::checkAuth() {
  if (!_server.authenticate(WEB_USER, WEB_PASSWORD)) {
    _server.requestAuthentication();
    return false;
  }
  return true;
}

void WebServerManager::setupRoutes() {
  // GET / - serve the configuration page
  _server.on("/", HTTP_GET, [this]() {
    if (!checkAuth()) {
      return;
    }
    Serial.println("[INFO] GET / - serving config page");
    if (!LittleFS.exists("/index.html")) {
      _server.send(404, "text/plain", "File not found");
      Serial.println("[ERROR] GET / - config page not found on fs");
      return;
    }
    File indexFile = LittleFS.open("/index.html", "r");
    _server.streamFile(indexFile, "text/html", false);
    indexFile.close();
  });

  // GET /wifi - return Wi‑Fi settings as JSON
  _server.on("/wifi", HTTP_GET, [this]() {
    if (!checkAuth()) {
      return;
    }
    Serial.println("[INFO] GET /wifi - returning wifi settings as JSON");
    JsonDocument doc = _configManager.getWifiConfig();
    String wifiJSON;
    serializeJson(doc, wifiJSON);
    _server.send(200, "application/json", wifiJSON);
  });

  // GET /instances - return all Uptime Kuma instances as JSON
  _server.on("/instances", HTTP_GET, [this]() {
    if (!checkAuth()) {
      return;
    }
    Serial.println("[INFO] GET /instances - returning Uptime Kuma instances as JSON");
    JsonDocument doc;
    doc = _configManager.getInstancesConfig(doc);
    String instancesJSON;
    serializeJson(doc, instancesJSON);
    _server.send(200, "application/json", instancesJSON);
  });

  // POST /wifi - process new Wi‑Fi settings
  _server.on("/wifi", HTTP_POST, [this]() {
    if (!checkAuth()) {
      return;
    }
    Serial.println("[INFO] POST /wifi - processing wifi settings");
    String body = _server.arg("plain");
    Serial.println("[DEBUG] Body: " + body);
    _configManager.setWifiConfig(body.c_str());
    _configManager.writeConfig();
    _server.send(200, "text/plain", "Config saved. Reboot to apply.");
  });

  // POST /instances - process new instance list
  _server.on("/instances", HTTP_POST, [this]() {
    if (!checkAuth()) {
      return;
    }
    Serial.println("[INFO] POST /instances - processing instances");
    String body = _server.arg("plain");
    Serial.println("[DEBUG] Body: " + body);
    _configManager.setInstanceConfig(body.c_str());
    _configManager.writeConfig();
    _server.send(200, "text/plain", "Config saved.");
  });

  // POST /reboot - manual reboot
  _server.on("/reboot", HTTP_POST, [this]() {
    if (!checkAuth()) {
      return;
    }
    Serial.println("[INFO] POST /reboot - rebooting");
    _server.send(200, "text/plain", "Rebooting…");
    delay(2000);
    ESP.restart();
  });
}