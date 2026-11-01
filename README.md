# AlashMQTT

[Русский](#русский) | [Қазақша](#қазақша) | [English](#english)

---

## Русский

Простой **клиент MQTT** для умного дома и интернета вещей: `mqtt.publish("дом/температура", 23.5)`, `mqtt.subscribe("дом/свет")`. Сам **переподключается и подписывается заново**, если связь пропала, поддерживает логин, «завещание» (брокер сообщит, что датчик исчез) и понятно объясняет отказ сервера. Работает поверх `WiFiClient`, `WiFiClientSecure` или `EthernetClient`. Библиотека PubSubClient не нужна. Сделана для уроков и наборов Alash Electronics.

```cpp
#include <AlashMQTT.h>
#include <WiFi.h>
WiFiClient net;
AlashMQTT mqtt(net);

void setup() {
  WiFi.begin("СЕТЬ", "ПАРОЛЬ");
  while (WiFi.status() != WL_CONNECTED) delay(300);
  mqtt.setServer("broker.hivemq.com", 1883);
  mqtt.setClientId("alash-1234");
}
void loop() {
  mqtt.update();                                   // связь и переподключение / link and reconnect
  if (mqtt.connected()) mqtt.publish("alash/demo/temp", 23.5);
  delay(5000);
}
```

### Что умеет
- **Простая публикация:** строки, числа (`publish(тема, 23.5, 1)` отправит "23.5"), байты, флаг retain.
- **Подписки, которые не теряются:** `subscribe()` можно вызвать до подключения; после потери связи библиотека сама подключается и подписывается на всё заново.
- **`update()` в `loop()`:** читает сообщения (в том числе пришедшие по частям), держит связь пингами, переподключается с растущей паузой 2, 4, 8, ... 30 секунд.
- **«Завещание» и retain:** брокер сам опубликует `offline`, если плата пропала без предупреждения.
- **Понятные отказы:** по коду ответа сервера: неверный логин, имя клиента занято, нет доступа, порт закрыт; `diagnose(Serial)` пробует подключиться и объясняет.
- **Любая сеть:** поверх `WiFiClient`, `WiFiClientSecure` (TLS) или `EthernetClient`.
- **Проверено по стандарту:** байты пакетов сверены с примерами MQTT 3.1.1.
- Для **ESP32, ESP8266** и других плат с сетью.

### Чем отличается от других библиотек
Мы посмотрели PubSubClient (Nick O'Leary), AsyncMqttClient и ArduinoMqttClient:
- PubSubClient не переподключается сам: каждый проект пишет свою функцию `reconnect()` с `while (!client.connected())`, которая блокирует программу. У нас `update()`.
- После потери связи подписки приходится восстанавливать вручную; у нас автоматически.
- Коды ошибок `state()` это числа -4...5, их надо искать в документации; у нас фраза на трёх языках и что делать.
- Буфер PubSubClient по умолчанию 256 байт и молча отбрасывает длинные сообщения; у нас `ERR_TOO_BIG` и настройка.

### Примеры
`Basic_ESP`, `Subscribe_ESP`, `SensorToCloud_ESP`, `Diagnose_ESP`

### Тесты
Байты пакетов CONNECT (с логином, паролем и завещанием), PUBLISH, SUBSCRIBE, PINGREQ, DISCONNECT и кодирование длины сверены с примерами стандарта MQTT 3.1.1, коды отказа CONNACK, подписки до подключения и после переподключения, приём сообщений целиком, по байту, подряд, QoS 1 с подтверждением, пакет больше буфера, пинг и потеря связи, рост пауз переподключения проверены без сети на имитации брокера: `extras/test`. Тесты нашли и помогли исправить две ошибки (необнулённый список подписок, повтор подписки).

Подробное руководство: [docs/WIKI.md](docs/WIKI.md).

---

## Қазақша

Ақылды үй мен заттар интернетіне арналған қарапайым **MQTT клиенті**: `mqtt.publish("үй/температура", 23.5)`, `mqtt.subscribe("үй/жарық")`. Байланыс үзілсе **өзі қайта қосылып, қайта жазылады**, логинді, «өсиетті» қолдайды және сервер бас тартуын түсіндіреді. PubSubClient қажет емес.

```cpp
#include <AlashMQTT.h>
#include <WiFi.h>
WiFiClient net;
AlashMQTT mqtt(net);

void setup() {
  WiFi.begin("СЕТЬ", "ПАРОЛЬ");
  while (WiFi.status() != WL_CONNECTED) delay(300);
  mqtt.setServer("broker.hivemq.com", 1883);
  mqtt.setClientId("alash-1234");
}
void loop() {
  mqtt.update();                                   // связь и переподключение / link and reconnect
  if (mqtt.connected()) mqtt.publish("alash/demo/temp", 23.5);
  delay(5000);
}
```

### Мүмкіндіктері
- **Қарапайым жариялау:** жолдар, сандар, байттар, retain.
- **Жоғалмайтын жазылымдар:** `subscribe()` қосылудан бұрын шақырылады; байланыс үзілсе өзі қайта жазылады.
- **`update()` `loop()` ішінде:** хабарларды оқиды, пинг жібереді, 2, 4, 8, ... 30 секундпен қайта қосылады.
- **«Өсиет» және retain:** тақта хабарсыз жоғалса, брокер `offline` жариялайды.
- **Түсінікті бас тартулар:** логин, клиент аты, қолжетімділік, порт.
- **Кез келген желі:** `WiFiClient`, `WiFiClientSecure`, `EthernetClient`.
- **Стандарт бойынша тексерілген.**
- **ESP32, ESP8266**.

### Басқа кітапханалардан айырмашылығы
PubSubClient (Nick O'Leary), AsyncMqttClient және ArduinoMqttClient қарадық:
- PubSubClient өзі қайта қосылмайды; бізде `update()`.
- Байланыс үзілсе жазылымдарды қолмен қалпына келтіресің; бізде автоматты.
- `state()` кодтары сандар; бізде үш тілде сөйлем.
- PubSubClient ұзын хабарларды үнсіз тастайды; бізде `ERR_TOO_BIG`.

### Мысалдар
`Basic_ESP`, `Subscribe_ESP`, `SensorToCloud_ESP`, `Diagnose_ESP`

### Тесттер
CONNECT, PUBLISH, SUBSCRIBE байттары стандарт мысалдарымен салыстырылған; бас тарту кодтары, жазылымдар, хабарларды қабылдау, пинг, қайта қосылу тексерілген: `extras/test`.

Толық нұсқаулық: [docs/WIKI.md](docs/WIKI.md).

---

## English

A simple **MQTT client** for smart-home and IoT projects: `mqtt.publish("home/temperature", 23.5)`, `mqtt.subscribe("home/light")`. It **reconnects and re-subscribes** by itself when the link drops, supports a login, a "last will" (the broker announces that the sensor vanished) and explains server refusals. Works over `WiFiClient`, `WiFiClientSecure` or `EthernetClient`. No PubSubClient needed. Made for classes and Alash Electronics kits.

```cpp
#include <AlashMQTT.h>
#include <WiFi.h>
WiFiClient net;
AlashMQTT mqtt(net);

void setup() {
  WiFi.begin("СЕТЬ", "ПАРОЛЬ");
  while (WiFi.status() != WL_CONNECTED) delay(300);
  mqtt.setServer("broker.hivemq.com", 1883);
  mqtt.setClientId("alash-1234");
}
void loop() {
  mqtt.update();                                   // связь и переподключение / link and reconnect
  if (mqtt.connected()) mqtt.publish("alash/demo/temp", 23.5);
  delay(5000);
}
```

### Features
- **Easy publishing:** strings, numbers (`publish(topic, 23.5, 1)` sends "23.5"), bytes, the retain flag.
- **Subscriptions that are never lost:** call `subscribe()` before connecting; after a drop the library reconnects and re-subscribes to everything.
- **`update()` in `loop()`:** reads messages (even ones arriving in pieces), keeps the link alive with pings, reconnects with growing delays 2, 4, 8, ... 30 seconds.
- **Last will and retain:** the broker itself publishes `offline` if the board vanishes without warning.
- **Clear refusals:** from the server reply code: wrong login, client id taken, no access, blocked port; `diagnose(Serial)` tries to connect and explains.
- **Any network:** over `WiFiClient`, `WiFiClientSecure` (TLS) or `EthernetClient`.
- **Checked against the standard:** packet bytes match the MQTT 3.1.1 examples.
- For **ESP32, ESP8266** and other boards with a network.

### How it differs from other libraries
We looked at PubSubClient (Nick O'Leary), AsyncMqttClient and ArduinoMqttClient:
- PubSubClient does not reconnect by itself: every project writes its own `reconnect()` with a blocking `while (!client.connected())`. We have `update()`.
- After a drop you restore subscriptions by hand; here it is automatic.
- `state()` returns numbers -4...5 to look up in the docs; we give a sentence in three languages with what to do.
- PubSubClient's default buffer is 256 bytes and silently drops long messages; we give `ERR_TOO_BIG` and a setting.

### Examples
`Basic_ESP`, `Subscribe_ESP`, `SensorToCloud_ESP`, `Diagnose_ESP`

### Tests
The bytes of CONNECT (with login, password and will), PUBLISH, SUBSCRIBE, PINGREQ, DISCONNECT and the length encoding match the MQTT 3.1.1 standard examples; CONNACK refusal codes, subscriptions before connecting and after reconnecting, receiving whole, byte-by-byte and back-to-back messages, QoS 1 acknowledgement, a packet larger than the buffer, ping and link loss and the growth of reconnect delays are checked without a network on a broker simulator: `extras/test`. The tests found and helped fix two bugs (an uninitialized subscription list and a repeated subscribe).

Full guide: [docs/WIKI.md](docs/WIKI.md).

---

MIT License, Alash Electronics.
