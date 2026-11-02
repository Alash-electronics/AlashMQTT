// [ru] AlashMQTT: не подключается к серверу? Скетч попробует подключиться и скажет причину (адрес, порт, логин, имя клиента).
// [kk] AlashMQTT: серверге қосылмай ма? Скетч қосылып көреді және себебін айтады (мекенжай, порт, логин, клиент аты).
// [en] AlashMQTT: cannot connect to the server? The sketch tries to connect and tells the reason (address, port, login, client name).
#include <AlashMQTT.h>
#if defined(ESP32)
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

WiFiClient net;
AlashMQTT mqtt(net);

void setup() {
  Serial.begin(115200);
  delay(500);
  WiFi.begin("ИМЯ_СЕТИ", "ПАРОЛЬ");
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(300);
  Serial.println(WiFi.status() == WL_CONNECTED ? "Wi-Fi подключён" : "Wi-Fi НЕ подключён: сначала исправьте его");
  mqtt.setLanguage(AlashMQTT::LANG_RU);       // LANG_KK: қазақша, LANG_EN: English
  mqtt.setServer("broker.hivemq.com", 1883);
  mqtt.setClientId("alash-check-3456");
  mqtt.diagnose(Serial);
}

void loop() {}
