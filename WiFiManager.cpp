#include "WiFiManager.h"

static unsigned long lastWiFiTryTime = 0;
static const unsigned long wifiTryInterval = 10000;
static const char* _ssid;
static const char* _pass;
static bool wasConnected = false;

// LED durum kontrolu degiskenleri
static unsigned long ledLastToggledTime = 0;
static unsigned long disconnectedStartTime = 0;
static bool blinkState = false;

// Baglanti animasyonu degiskenleri
static unsigned long connectedLedStartTime = 0;
static bool showingConnectedLed = false;
static int connectedBlinkCount = 0;

// Gate islemleri sirasinda WiFiManager'in LED'e dokunmasini engeller
static bool wifiLedEnabled = true;

void setWiFiLedEnabled(bool enabled) {
    wifiLedEnabled = enabled;
}

void setupWiFi(const char* ssid, const char* pass) {
    _ssid = ssid;
    _pass = pass;
    
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _pass);
    WiFi.setOutputPower(17);
    WiFi.setSleepMode(WIFI_MODEM_SLEEP);
    lastWiFiTryTime = millis();
}

void handleWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    if (wasConnected) {
       wasConnected = false;
       disconnectedStartTime = millis(); 
    }
    
    unsigned long currentMillis = millis();
    unsigned long disconnectedDuration = currentMillis - disconnectedStartTime;

    if (wifiLedEnabled) {
      if (disconnectedDuration <= 10000) {
          if (currentMillis - ledLastToggledTime >= 500) {
             blinkState = !blinkState;
             digitalWrite(LED_BUILTIN, blinkState ? LOW : HIGH);
             ledLastToggledTime = currentMillis;
          }
      } else {
          digitalWrite(LED_BUILTIN, HIGH); 
      }
    }

    if (currentMillis - lastWiFiTryTime >= wifiTryInterval) {
      Serial.println("WiFi aranıyor...");
      WiFi.begin(_ssid, _pass);
      lastWiFiTryTime = millis();
    }
  } else {
    if (!wasConnected) {
      Serial.println("\nWiFi baglandi! Hizli yanip sönüyor...");
      
      connectedLedStartTime = millis();
      showingConnectedLed = true;
      connectedBlinkCount = 0;
      blinkState = true;
      if (wifiLedEnabled) digitalWrite(LED_BUILTIN, LOW);
      
      wasConnected = true;
      disconnectedStartTime = 0;
    }

    if (showingConnectedLed && wifiLedEnabled) {
       unsigned long currentMillis = millis();
       if (currentMillis - connectedLedStartTime >= 100) {
          blinkState = !blinkState;
          digitalWrite(LED_BUILTIN, blinkState ? LOW : HIGH);
          connectedLedStartTime = currentMillis;
          
          connectedBlinkCount++;
          
          if (connectedBlinkCount >= 6) {
             digitalWrite(LED_BUILTIN, HIGH);
             showingConnectedLed = false;
          }
       }
    }
  }
}
