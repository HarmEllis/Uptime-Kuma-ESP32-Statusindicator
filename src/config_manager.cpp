#include "config.h"
#include "config_manager.h"
#include <FS.h>
#include <LittleFS.h>

ConfigManager::ConfigManager(Config& cfg) : _cfg(cfg) {}

JsonDocument ConfigManager::deserializeInputJSON(String settingsJSON) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, settingsJSON);
  if (err) {
    Serial.print("[ERROR] ❌  JSON parse error: ");
    Serial.println(err.c_str());
    return getWifiConfig();
  }
  return doc;
}

JsonDocument ConfigManager::getWifiConfig() {
  JsonDocument doc;
  doc["ssid"]     = _cfg.wifi_ssid;
  doc["password"] = _cfg.wifi_pass;
  return doc;
}

JsonDocument ConfigManager::setWifiConfig(String settingsJSON) {
  JsonDocument doc = deserializeInputJSON(settingsJSON);
  _cfg.wifi_ssid  = doc["ssid"]   | "";
  _cfg.wifi_pass  = doc["password"] | "";
  Serial.println("[DEBUG] Wifi settings set: " + _cfg.wifi_ssid + " / " + _cfg.wifi_pass);
  return doc;
}

JsonDocument ConfigManager::getInstancesConfig(JsonDocument doc) {
  JsonArray arr = doc["instances"].to<JsonArray>();
  for (const auto& inst : _cfg.instances) {
    JsonObject obj = arr.add<JsonObject>();
    obj["id"]      = inst.id;
    obj["name"]    = inst.name;
    obj["endpoint"]= inst.endpoint;
    obj["apikey"]  = inst.apikey;
  }
  return doc;
}

void ConfigManager::setInstanceConfig(String settingsJSON) {
  JsonDocument doc = deserializeInputJSON(settingsJSON);
  _cfg.instances.clear();
  JsonArray arr = doc["instances"].as<JsonArray>();
  for (JsonObject obj : arr) {
    Instance inst;
    inst.id      = obj["id"]   | "";
    inst.name    = obj["name"] | "";
    inst.endpoint= obj["endpoint"] | "";
    inst.apikey  = obj["apikey"] | "";
    _cfg.instances.push_back(inst);
  }
}

bool ConfigManager::writeConfig() {
  Serial.println("[INFO] Writing config to LittleFS");
  JsonDocument doc = getWifiConfig();
  doc = getInstancesConfig(doc);

  File f = LittleFS.open("/config.json", "w");
  if (!f) {
    Serial.println("[ERROR] ❌  Cannot open config.json for writing");
    return false;
  }

  serializeJson(doc, f);
  f.close();
  return true;
}

bool ConfigManager::readConfig() {
  Serial.println("[INFO] Reading config from LittleFS");
  if (!LittleFS.begin(true)) {
    Serial.println("[ERROR] ❌  LittleFS init failed!");
    return false;
  }

  File f = LittleFS.open("/config.json", "r");
  if (!f) {
    Serial.println("[WARN] ⚠️  No config.json found – using defaults");
    return false;
  }

  size_t size = f.size();
  std::unique_ptr<char[]> buf(new char[size]);
  f.readBytes(buf.get(), size);
  setWifiConfig(buf.get());
  setInstanceConfig(buf.get());
  return true;
}