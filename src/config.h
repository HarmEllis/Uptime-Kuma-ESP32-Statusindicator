#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <vector>
#include <IPAddress.h>

/* ----------- WiFi Hotspot Constanten ------------------------------------------- */
constexpr const char* AP_SSID = "ESP32_Hotspot";
constexpr const char* AP_PASSWORD = "ESP32Wroom32";
constexpr uint8_t AP_CHANNEL = 6;

// IPAddress kan niet constexpr zijn, dus deze blijven gewoon const
const IPAddress AP_IP = IPAddress(192, 168, 4, 1);
const IPAddress AP_GATEWAY = AP_IP;
const IPAddress AP_SUBNET = IPAddress(255, 255, 255, 0);

/* ----------- WiFi Constanten --------------------------------------------------- */
constexpr uint8_t WIFI_CONNECT_TIMEOUT = 30; // seconden

/* ----------- LED Constanten ---------------------------------------------------- */
#define LED_ON HIGH
#define LED_OFF LOW

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // GPIO2 voor de meeste ESP32 boards
#endif

constexpr uint8_t GREEN_PIN = 22; // GPIO-pin groene LED
constexpr uint8_t RED_PIN = 23;   // GPIO-pin rode LED
constexpr uint16_t BLINK_INTERVAL_MS = 500;

/* ----------- Structs ----------------------------------------------------------- */
struct Instance {
  String id;       // UUID van instance
  String name;     // Instance naam
  String endpoint; // Uptime-Kuma endpoint URL
  String apikey;   // API-Key
};

struct Config {
  String wifi_ssid;
  String wifi_pass;
  uint16_t pollInterval = 15; // seconden
  std::vector<Instance> instances;
};

struct LedState {
  bool greenOn = false;
  bool redBlink = false;
  uint32_t lastRedToggle = 0;
  bool redOn = false;
  bool builtinBlink = false;
  bool builtinOn = false;
  uint32_t lastBuiltinToggle = 0;
};

#endif