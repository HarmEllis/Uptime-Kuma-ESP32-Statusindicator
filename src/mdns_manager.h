#ifndef MDNS_MANAGER_H
#define MDNS_MANAGER_H

#include <Arduino.h>

class MDNSManager {
public:
    // Initialize mDNS with the given hostname
    static bool begin(const char* hostname);
    
    // Add a service to mDNS-SD
    static void addService(const char* service, const char* proto, uint16_t port);
    
    // Add service text record
    static void addServiceTxt(const char* service, const char* proto, const char* key, const char* value);
    
    // Check if mDNS is initialized
    static bool isInitialized();
    
    // Get the current hostname
    static const char* getHostname();
    
private:
    static bool _initialized;
    static String _hostname;
};

#endif // MDNS_MANAGER_H