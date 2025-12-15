#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <vector>
#include <IPAddress.h>

/* ----------- WiFi Hotspot Constants --------------------------------------------- */
constexpr const char* AP_SSID = "ESP32_Hotspot";
constexpr const char* AP_PASSWORD = "FFzppG3oJ76PRs";
constexpr uint8_t AP_CHANNEL = 6;
const IPAddress AP_IP = IPAddress(192, 168, 4, 1);
const IPAddress AP_GATEWAY = AP_IP;
const IPAddress AP_SUBNET = IPAddress(255, 255, 255, 0);

/* ----------- WiFi Constant ------------------------------------------------------ */
constexpr uint8_t WIFI_CONNECT_TIMEOUT = 30; // seconden

/* ----------- Web server  -------------------------------------------------------- */
constexpr const char* WEB_USER = "UptimeKumaMonitorAdmin";
constexpr const char* WEB_PASSWORD = "gH!cwb#3SR(7bY";

// mDNS hostname - device will be accessible at http://esp-uptimemonitor.local
constexpr const char* MDNS_HOSTNAME = "esp-uptimemonitor";

/* ----------- LED Constants ------------------------------------------------------ */
#define LED_ON HIGH
#define LED_OFF LOW

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // GPIO2 for most ESP32 boards
#endif

constexpr uint8_t GREEN_PIN = 22; // GPIO-pin groene LED
constexpr uint8_t RED_PIN = 23;   // GPIO-pin rode LED
constexpr uint16_t BLINK_INTERVAL_MS = 500;

/* ----------- Structs ----------------------------------------------------------- */
struct Instance {
  String id;       // UUID
  String name;
  String endpoint;
  String apikey;
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