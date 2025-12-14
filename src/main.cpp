#include <Arduino.h>
#include "config.h"
#include "config_manager.h"
#include "wifi_manager.h"
#include "led_manager.h"
#include "web_server.h"

/* ----------- Global objects --------------------------------- */
Config cfg;
LedState ledState;

ConfigManager configManager(cfg);
WiFiManager wifiManager(cfg, ledState);
LedManager ledManager(ledState);
WebServerManager webServer(cfg, configManager);

/* ----------- Externe LED manager pointer -------------------- */
extern LedManager* g_ledManager;

/* ----------- setup ------------------------------------------ */
void setup() {
  Serial.begin(115200);
  Serial.println();

  // Configuratie laden
  configManager.readConfig();

  // LEDs initialiseren
  ledManager.setup();

  // WiFi setup
  wifiManager.setup();

  // Webserver starten
  webServer.setup();

  // LED task starten (FreeRTOS)
  g_ledManager = &ledManager;
  xTaskCreatePinnedToCore(ledTaskFunc, "LedTask", 2048, NULL, 1, NULL, 0);
}

/* ----------- loop ------------------------------------------- */
void loop() {
  webServer.handleClient();
}