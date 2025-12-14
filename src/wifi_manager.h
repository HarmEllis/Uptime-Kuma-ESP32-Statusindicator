#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

class WiFiManager {
public:
  WiFiManager(Config& cfg, LedState& ledState);
  
  void setup();
  void startHotspot();
  
private:
  Config& _cfg;
  LedState& _ledState;
};

#endif