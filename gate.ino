#include "gate_config.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <PubSubClient.h>
#include "WiFiManager.h"
#include "gate_actions.h"

bool isGateBusy = false;
bool delayedPending = false;
String delayedRequesterId = "";
unsigned long lastRequestTime = 0;

BearSSL::WiFiClientSecure secureClient;
PubSubClient mqtt(secureClient);

// --- BlynkTimer yerine basit zamanlayici ---
struct DelayedAction {
  unsigned long triggerTime;
  void (*callback)();
  bool active;
};
static DelayedAction actions[4] = {};

void scheduleAction(void (*callback)(), unsigned long delayMs) {
  for (int i = 0; i < 4; i++) {
    if (!actions[i].active) {
      actions[i] = {millis() + delayMs, callback, true};
      return;
    }
  }
}

void processScheduledActions() {
  unsigned long now = millis();
  for (int i = 0; i < 4; i++) {
    if (actions[i].active && now >= actions[i].triggerTime) {
      actions[i].active = false;
      actions[i].callback();
    }
  }
}

void clearScheduledActions() {
  for (int i = 0; i < 4; i++) {
    actions[i].active = false;
  }
}

bool isThrottled() {
  if (millis() - lastRequestTime < 1500) return true;
  lastRequestTime = millis();
  return false;
}

void publishLastOpened() {
  if (!mqtt.connected()) return;
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", millis() / 1000);
  mqtt.publish(MQTT_TOPIC_LAST_OPENED, buf, true);
}

void publishEventFor(const String& evt, const String& targetId = "") {
  if (!mqtt.connected()) return;
  String payload = evt;
  if (targetId.length() > 0) {
    payload += "|";
    payload += targetId;
  }
  mqtt.publish(MQTT_TOPIC_EVENT, payload.c_str(), false);
}

void executeDelayedOpen() {
  Serial.println("Gecikmeli acma tetiklendi.");
  delayedPending = false;
  publishEventFor("triggered", delayedRequesterId);
  delayedRequesterId = "";
  openGate();
  publishLastOpened();
}

// --- MQTT mesaj handler ---
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String raw;
  for (unsigned int i = 0; i < length; i++) raw += (char)payload[i];

  String cmd = raw;
  String senderId = "";
  int sep = raw.indexOf('|');
  if (sep >= 0) {
    cmd = raw.substring(0, sep);
    senderId = raw.substring(sep + 1);
  }

  Serial.printf("MQTT komut: %s\n", cmd.c_str());

  if (cmd == "restart") {
    digitalWrite(LED_PIN, LOW);
    publishEventFor("restarting", senderId);
    delay(100);
    ESP.restart();
    return;
  }

  if (cmd == "cancel") {
    if (delayedPending) {
      String targetId = delayedRequesterId.length() > 0 ? delayedRequesterId : senderId;
      clearScheduledActions();
      delayedPending = false;
      delayedRequesterId = "";
      isGateBusy = false;
      digitalWrite(RELAY_PIN, RELAY_OFF);
      setWiFiLedEnabled(true);
      digitalWrite(LED_PIN, HIGH);
      Serial.println("Gecikmeli istek iptal edildi.");
      publishEventFor("cancelled", targetId);
    } else {
      Serial.println("Iptal edilecek bekleyen istek yok.");
      publishEventFor("cancel_empty", senderId);
    }
    return;
  }

  if (isGateBusy || delayedPending) {
    Serial.println("Kapi mesgul, istek reddedildi.");
    publishEventFor("rejected_busy", senderId);
    return;
  }
  if (isThrottled()) {
    Serial.println("Throttled, komut atlandi.");
    publishEventFor("rejected_throttled", senderId);
    return;
  }

  if (cmd == "open_now") {
    publishEventFor("accepted_now", senderId);
    openGate();
    publishLastOpened();
  } else if (cmd.startsWith("delay:")) {
    int delaySec = cmd.substring(6).toInt();
    if (delaySec > 0 && delaySec <= 120) {
      delayedPending = true;
      delayedRequesterId = senderId;
      char evt[24];
      snprintf(evt, sizeof(evt), "accepted_delay:%d", delaySec);
      publishEventFor(evt, senderId);
      Serial.printf("Istek: %d sn sonra acilacak.\n", delaySec);
      scheduleAction(executeDelayedOpen, (unsigned long)delaySec * 1000);
    } else {
      Serial.println("Gecersiz delay degeri.");
      publishEventFor("invalid_delay", senderId);
    }
  }
}

void connectMqtt() {
  static unsigned long lastTry = 0;
  if (mqtt.connected()) return;
  if (millis() - lastTry < 5000) return;
  lastTry = millis();

  Serial.println("MQTT'ye baglaniliyor...");
  if (mqtt.connect("gate-esp8266", MQTT_USER, MQTT_PASS,
        MQTT_TOPIC_STATUS, 1, true, MQTT_LWT_PAYLOAD)) {
    Serial.println("MQTT baglandi!");
    mqtt.subscribe(MQTT_TOPIC_CMD, 1);
    if (!isGateBusy && !delayedPending) {
      digitalWrite(LED_PIN, LOW);
      scheduleAction([]() { if (!isGateBusy) digitalWrite(LED_PIN, HIGH); }, 100);
    }
  } else {
    Serial.printf("MQTT hata: %d\n", mqtt.state());
  }
}

void periodicTasks() {
  unsigned long uptimeSec = millis() / 1000;

  static unsigned long lastStatusPublish = 0;
  if (mqtt.connected() && uptimeSec - lastStatusPublish >= STATUS_INTERVAL_SEC) {
    lastStatusPublish = uptimeSec;
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu", uptimeSec);
    mqtt.publish(MQTT_TOPIC_STATUS, buf, true);
  }

  static unsigned long lastLog = 0;
  if (uptimeSec - lastLog >= 60) {
    lastLog = uptimeSec;
    Serial.printf("Heap: %u bytes | Uptime: %lu sn\n", ESP.getFreeHeap(), uptimeSec);
  }

  if (uptimeSec > 86400 && !isGateBusy) {
    Serial.println("24 saat doldu, otomatik restart...");
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  int unusedPins[] = {0, 2, 4, 12, 13, 14, 15};
  for (int i = 0; i < sizeof(unusedPins) / sizeof(int); i++) {
    int pin = unusedPins[i];
    if (pin == RELAY_PIN || pin == LED_PIN) continue;
    pinMode(pin, INPUT_PULLUP);
  }

  setupWiFi(ssid, pass);

  secureClient.setInsecure();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
}

void loop() {
  handleWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    connectMqtt();
    mqtt.loop();
  }

  static unsigned long gateBusySince = 0;
  if (isGateBusy) {
    if (gateBusySince == 0) gateBusySince = millis();
    if (millis() - gateBusySince > 15000) {
      closeGate();
      Serial.println("UYARI: Gate busy timeout, zorla sifirlandı!");
    }
  } else {
    gateBusySince = 0;
  }

  processScheduledActions();
  periodicTasks();
}
