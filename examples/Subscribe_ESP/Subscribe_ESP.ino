// [ru] AlashMQTT: управление светодиодом сообщениями. Отправьте на тему alash/demo/led текст on или off (из браузера или приложения MQTT).
//      Подписка запоминается: после потери связи плата переподключается и подписывается заново сама.
// [kk] AlashMQTT: жарықдиодты хабарламалармен басқару. alash/demo/led тақырыбына on немесе off мәтінін жіберіңіз (браузерден немесе MQTT қолданбасынан).
//      Жазылым есте сақталады: байланыс үзілгенде тақта қайта қосылып, өзі қайта жазылады.
// [en] AlashMQTT: controlling an LED with messages. Send the text on or off to the topic alash/demo/led (from a browser or an MQTT app).
//      The subscription is remembered: after a lost connection the board reconnects and subscribes again by itself.
#include <AlashMQTT.h>
#if defined(ESP32)
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

WiFiClient net;
AlashMQTT mqtt(net);

void onMessage(const char *topic, const uint8_t *payload, unsigned int length) {
  Serial.print(topic);
  Serial.print(": ");
  for (unsigned int i = 0; i < length; i++) Serial.print((char)payload[i]);
  Serial.println();
  if (length >= 2 && payload[0] == 'o' && payload[1] == 'n') digitalWrite(LED_BUILTIN, HIGH);   // "on"
  if (length >= 3 && payload[0] == 'o' && payload[1] == 'f') digitalWrite(LED_BUILTIN, LOW);    // "off"
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  WiFi.begin("ИМЯ_СЕТИ", "ПАРОЛЬ");
  while (WiFi.status() != WL_CONNECTED) delay(300);
  mqtt.setServer("broker.hivemq.com", 1883);
  mqtt.setClientId("alash-led-5678");
  mqtt.onMessage(onMessage);
  mqtt.subscribe("alash/demo/led");           // можно до подключения
}

void loop() {
  mqtt.update();
}
