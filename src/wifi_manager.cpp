#include "wifi_manager.h"

WiFiManager::WiFiManager(Config& cfg, LedState& ledState) 
  : _cfg(cfg), _ledState(ledState) {}

void WiFiManager::startHotspot() {
  Serial.print("[INFO] Hotspot config:   ");
  Serial.println(WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET) ? "Ready" : "Failed");
  
  WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, false, 6);
  IPAddress localIP = WiFi.softAPIP();
  
  Serial.print("[INFO] Hotspot IP:       ");
  Serial.println(localIP);
  Serial.print("[INFO] Hotspot SSID:     ");
  Serial.println(AP_SSID);
  Serial.print("[INFO] Hotspot Password: ");
  Serial.println(AP_PASSWORD);
  
  _ledState.builtinBlink = true;
}

void WiFiManager::setup() {
  if (_cfg.wifi_ssid == "" || _cfg.wifi_pass == "") {
    Serial.println("[WARN] No WiFi credentials set, starting Hotspot");
    startHotspot();
    return;
  }

  int attempt = 0;
  WiFi.begin(_cfg.wifi_ssid.c_str(), _cfg.wifi_pass.c_str());
  Serial.print("[INFO] Connecting to WiFi network: " + _cfg.wifi_ssid);
  
  while (WiFi.status() != WL_CONNECTED && attempt < WIFI_CONNECT_TIMEOUT * 2) {
    delay(500);
    Serial.print(".");
    attempt++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[ERROR] Could not connect to WIFI, starting Hotspot");
    startHotspot();
  } else {
    Serial.println("\n[INFO] WiFi connected, IP: " + WiFi.localIP().toString());
    _ledState.builtinOn = true;
  }
}