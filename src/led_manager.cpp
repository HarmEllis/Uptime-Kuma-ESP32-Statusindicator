#include "config.h"
#include "led_manager.h"

LedManager::LedManager(LedState& ledState) : _ledState(ledState) {}

void LedManager::setup() {
  Serial.println("[INFO] Setting up LEDs");
  
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  #ifdef LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  #endif

  // Blink alle LEDs om functionaliteit te testen
  digitalWrite(GREEN_PIN, LED_ON);
  digitalWrite(RED_PIN, LED_ON);
  #ifdef LED_BUILTIN
  digitalWrite(LED_BUILTIN, LED_ON);
  #endif

  delay(1000);

  digitalWrite(GREEN_PIN, LED_OFF);
  digitalWrite(RED_PIN, LED_OFF);
  #ifdef LED_BUILTIN
  digitalWrite(LED_BUILTIN, LED_OFF);
  #endif
}

void LedManager::setLEDState(uint8_t pin, bool state) {
  if (state) {
    digitalWrite(pin, LED_ON);
  } else {
    digitalWrite(pin, LED_OFF);
  }
}

void LedManager::ledTask() {
  setLEDState(GREEN_PIN, _ledState.greenOn);

  if (_ledState.redBlink) {
    if (millis() - _ledState.lastRedToggle >= BLINK_INTERVAL_MS) {
      _ledState.redOn = !_ledState.redOn;
      _ledState.lastRedToggle = millis();
    }
  }
  setLEDState(RED_PIN, _ledState.redOn);

  #ifdef LED_BUILTIN
  if (_ledState.builtinBlink) {
    if (millis() - _ledState.lastBuiltinToggle >= BLINK_INTERVAL_MS) {
      _ledState.builtinOn = !_ledState.builtinOn;
      _ledState.lastBuiltinToggle = millis();
    }
  }
  setLEDState(LED_BUILTIN, _ledState.builtinOn);
  #endif

  delay(50);
}

// Global pointer voor de task (nodig omdat FreeRTOS C-style callbacks gebruikt)
LedManager* g_ledManager = nullptr;

void ledTaskFunc(void* parameter) {
  for (;;) {
    if (g_ledManager != nullptr) {
      g_ledManager->ledTask();
    }
  }
}