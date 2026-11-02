// [ru] AlashMQTT: самый простой пример. ESP32 / ESP8266 публикует число в открытый тестовый сервер MQTT раз в 5 секунд.
//      Посмотреть сообщения можно в браузере: http://www.hivemq.com/demos/websocket-client/ (подпишитесь на alash/demo/#).
//      Публичный сервер открыт всем: не отправляйте на него личные данные и придумайте свою тему вместо alash/demo.
// [kk] AlashMQTT: ең қарапайым мысал. ESP32 / ESP8266 ашық MQTT сынақ серверіне әр 5 секунд сайын сан жариялайды.
//      Хабарламаларды браузерде көруге болады: http://www.hivemq.com/demos/websocket-client/ (alash/demo/# тақырыбына жазылыңыз).
//      Жария сервер барлығына ашық: оған жеке деректер жібермеңіз және alash/demo орнына өз тақырыбыңызды ойлап табыңыз.
// [en] AlashMQTT: the simplest example. An ESP32 / ESP8266 publishes a number to an open MQTT test server every 5 seconds.
//      You can watch the messages in a browser: http://www.hivemq.com/demos/websocket-client/ (subscribe to alash/demo/#).
//      The public server is open to everyone: do not send personal data to it and invent your own topic instead of alash/demo.
#include <AlashMQTT.h>
#if defined(ESP32)
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

WiFiClient net;
AlashMQTT mqtt(net);
uint32_t last = 0;
long counter = 0;

void setup() {
  Serial.begin(115200);
  WiFi.begin("ИМЯ_СЕТИ", "ПАРОЛЬ");
  while (WiFi.status() != WL_CONNECTED) delay(300);
  mqtt.setServer("broker.hivemq.com", 1883);
  mqtt.setClientId("alash-demo-1234");        // у каждого устройства своё имя, иначе брокер выбросит одного из двух
}

void loop() {
  mqtt.update();                              // держит связь, сам переподключается
  if (mqtt.connected() && millis() - last > 5000) {
    last = millis();
    mqtt.publish("alash/demo/counter", ++counter);
    Serial.println(counter);
  } else if (!mqtt.connected() && mqtt.error() != AlashMQTT::ERR_OK) {
    static uint32_t lastMsg = 0;
    if (millis() - lastMsg > 5000) { lastMsg = millis(); Serial.println(mqtt.errorText()); }
  }
}
