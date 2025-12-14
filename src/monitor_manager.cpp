#include "monitor_manager.h"

MonitorManager::MonitorManager(Config& cfg, LedState& ledState)
  : _cfg(cfg), _ledState(ledState) {}

void MonitorManager::pollAllInstances() {
  if (_cfg.instances.empty()) {
    Serial.println("[INFO] No instances configured, skipping poll");
    return;
  }

  std::vector<MonitorResult> results;
  
  // Poll each instance
  for (const auto& instance : _cfg.instances) {
    Serial.printf("\n[INFO] Polling instance: %s\n", instance.name.c_str());
    MonitorResult result = fetchMetrics(instance);
    results.push_back(result);
    
    // Print summary for this instance
    Serial.printf("=== %s Metrics ===\n", instance.name.c_str());
    Serial.printf("API reachable: %s\n", result.apiReachable ? "yes" : "no");
    Serial.printf("API key valid: %s\n", result.apiKeyValid ? "yes" : "no");
    Serial.printf("Monitors up:   %d\n", result.monitorsUp);
    Serial.printf("Monitors down: %d\n", result.monitorsDown);
    
    if (!result.apiReachable) {
      Serial.println("!! Instance is UNREACHABLE !!");
    } else if (result.monitorsDown > 0) {
      Serial.println("!! One or more monitors are DOWN !!");
    } else {
      Serial.println("All monitors are UP.");
    }
  }
  
  // Update LED state based on all results
  updateLEDState(results);
}

MonitorResult MonitorManager::fetchMetrics(const Instance& instance) {
  MonitorResult result;
  
  HTTPClient http;
  WiFiClient* client = nullptr;

  // HTTPS or HTTP
  if (instance.endpoint.startsWith("https://")) {
    // For production use, set a CA certificate
    client = new WiFiClientSecure();
    ((WiFiClientSecure*)client)->setInsecure();
  } else {
    client = new WiFiClient();
  }

  // Build the URL
  String url = instance.endpoint + "/metrics";

  // Basic Auth for API key
  String auth = ":" + instance.apikey;
  auth = base64::encode(auth);
  http.setAuthorization(auth.c_str());

  http.begin(*client, url);
  http.setTimeout(15000); // 15s timeout

  int httpCode = http.GET();
  
  if (httpCode > 0) {
    result.apiReachable = true;
    String payload = http.getString();

    // Parse the Prometheus text format
    parseMetrics(payload, result);

    // If API key was used, valid gauge indicates it works
    result.apiKeyValid = (result.monitorsUp + result.monitorsDown) > 0;
  } else {
    // HTTP error - could not reach the endpoint
    result.apiReachable = false;
    Serial.printf("[ERROR] HTTP error: %s (code %d)\n", 
                  http.errorToString(httpCode).c_str(), httpCode);
  }

  http.end();
  if (client) {
    delete client;
  }
  
  return result;
}

void MonitorManager::parseMetrics(const String& data, MonitorResult& result) {
  /* The Prometheus format is plain text, each line looks like:
     monitor_status{monitor_name="...", monitor_type="..."} 1
     monitor_status{monitor_name="...", monitor_type="..."} 0
     etc.
     We only care about the gauge value (0 = DOWN, 1 = UP). */

  int pos = 0;
  int lineStart = 0;
  
  // Process line by line
  while (lineStart < data.length()) {
    int lineEnd = data.indexOf('\n', lineStart);
    if (lineEnd == -1) lineEnd = data.length();
    
    String line = data.substring(lineStart, lineEnd);
    line.trim();
    
    // Only process lines that start with "monitor_status{"
    if (line.startsWith("monitor_status{")) {
      // Find the closing brace
      int braceEnd = line.indexOf('}');
      if (braceEnd == -1) {
        lineStart = lineEnd + 1;
        continue;
      }
      
      // Get everything after the closing brace
      String valueStr = line.substring(braceEnd + 1);
      valueStr.trim();
      
      int status = valueStr.toInt(); // 0,1,2,3

      switch (status) {
        case 0: 
          result.monitorsDown++; 
          break; // DOWN
        case 1: 
          result.monitorsUp++; 
          break;   // UP
        // 2 = PENDING, 3 = MAINTENANCE - ignore for this summary
        default: 
          break;
      }
    }
    
    lineStart = lineEnd + 1;
  }
}

void MonitorManager::updateLEDState(const std::vector<MonitorResult>& results) {
  bool anyUnreachable = false;
  bool anyDown = false;
  bool allUp = true;

  // Check all results
  for (const auto& result : results) {
    if (!result.apiReachable) {
      anyUnreachable = true;
      allUp = false;
    } else if (result.monitorsDown > 0) {
      anyDown = true;
      allUp = false;
    }
  }

  // Update LED state based on priority:
  // 1. Unreachable (highest priority) -> Red blinking
  // 2. Monitors down -> Red solid
  // 3. All up -> Green solid
  
  if (anyUnreachable) {
    // Red blinking - instance unreachable
    _ledState.greenOn = false;
    _ledState.redBlink = true;
    _ledState.redOn = true;
    Serial.println("[LED] Status: RED BLINKING (instance unreachable)");
  } else if (anyDown) {
    // Red solid - one or more monitors down
    _ledState.greenOn = false;
    _ledState.redBlink = false;
    _ledState.redOn = true;
    Serial.println("[LED] Status: RED SOLID (monitors down)");
  } else if (allUp) {
    // Green solid - all monitors up
    _ledState.greenOn = true;
    _ledState.redBlink = false;
    _ledState.redOn = false;
    Serial.println("[LED] Status: GREEN (all monitors up)");
  }
}

void MonitorManager::pollTask() {
  pollAllInstances();
  
  // Wait for the configured poll interval
  delay(_cfg.pollInterval * 1000);
}

// Global pointer for the task (needed because FreeRTOS uses C-style callbacks)
MonitorManager* g_monitorManager = nullptr;

void pollTaskFunc(void* parameter) {
  // Wait a bit before starting the first poll
  delay(5000);
  
  for (;;) {
    if (g_monitorManager != nullptr) {
      g_monitorManager->pollTask();
    }
  }
}