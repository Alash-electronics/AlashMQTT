// AlashMQTT: клиент MQTT для умного дома и интернета вещей (ESP32, ESP8266 и любые платы с Client): публикация, подписка,
// автопереподключение с повторной подпиской, "завещание", понятные причины отказов. Без зависимостей. MIT License. Alash Electronics.
#ifndef ALASH_MQTT_H
#define ALASH_MQTT_H

#include <Arduino.h>
#include <Client.h>

// Размер буфера пакета (тема + сообщение + 5 байт). Можно задать до #include: #define ALASH_MQTT_BUFFER 1024
#ifndef ALASH_MQTT_BUFFER
#if defined(ARDUINO_ARCH_AVR)
#define ALASH_MQTT_BUFFER 128
#else
#define ALASH_MQTT_BUFFER 512
#endif
#endif
#ifndef ALASH_MQTT_MAX_SUBS
#define ALASH_MQTT_MAX_SUBS 6
#endif
#ifndef ALASH_MQTT_TOPIC_LEN
#define ALASH_MQTT_TOPIC_LEN 64
#endif

class AlashMQTT {
 public:
  typedef void (*Callback)(const char *topic, const uint8_t *payload, unsigned int length);
  enum Error : uint8_t {
    ERR_OK = 0,
    ERR_NO_SERVER,         // не удалось соединиться с адресом брокера (нет интернета, неверный адрес или порт)
    ERR_BAD_PROTOCOL,      // брокер не поддерживает MQTT 3.1.1
    ERR_ID_REJECTED,       // брокер отклонил имя клиента (setClientId)
    ERR_BROKER_DOWN,       // сервер MQTT недоступен
    ERR_BAD_CREDENTIALS,   // неверные логин или пароль
    ERR_NOT_AUTHORIZED,    // у этого пользователя нет доступа
    ERR_TIMEOUT,           // брокер не ответил на подключение
    ERR_TOO_BIG,           // тема и сообщение не помещаются в буфер (ALASH_MQTT_BUFFER)
    ERR_NOT_CONNECTED,     // публикация без подключения
    ERR_LOST                // связь с брокером пропала
  };
  enum Language : uint8_t { LANG_RU = 0, LANG_KK = 1, LANG_EN = 2 };

  explicit AlashMQTT(Client &client) : _c(&client) {}

  // ---- Настройка (до первого update / connect) ------------------------------------------------------
  void setServer(const char *host, uint16_t port = 1883) { _host = host; _port = port; }
  void setClientId(const char *id) { strncpy(_id, id, sizeof(_id) - 1); _id[sizeof(_id) - 1] = 0; }
  void setCredentials(const char *user, const char *pass) { _user = user; _pass = pass; }
  void setWill(const char *topic, const char *message, bool retain = true) { _willTopic = topic; _willMsg = message; _willRetain = retain; }
  void setKeepAlive(uint16_t seconds) { _keepAlive = seconds ? seconds : 1; }
  void onMessage(Callback cb) { _cb = cb; }

  // ---- Работа ------------------------------------------------------------------------------------
  bool connect(uint16_t timeoutMs = 5000);        // подключиться сейчас (блокирует до timeoutMs)
  void disconnect();
  bool connected();
  // update() в loop(): читает сообщения, держит связь, при потере сам переподключается (паузы 2, 4, 8, ... 30 секунд) и подписывается заново
  void update();

  bool subscribe(const char *topic);              // запоминается и восстанавливается после переподключения
  bool unsubscribe(const char *topic);
  bool publish(const char *topic, const char *message, bool retain = false);
  bool publish(const char *topic, const uint8_t *payload, unsigned int length, bool retain = false);
  bool publish(const char *topic, float value, uint8_t decimals = 2, bool retain = false);   // число текстом, например "23.50"
  bool publish(const char *topic, long value, bool retain = false);

  // ---- Диагностика -----------------------------------------------------------------
  Error error() const { return _err; }
  void setLanguage(Language lang) { _lang = lang; }
  const __FlashStringHelper *errorText();
  void diagnose(Print &out);

  // Кодирование пакетов (static: проверяются тестами по примерам из стандарта MQTT 3.1.1)
  static unsigned int encodeLength(uint8_t *out, unsigned int len);                 // "остаток длины": 1-4 байта
  static unsigned int encodePublish(uint8_t *buf, size_t size, const char *topic, const uint8_t *payload, unsigned int len, bool retain);   // 0 если не помещается

 private:
  Client *_c;
  const char *_host = nullptr, *_user = nullptr, *_pass = nullptr, *_willTopic = nullptr, *_willMsg = nullptr;
  uint16_t _port = 1883, _keepAlive = 30;
  char _id[32] = "alash-mqtt";
  bool _willRetain = true, _up = false;
  Callback _cb = nullptr;
  Error _err = ERR_OK;
  Language _lang = LANG_RU;
  uint8_t _buf[ALASH_MQTT_BUFFER] = {0};
  char _subs[ALASH_MQTT_MAX_SUBS][ALASH_MQTT_TOPIC_LEN] = {{0}};     // обязательно обнулены: пустая строка = свободное место
  uint16_t _nextId = 1;
  uint32_t _lastIn = 0, _lastOut = 0, _nextTry = 0, _backoff = 2000;
  bool _pingSent = false;
  unsigned int _have = 0, _need = 0, _skip = 0;   // сборка входящего пакета
  bool _needKnown = false;
  bool sendRaw(const uint8_t *data, unsigned int len);
  bool sendSubscribe(const char *topic);
  bool readPacket();
  void handlePacket(uint8_t header, uint8_t *body, unsigned int len);
  void fail(Error e);
};

#endif  // ALASH_MQTT_H
