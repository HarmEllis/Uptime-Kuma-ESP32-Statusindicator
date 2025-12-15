#include "mdns_manager.h"
#include <ESPmDNS.h>

// Static member initialization
bool MDNSManager::_initialized = false;
String MDNSManager::_hostname = "";

bool MDNSManager::begin(const char* hostname) {
    if (_initialized) {
        Serial.println("[INFO] Already initialized");
        return true;
    }
    
    if (hostname == nullptr || strlen(hostname) == 0) {
        Serial.println("[INFO] Error: Invalid hostname");
        return false;
    }
    
    Serial.print("[INFO] Starting mDNS responder with hostname: ");
    Serial.println(hostname);
    
    if (!MDNS.begin(hostname)) {
        Serial.println("[ERROR] Failed to start mDNS responder");
        return false;
    }
    
    _hostname = hostname;
    _initialized = true;
    
    Serial.print("[INFO] Success! Device accessible at: http://");
    Serial.print(hostname);
    Serial.println(".local");
    
    return true;
}

void MDNSManager::addService(const char* service, const char* proto, uint16_t port) {
    if (!_initialized) {
        Serial.println("[INFO] Error: mDNS not initialized. Call begin() first.");
        return;
    }
    
    MDNS.addService(service, proto, port);
    Serial.print("[INFO] Service added: _");
    Serial.print(service);
    Serial.print("._");
    Serial.print(proto);
    Serial.print(" port ");
    Serial.println(port);
}

void MDNSManager::addServiceTxt(const char* service, const char* proto, const char* key, const char* value) {
    if (!_initialized) {
        Serial.println("[ERROR] mDNS not initialized. Call begin() first.");
        return;
    }
    
    MDNS.addServiceTxt(service, proto, key, value);
    Serial.print("[INFO] Service TXT added: ");
    Serial.print(key);
    Serial.print("=");
    Serial.println(value);
}

bool MDNSManager::isInitialized() {
    return _initialized;
}

const char* MDNSManager::getHostname() {
    return _hostname.c_str();
}