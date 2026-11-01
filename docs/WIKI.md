# AlashMQTT: руководство / нұсқаулық / guide

[Русский](#русский) | [Қазақша](#қазақша) | [English](#english)

---

## Русский

Простой **клиент MQTT** для умного дома и интернета вещей: `mqtt.publish("дом/температура", 23.5)`, `mqtt.subscribe("дом/свет")`. Сам **переподключается и подписывается заново**, если связь пропала, поддерживает логин, «завещание» (брокер сообщит, что датчик исчез) и понятно объясняет отказ сервера. Работает поверх `WiFiClient`, `WiFiClientSecure` или `EthernetClient`. Библиотека PubSubClient не нужна. Сделана для уроков и наборов Alash Electronics.

### Установка
Arduino IDE: Скетч, Подключить библиотеку, Управлять библиотеками, найти **AlashMQTT** и нажать Установить. Или скачайте ZIP с GitHub: Скетч, Подключить библиотеку, Добавить .ZIP библиотеку.

### Платы
Arduino Uno / Nano / Mega (AVR), ESP32, ESP8266 и другие, где работает Arduino-ядро.

### Быстрый старт
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

### Справочник по методам
| Метод | Что делает |
|---|---|
| `AlashMQTT(client)` | Создать клиент поверх WiFiClient / EthernetClient |
| `setServer(host, port) / setClientId(id)` | Адрес брокера и имя клиента |
| `setCredentials(user, pass) / setWill(topic, msg, retain)` | Логин и «завещание» |
| `update()` | Вызывать в loop(): связь, приём, переподключение |
| `connected() / connect() / disconnect()` | Состояние и ручное управление |
| `publish(topic, message/number, retain)` | Отправить сообщение |
| `subscribe(topic) / unsubscribe(topic)` | Подписаться (запоминается) |
| `onMessage(callback)` | Функция для входящих сообщений |
| `errorText() / diagnose(Serial)` | Причина отказа / самопроверка |

### Если что-то не работает
**Не подключается (ERR_NO_SERVER или ERR_TIMEOUT)**

Проверьте, что Wi-Fi подключён, адрес и порт (1883 без шифрования, 8883 с TLS). В школьных и корпоративных сетях порт 1883 часто закрыт: раздайте интернет с телефона.

**Соединение то и дело обрывается**

Два устройства с одинаковым именем клиента выбрасывают друг друга. Задайте уникальное `setClientId()` каждой плате.

**Сообщения не приходят**

Тема подписки и тема публикации должны совпадать до символа (регистр важен). Публичный сервер открыт всем: придумайте длинную уникальную тему. Для шифрования используйте WiFiClientSecure и порт 8883.

### Самопроверка
В библиотеке есть `diagnose(Serial)`: она печатает, что видит плата, и подсказывает, что проверить. Язык подсказок: `setLanguage(LANG_RU / LANG_KK / LANG_EN)`.

### Примеры
`Basic_ESP`, `Subscribe_ESP`, `SensorToCloud_ESP`, `Diagnose_ESP`

---

## Қазақша

Ақылды үй мен заттар интернетіне арналған қарапайым **MQTT клиенті**: `mqtt.publish("үй/температура", 23.5)`, `mqtt.subscribe("үй/жарық")`. Байланыс үзілсе **өзі қайта қосылып, қайта жазылады**, логинді, «өсиетті» қолдайды және сервер бас тартуын түсіндіреді. PubSubClient қажет емес.

### Орнату
Arduino IDE: Sketch, Include Library, Manage Libraries, **AlashMQTT** табып, Install басыңыз. Немесе GitHub-тан ZIP жүктеп, Sketch, Include Library, Add .ZIP Library.

### Тақталар
Arduino Uno / Nano / Mega (AVR), ESP32, ESP8266 және Arduino ядросы бар басқалар.

### Жылдам бастау
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

### Әдістер анықтамасы
| Әдіс | Не істейді |
|---|---|
| `AlashMQTT(client)` | Клиент жасау |
| `setServer(host, port) / setClientId(id)` | Брокер мекенжайы мен клиент аты |
| `setCredentials(user, pass) / setWill(topic, msg, retain)` | Логин және «өсиет» |
| `update()` | loop() ішінде |
| `connected() / connect() / disconnect()` | Күй және қолмен басқару |
| `publish(topic, message/number, retain)` | Хабар жіберу |
| `subscribe(topic) / unsubscribe(topic)` | Жазылу |
| `onMessage(callback)` | Кіріс хабарлар функциясы |
| `errorText() / diagnose(Serial)` | Бас тарту себебі |

### Бірдеңе жұмыс істемесе
**Қосылмайды**

Wi-Fi, мекенжай және портты тексеріңіз. Мектеп желісінде 1883 порты жабық болуы мүмкін.

**Байланыс үзіліп тұрады**

Бірдей клиент аты бар екі құрылғы бір-бірін шығарып тастайды. Әр тақтаға бірегей `setClientId()` беріңіз.

**Хабарлар келмейді**

Тақырыптар таңбасына дейін сәйкес болуы керек.

### Өзін-өзі тексеру
Кітапханада `diagnose(Serial)` бар: тақта не көргенін басып шығарады және нені тексеру керегін айтады. Кеңес тілі: `setLanguage(LANG_RU / LANG_KK / LANG_EN)`.

### Мысалдар
`Basic_ESP`, `Subscribe_ESP`, `SensorToCloud_ESP`, `Diagnose_ESP`

---

## English

A simple **MQTT client** for smart-home and IoT projects: `mqtt.publish("home/temperature", 23.5)`, `mqtt.subscribe("home/light")`. It **reconnects and re-subscribes** by itself when the link drops, supports a login, a "last will" (the broker announces that the sensor vanished) and explains server refusals. Works over `WiFiClient`, `WiFiClientSecure` or `EthernetClient`. No PubSubClient needed. Made for classes and Alash Electronics kits.

### Installation
Arduino IDE: Sketch, Include Library, Manage Libraries, search **AlashMQTT** and click Install. Or download the ZIP from GitHub: Sketch, Include Library, Add .ZIP Library.

### Boards
Arduino Uno / Nano / Mega (AVR), ESP32, ESP8266 and others with an Arduino core.

### Quick start
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

### Method reference
| Method | What it does |
|---|---|
| `AlashMQTT(client)` | Create a client over WiFiClient / EthernetClient |
| `setServer(host, port) / setClientId(id)` | Broker address and client id |
| `setCredentials(user, pass) / setWill(topic, msg, retain)` | Login and last will |
| `update()` | Call in loop(): link, receive, reconnect |
| `connected() / connect() / disconnect()` | State and manual control |
| `publish(topic, message/number, retain)` | Send a message |
| `subscribe(topic) / unsubscribe(topic)` | Subscribe (remembered) |
| `onMessage(callback)` | Callback for incoming messages |
| `errorText() / diagnose(Serial)` | Refusal reason / self-check |

### If something does not work
**It does not connect (ERR_NO_SERVER or ERR_TIMEOUT)**

Check that Wi-Fi is connected and the address and port (1883 plain, 8883 TLS). School and company networks often block 1883: use a phone hotspot.

**The connection keeps dropping**

Two devices with the same client id kick each other out. Give every board a unique `setClientId()`.

**Messages do not arrive**

The subscribe topic and the publish topic must match to the character (case matters). A public server is open to everyone: invent a long unique topic. For encryption use WiFiClientSecure and port 8883.

### Self-check
The library has `diagnose(Serial)`: it prints what the board sees and tells what to check. Tips language: `setLanguage(LANG_RU / LANG_KK / LANG_EN)`.

### Examples
`Basic_ESP`, `Subscribe_ESP`, `SensorToCloud_ESP`, `Diagnose_ESP`

---

MIT License, Alash Electronics. https://github.com/Alash-electronics/AlashMQTT
