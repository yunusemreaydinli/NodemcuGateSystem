#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define WIFI_SSID "WIFI NAME"
#define WIFI_PASSWORD "WIFI PASSWORD"

#define BOT_TOKEN "TELEGRAM BOT TOKEN"

const char * USER_IDS[] = {
    "USER ID",
};

const char * ADMIN_IDS[] = {
    "ADMIN ID", 
};

const unsigned long BOT_MTBS = 500;

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;

const int ledPin = D1;
bool bakim = false;

void handleNewMessages(int numNewMessages) {
  Serial.print("handleNewMessages ");
  Serial.println(numNewMessages);

//Kullanıcı yetki kontrolü
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;

    bool yetki = false;
    bool admin = false;

    for (int y = 0; y < (sizeof USER_IDS / sizeof *USER_IDS); y++) {
      if (chat_id == USER_IDS[y]){
        yetki = true;
      }
    }

    for (int y = 0; y < (sizeof ADMIN_IDS / sizeof *ADMIN_IDS); y++) {
      if (chat_id == ADMIN_IDS[y]){
        admin = true;
      }
    }

   if (yetki == false){
        bot.sendMessage(chat_id, "🚫 *Yetkisiz Kullanıcı!*", "Markdown");
        continue;
      }
//---------------------------

    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;
    if (from_name == "") {
      from_name = "Ziyaretçi: ";
    }

    if(bakim == true){
      bot.sendMessage(chat_id, "🟢 *Bakım modu açık lütfen daha sonra tekrar deneyiniz.*", "Markdown");
    }

    if (text == "🚪 Kapıyı aç" && bakim == false) {
      bot.sendMessage(chat_id, "*Kapı açılıyor... ⏳*", "Markdown");
      digitalWrite(ledPin, HIGH);
      delay(700); // Zile basma süresi
      digitalWrite(ledPin, LOW);
      bot.sendMessage(chat_id, "Hoşgeldiniz, *" + from_name + "*!", "Markdown");
    }

// Alt panel buton
  if (text == "/start") {
      String welcome = "Merhaba *" + from_name + "*, NodemcuGateBot'a hoşgeldiniz! 😊\n";
      bot.sendMessage(chat_id, welcome, "Markdown");

      String keyboardJson = "[[\"🚪 Kapıyı aç\"]]";

      if (admin == true) {
        keyboardJson = "[[\"🚪 Kapıyı aç\"],[\"🔧 Admin Paneli\"]]";      
      }
        bot.sendMessageWithReplyKeyboard(chat_id, "*Lütfen seçeneklerden seçim yapın.*", "Markdown", keyboardJson, true);
    }
    if (admin == true) {
      if (text == "🔧 Admin Paneli") {
        String keyboardJson = "[[{ \"text\" : \"🟢 Bakım Modunu Aç\", \"callback_data\" : \"bakim_modunu_ac\" }],[{ \"text\" : \"🔴 Bakım Modunu Kapat\", \"callback_data\" : \"bakim_modunu_kapat\" }],[{ \"text\" : \"🔄 Yeniden Başlat\", \"callback_data\" : \"resetle\" }]]";
        bot.sendMessageWithInlineKeyboard(chat_id, "🔵 *Yönetici: " + from_name + "*, Admin Paneline Hoşgeldiniz!", "Markdown", keyboardJson);
      }
      if (bot.messages[i].type == "callback_query") {
        if(bot.messages[i].text == "bakim_modunu_ac"){
          bot.sendMessage(chat_id, "*Bakım Modu Açıldı!* 🟢", "Markdown");
          bakim=true;
        }
        if(bot.messages[i].text == "bakim_modunu_kapat"){
          bot.sendMessage(chat_id, "*Bakım Modu Kapandı!* 🔴", "Markdown");
          bakim=false;
        }
        if(bot.messages[i].text == "resetle"){
          bot.sendMessage(chat_id, "🔄 *Resetleniyor...*\nYeniden başlatılması ~10sn sürebilir.\nNodemcu çalıştığında loga bir bildiri mesajı gelecektir!", "Markdown");
          //ESP.reset();
        }
      }
    }
    // Log mesajını gruba gönderme
    bot.sendMessage("-588449967", "*" + from_name + ":* "+ text, "Markdown");
  }
}
//------------------

void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(ledPin, OUTPUT);
  delay(10);
  digitalWrite(ledPin, LOW);

  configTime(0, 0, "pool.ntp.org");
  secured_client.setTrustAnchors(&cert);
  Serial.print("Wifiye baglaniliyor... Baglanilan WiFi: ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFiye baglandi. IP adresi: ");
  Serial.println(WiFi.localIP());

  bot.sendMessage("-588449967", "🚀 *Nodemcu Çalışmaya Başladı!*", "Markdown");

  Serial.print("Baglanma zamani: ");
  time_t now = time(nullptr);
  
  while (now < 24 * 3600) {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);
}

void loop() {
  if (millis() - bot_lasttime > BOT_MTBS) {
    
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("Yanit aldi");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    bot_lasttime = millis();
  }
}
