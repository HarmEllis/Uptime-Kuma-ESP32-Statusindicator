#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include "config.h"

class LedManager {
public:
  LedManager(LedState& ledState);
  
  void setup();
  void setLEDState(uint8_t pin, bool state);
  void ledTask(); // Voor de FreeRTOS task
  
private:
  LedState& _ledState;
};

// FreeRTOS task wrapper functie
void ledTaskFunc(void* parameter);

#endif