#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <ESP8266WiFi.h>

void setupWiFi(const char* ssid, const char* pass);
void handleWiFi();
void setWiFiLedEnabled(bool enabled);

#endif
