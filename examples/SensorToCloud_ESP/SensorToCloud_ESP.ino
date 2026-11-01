// AlashMQTT: датчик в сеть. Температура раз в 10 секунд уходит на сервер, а "завещание" само сообщит остальным, что датчик пропал:
// если плата отключится без предупреждения (выключили питание, пропал Wi-Fi), брокер сам опубликует "offline" на тему статуса.
// Тему статуса и значения читайте в любом приложении MQTT (например, MQTT Dash, MyMQTT) или Home Assistant.
#include <AlashMQTT.h>
#if defined(ESP32)
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

WiFiClient net;
AlashMQTT mqtt(net);
uint32_t last = 0;

float readTemperature() {
  return 20.0 + (millis() % 5000) / 1000.0;   // здесь должна быть настоящая температура: AlashAHT, AlashDS18B20...
}

void setup() {
  Serial.begin(115200);
  WiFi.begin("ИМЯ_СЕТИ", "ПАРОЛЬ");
  while (WiFi.status() != WL_CONNECTED) delay(300);
  mqtt.setServer("broker.hivemq.com", 1883);
  mqtt.setClientId("alash-sensor-9012");
  mqtt.setWill("alash/demo/status", "offline", true);   // сообщение, которое опубликует брокер, если связь пропадёт
}

void loop() {
  static bool wasUp = false;
  mqtt.update();
  if (mqtt.connected() && !wasUp) {
    wasUp = true;
    mqtt.publish("alash/demo/status", "online", true);  // retain: новые подписчики сразу видят последний статус
  }
  if (!mqtt.connected()) wasUp = false;
  if (mqtt.connected() && millis() - last > 10000) {
    last = millis();
    mqtt.publish("alash/demo/temperature", readTemperature(), 1);
  }
}
