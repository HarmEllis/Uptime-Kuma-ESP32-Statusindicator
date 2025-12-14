#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

class ConfigManager {
public:
  ConfigManager(Config& cfg);
  
  bool readConfig();
  bool writeConfig();
  
  JsonDocument getWifiConfig();
  JsonDocument setWifiConfig(String settingsJSON);
  
  JsonDocument getInstancesConfig(JsonDocument doc);
  void setInstanceConfig(String settingsJSON);

private:
  Config& _cfg;
  JsonDocument deserializeInputJSON(String settingsJSON);
};

#endif