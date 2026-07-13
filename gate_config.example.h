#ifndef GATE_CONFIG_H
#define GATE_CONFIG_H

// Copy this file to gate_config.h and fill in your own values.
// gate_config.h is gitignored and must never be committed.

// WiFi
const char ssid[] = "YOUR_WIFI_SSID";
const char pass[] = "YOUR_WIFI_PASSWORD";

// MQTT (HiveMQ Cloud - https://console.hivemq.cloud/)
const char MQTT_HOST[] = "your-cluster-id.s1.eu.hivemq.cloud";
const int  MQTT_PORT   = 8883;
const char MQTT_USER[] = "YOUR_MQTT_USERNAME";
const char MQTT_PASS[] = "YOUR_MQTT_PASSWORD";
const char MQTT_TOPIC_CMD[]    = "gate/cmd";
const char MQTT_TOPIC_STATUS[] = "gate/status";
const char MQTT_TOPIC_LAST_OPENED[] = "gate/last_opened";
const char MQTT_TOPIC_EVENT[] = "gate/event";
const char MQTT_LWT_PAYLOAD[] = "offline";
const unsigned long STATUS_INTERVAL_SEC = 5;

// Hardware Pins
#define RELAY_PIN D1
#define RELAY_ON  HIGH
#define RELAY_OFF LOW
#define LED_PIN   LED_BUILTIN

#endif
