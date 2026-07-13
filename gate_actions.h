#ifndef GATE_ACTIONS_H
#define GATE_ACTIONS_H

#include "gate_config.h"
#include <Arduino.h>
#include "WiFiManager.h"

extern bool isGateBusy;
extern void scheduleAction(void (*callback)(), unsigned long delayMs);

void closeGate() {
  digitalWrite(RELAY_PIN, RELAY_OFF);
  setWiFiLedEnabled(true);
  digitalWrite(LED_PIN, HIGH);
  isGateBusy = false;
  Serial.println("Kapi gucu kesildi (800ms doldu). Sistem yeni isteklere acik.");
}

void openGate() {
  isGateBusy = true;
  setWiFiLedEnabled(false);
  // LED only turns on when gate action really starts.
  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAY_PIN, RELAY_ON);
  Serial.println("Kapi gucu verildi, aciliyor...");
  scheduleAction(closeGate, 800);
}

#endif
