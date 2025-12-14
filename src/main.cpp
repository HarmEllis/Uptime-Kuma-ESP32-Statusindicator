#include <Arduino.h>
#include "config.h"
#include "config_manager.h"
#include "wifi_manager.h"
#include "led_manager.h"
#include "web_server.h"
#include "monitor_manager.h"

/* ----------- Global objects --------------------------------- */
Config cfg;
LedState ledState;

ConfigManager configManager(cfg);
WiFiManager wifiManager(cfg, ledState);
LedManager ledManager(ledState);
WebServerManager webServer(cfg, configManager);
MonitorManager monitorManager(cfg, ledState);

/* ----------- External manager pointers ---------------------- */
extern LedManager* g_ledManager;
extern MonitorManager* g_monitorManager;

/* ----------- setup ------------------------------------------ */
void setup() {
  Serial.begin(115200);
  Serial.println();

  // Load configuration
  configManager.readConfig();

  // Initialize LEDs
  ledManager.setup();

  // WiFi setup
  wifiManager.setup();

  // Start webserver
  webServer.setup();

  // Start LED task (FreeRTOS)
  g_ledManager = &ledManager;
  xTaskCreatePinnedToCore(ledTaskFunc, "LedTask", 2048, NULL, 1, NULL, 0);

  // Start monitoring task (FreeRTOS)
  g_monitorManager = &monitorManager;
  xTaskCreatePinnedToCore(pollTaskFunc, "PollTask", 8192, NULL, 2, NULL, 1);
}

/* ----------- loop ------------------------------------------- */
void loop() {
  webServer.handleClient();
}