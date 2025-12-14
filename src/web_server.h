#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include "config.h"
#include "config_manager.h"

class WebServerManager {
public:
  WebServerManager(Config& cfg, ConfigManager& configManager);
  
  void setup();
  void handleClient();
  
private:
  Config& _cfg;
  ConfigManager& _configManager;
  WebServer _server;
  
  void setupRoutes();
};

#endif