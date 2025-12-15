#ifndef MONITOR_MANAGER_H
#define MONITOR_MANAGER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <base64.h>
#include "config.h"

struct MonitorResult {
  bool apiReachable = false;
  bool apiKeyValid = true;
  int monitorsUp = 0;
  int monitorsDown = 0;
};

class MonitorManager {
public:
  MonitorManager(Config& cfg, LedState& ledState);
  
  void pollAllInstances();
  void pollTask(); // For FreeRTOS task
  
private:
  Config& _cfg;
  LedState& _ledState;
  
  MonitorResult fetchMetrics(const Instance& instance);
  void parseMetrics(const String& data, MonitorResult& result);
  void updateLEDState(const std::vector<MonitorResult>& results);
};

// FreeRTOS task wrapper function
void pollTaskFunc(void* parameter);

#endif