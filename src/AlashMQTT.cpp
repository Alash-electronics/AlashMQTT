// MIT License. Alash Electronics.
#include "AlashMQTT.h"

#define T(ru, kk, en) (_lang == LANG_KK ? F(kk) : _lang == LANG_EN ? F(en) : F(ru))

// Типы пакетов MQTT 3.1.1 (старшие 4 бита первого байта)
static const uint8_t T_CONNECT = 0x10, T_CONNACK = 0x20, T_PUBLISH = 0x30, T_PUBACK = 0x40, T_SUBSCRIBE = 0x82,
                     T_UNSUBSCRIBE = 0xA2, T_PINGREQ = 0xC0, T_PINGRESP = 0xD0, T_DISCONNECT = 0xE0;

// "Оставшаяся длина": по 7 бит в байте, старший бит говорит, что дальше есть ещё байт
unsigned int AlashMQTT::encodeLength(uint8_t *out, unsigned int len) {
  unsigned int n = 0;
  do {
    uint8_t b = len & 0x7F;
    len >>= 7;
    if (len) b |= 0x80;
    out[n++] = b;
  } while (len && n < 4);
  return n;
}

unsigned int AlashMQTT::encodePublish(uint8_t *buf, size_t size, const char *topic, const uint8_t *payload, unsigned int len, bool retain) {
  unsigned int tl = strlen(topic);
  unsigned int rem = 2 + tl + len;
  uint8_t lenBytes[4];
  unsigned int ln = encodeLength(lenBytes, rem);
  if (1 + ln + rem > size) return 0;
  unsigned int p = 0;
  buf[p++] = T_PUBLISH | (retain ? 1 : 0);
  for (unsigned int i = 0; i < ln; i++) buf[p++] = lenBytes[i];
  buf[p++] = tl >> 8; buf[p++] = tl & 0xFF;
  memcpy(buf + p, topic, tl); p += tl;
  if (len) { memcpy(buf + p, payload, len); p += len; }
  return p;
}

bool AlashMQTT::sendRaw(const uint8_t *data, unsigned int len) {
  if (_c->write(data, len) != len) {
    fail(ERR_LOST);
    return false;
  }
  _lastOut = millis();
  return true;
}

void AlashMQTT::fail(Error e) {
  _err = e;
  _up = false;
  _c->stop();
  _pingSent = false;
}

// ---- Подключение --------------------------------------------------------------------------------

bool AlashMQTT::connect(uint16_t timeoutMs) {
  if (!_host) {
    _err = ERR_NO_SERVER;
    return false;
  }
  _c->stop();
  _up = false;
  if (!_c->connect(_host, _port)) {
    _err = ERR_NO_SERVER;
    return false;
  }
  // CONNECT: протокол "MQTT" 4, флаги, пауза связи, потом поля по порядку
  uint8_t flags = 0x02;                                   // чистая сессия
  unsigned int idl = strlen(_id), ul = _user ? strlen(_user) : 0, pl = _pass ? strlen(_pass) : 0;
  unsigned int wtl = _willTopic ? strlen(_willTopic) : 0, wml = _willMsg ? strlen(_willMsg) : 0;
  if (_willTopic) flags |= 0x04 | (_willRetain ? 0x20 : 0);
  if (_user) flags |= 0x80;
  if (_pass) flags |= 0x40;
  unsigned int rem = 10 + 2 + idl + (_willTopic ? 2 + wtl + 2 + wml : 0) + (_user ? 2 + ul : 0) + (_pass ? 2 + pl : 0);
  uint8_t lb[4];
  unsigned int ln = encodeLength(lb, rem);
  if (1 + ln + rem > sizeof(_buf)) {
    _c->stop();
    _err = ERR_TOO_BIG;
    return false;
  }
  unsigned int p = 0;
  _buf[p++] = T_CONNECT;
  for (unsigned int i = 0; i < ln; i++) _buf[p++] = lb[i];
  static const uint8_t proto[] = {0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04};
  memcpy(_buf + p, proto, sizeof proto); p += sizeof proto;
  _buf[p++] = flags;
  _buf[p++] = _keepAlive >> 8; _buf[p++] = _keepAlive & 0xFF;
  auto putStr = [&](const char *s, unsigned int l) { _buf[p++] = l >> 8; _buf[p++] = l & 0xFF; memcpy(_buf + p, s, l); p += l; };
  putStr(_id, idl);
  if (_willTopic) { putStr(_willTopic, wtl); putStr(_willMsg ? _willMsg : "", wml); }
  if (_user) putStr(_user, ul);
  if (_pass) putStr(_pass, pl);
  if (!sendRaw(_buf, p)) return false;

  // Ждём CONNACK (4 байта: 20 02 флаги код)
  uint32_t t0 = millis();
  while ((uint32_t)(millis() - t0) < timeoutMs) {
    if (_c->available() >= 4) {
      uint8_t ack[4];
      for (uint8_t i = 0; i < 4; i++) ack[i] = (uint8_t)_c->read();
      if (ack[0] != T_CONNACK || ack[1] != 2) { fail(ERR_BAD_PROTOCOL); return false; }
      switch (ack[3]) {
        case 0: break;
        case 1: fail(ERR_BAD_PROTOCOL); return false;
        case 2: fail(ERR_ID_REJECTED); return false;
        case 3: fail(ERR_BROKER_DOWN); return false;
        case 4: fail(ERR_BAD_CREDENTIALS); return false;
        default: fail(ERR_NOT_AUTHORIZED); return false;
      }
      _up = true;
      _err = ERR_OK;
      _lastIn = _lastOut = millis();
      _backoff = 2000;
      _have = 0;
      for (uint8_t i = 0; i < ALASH_MQTT_MAX_SUBS; i++)    // после переподключения подписки восстанавливаются сами
        if (_subs[i][0]) sendSubscribe(_subs[i]);
      return _up;
    }
    if (!_c->connected()) { fail(ERR_LOST); return false; }
    delay(5);
  }
  fail(ERR_TIMEOUT);
  return false;
}

void AlashMQTT::disconnect() {
  if (_up) {
    uint8_t d[2] = {T_DISCONNECT, 0};
    _c->write(d, 2);
  }
  _c->stop();
  _up = false;
}

bool AlashMQTT::connected() {
  if (_up && !_c->connected()) fail(ERR_LOST);
  return _up;
}

// ---- Подписка и публикация --------------------------------------------------------------------------

bool AlashMQTT::sendSubscribe(const char *topic) {
  unsigned int tl = strlen(topic);
  unsigned int rem = 2 + 2 + tl + 1;
  uint8_t lb[4];
  unsigned int ln = encodeLength(lb, rem);
  if (1 + ln + rem > sizeof(_buf)) { _err = ERR_TOO_BIG; return false; }
  unsigned int p = 0;
  _buf[p++] = T_SUBSCRIBE;
  for (unsigned int i = 0; i < ln; i++) _buf[p++] = lb[i];
  uint16_t id = _nextId++; if (!_nextId) _nextId = 1;
  _buf[p++] = id >> 8; _buf[p++] = id & 0xFF;
  _buf[p++] = tl >> 8; _buf[p++] = tl & 0xFF;
  memcpy(_buf + p, topic, tl); p += tl;
  _buf[p++] = 0;                                           // QoS 0
  return sendRaw(_buf, p);
}

bool AlashMQTT::subscribe(const char *topic) {
  if (!topic || strlen(topic) >= ALASH_MQTT_TOPIC_LEN) { _err = ERR_TOO_BIG; return false; }
  int free = -1;
  for (uint8_t i = 0; i < ALASH_MQTT_MAX_SUBS; i++) {
    if (!strcmp(_subs[i], topic)) { free = -2; break; }     // уже есть
    if (!_subs[i][0] && free == -1) free = i;
  }
  if (free == -2) return true;                              // уже подписаны: повторно не отправляем
  if (free == -1) { _err = ERR_TOO_BIG; return false; }     // список подписок полон
  strcpy(_subs[free], topic);
  return _up ? sendSubscribe(topic) : true;                 // без связи подпишется при подключении
}

bool AlashMQTT::unsubscribe(const char *topic) {
  for (uint8_t i = 0; i < ALASH_MQTT_MAX_SUBS; i++)
    if (!strcmp(_subs[i], topic)) _subs[i][0] = 0;
  if (!_up) return true;
  unsigned int tl = strlen(topic), rem = 2 + 2 + tl;
  uint8_t lb[4];
  unsigned int ln = encodeLength(lb, rem), p = 0;
  if (1 + ln + rem > sizeof(_buf)) return false;
  _buf[p++] = T_UNSUBSCRIBE;
  for (unsigned int i = 0; i < ln; i++) _buf[p++] = lb[i];
  uint16_t id = _nextId++; if (!_nextId) _nextId = 1;
  _buf[p++] = id >> 8; _buf[p++] = id & 0xFF;
  _buf[p++] = tl >> 8; _buf[p++] = tl & 0xFF;
  memcpy(_buf + p, topic, tl); p += tl;
  return sendRaw(_buf, p);
}

bool AlashMQTT::publish(const char *topic, const uint8_t *payload, unsigned int length, bool retain) {
  if (!connected()) { _err = ERR_NOT_CONNECTED; return false; }
  unsigned int n = encodePublish(_buf, sizeof(_buf), topic, payload, length, retain);
  if (!n) { _err = ERR_TOO_BIG; return false; }
  return sendRaw(_buf, n);
}

bool AlashMQTT::publish(const char *topic, const char *message, bool retain) {
  return publish(topic, (const uint8_t *)message, strlen(message), retain);
}

bool AlashMQTT::publish(const char *topic, float value, uint8_t decimals, bool retain) {
  char b[24];
  if (isnan(value)) return publish(topic, "nan", retain);
  if (decimals > 6) decimals = 6;
  // свой вывод числа: dtostrf нет на всех платах
  bool neg = value < 0;
  float a = neg ? -value : value;
  unsigned long scale = 1;
  for (uint8_t i = 0; i < decimals; i++) scale *= 10;
  unsigned long whole = (unsigned long)a, frac = (unsigned long)((a - whole) * scale + 0.5f);
  if (frac >= scale) { whole++; frac -= scale; }
  if (decimals) snprintf(b, sizeof b, "%s%lu.%0*lu", neg ? "-" : "", whole, decimals, frac);
  else snprintf(b, sizeof b, "%s%lu", neg ? "-" : "", whole);
  return publish(topic, b, retain);
}

bool AlashMQTT::publish(const char *topic, long value, bool retain) {
  char b[16];
  snprintf(b, sizeof b, "%ld", value);
  return publish(topic, b, retain);
}

// ---- Чтение и поддержание связи ------------------------------------------------------------------------------

// Собирает входящий пакет по частям, не блокируя программу. true, когда пакет получен целиком и разобран.
bool AlashMQTT::readPacket() {
  while (_c->available()) {
    uint8_t b = (uint8_t)_c->read();
    if (_skip) {                                            // пропускаем хвост пакета, который не поместился в буфер
      if (--_skip == 0) { _have = 0; _needKnown = false; }
      continue;
    }
    _buf[_have++] = b;
    if (!_needKnown) {
      if (_have >= 2 && !(b & 0x80)) {                      // пришёл последний байт длины: знаем размер пакета
        unsigned int rem = 0, mul = 1;
        for (unsigned int i = 1; i < _have; i++) { rem += (_buf[i] & 0x7F) * mul; mul *= 128; }
        _need = _have + rem;
        _needKnown = true;
        if (_need > sizeof(_buf)) {                         // слишком большой: пропустим остаток, сообщим ошибку
          _err = ERR_TOO_BIG;
          _skip = rem;
          if (_skip == 0) { _have = 0; _needKnown = false; }
          continue;
        }
      } else if (_have >= 5) {                              // длина не может занимать больше 4 байт: поток испорчен
        _have = 0; _needKnown = false;
        fail(ERR_LOST);
        return false;
      }
    }
    if (_needKnown && _have >= _need) {
      unsigned int hdr = 1;
      while (hdr < _have && (_buf[hdr] & 0x80)) hdr++;
      hdr++;                                                // байт типа + байты длины
      handlePacket(_buf[0], _buf + hdr, _have - hdr);
      _have = 0; _needKnown = false;
      _lastIn = millis();
      return true;
    }
  }
  return false;
}

void AlashMQTT::handlePacket(uint8_t h, uint8_t *body, unsigned int len) {
  uint8_t type = h & 0xF0;
  if (type == T_PUBLISH) {
    uint8_t qos = (h >> 1) & 3;
    if (len < 2) return;
    unsigned int tl = (body[0] << 8) | body[1];
    if (2 + tl > len) return;
    unsigned int p = 2 + tl;
    uint16_t id = 0;
    if (qos > 0) { if (p + 2 > len) return; id = (body[p] << 8) | body[p + 1]; p += 2; }
    // тема обрывается нулём прямо в буфере: сдвигаем на байт влево, чтобы поставить "\0" (перед темой два байта длины)
    char *topic = (char *)body + 1;
    memmove(topic, body + 2, tl);
    topic[tl] = 0;
    const uint8_t *payload = body + p;
    unsigned int plen = len - p;
    if (_cb) _cb(topic, payload, plen);
    if (qos == 1) {                                         // подтверждаем QoS 1, чтобы брокер не слал повторно
      uint8_t ack[4] = {T_PUBACK, 2, (uint8_t)(id >> 8), (uint8_t)(id & 0xFF)};
      sendRaw(ack, 4);
    }
  } else if (type == T_PINGRESP) {
    _pingSent = false;
  }
}

void AlashMQTT::update() {
  uint32_t now = millis();
  if (!connected()) {
    if (_host && (int32_t)(now - _nextTry) >= 0) {
      if (!connect(3000)) {
        _nextTry = millis() + _backoff;
        _backoff = _backoff < 30000 ? _backoff * 2 : 30000;   // 2, 4, 8, 16, 30 секунд
      }
    }
    return;
  }
  while (readPacket()) {}
  if (!_up) return;
  // Поддержание связи: PINGREQ на половине периода, нет ответа дольше 1.5 периода: связь потеряна
  uint32_t ka = (uint32_t)_keepAlive * 1000UL;
  if (!_pingSent && (uint32_t)(now - _lastOut) >= ka / 2) {
    uint8_t ping[2] = {T_PINGREQ, 0};
    if (sendRaw(ping, 2)) _pingSent = true;
  }
  if ((uint32_t)(now - _lastIn) > ka * 3 / 2) fail(ERR_LOST);
}

// ---- Диагностика ---------------------------------------------------------------------------------------------

const __FlashStringHelper *AlashMQTT::errorText() {
  switch (_err) {
    case ERR_OK: return F("OK");
    case ERR_NO_SERVER:
      return T("Не удалось соединиться с брокером: проверьте интернет (Wi-Fi подключён?), адрес и порт (обычно 1883, для TLS 8883) в setServer()",
               "Брокерге қосыла алмады: интернетті (Wi-Fi қосулы ма?), setServer() ішіндегі мекенжай мен портты (әдетте 1883, TLS үшін 8883) тексеріңіз",
               "Could not reach the broker: check the internet (is Wi-Fi connected?) and the address and port in setServer() (usually 1883, 8883 for TLS)");
    case ERR_BAD_PROTOCOL:
      return T("Брокер не поддерживает MQTT 3.1.1: используйте другой сервер", "Брокер MQTT 3.1.1 қолдамайды: басқа сервер қолданыңыз", "The broker does not support MQTT 3.1.1: use another server");
    case ERR_ID_REJECTED:
      return T("Брокер отклонил имя клиента: задайте другое через setClientId() (оно должно быть уникальным и без пробелов)",
               "Брокер клиент атын қабылдамады: setClientId() арқылы басқасын беріңіз (бірегей, бос орынсыз)",
               "The broker rejected the client id: set another with setClientId() (unique, no spaces)");
    case ERR_BROKER_DOWN:
      return T("Сервер MQTT сейчас недоступен: попробуйте позже", "MQTT сервері қазір қолжетімсіз: кейінірек көріңіз", "The MQTT server is unavailable: try again later");
    case ERR_BAD_CREDENTIALS:
      return T("Неверный логин или пароль: проверьте setCredentials()", "Логин не құпиясөз қате: setCredentials() тексеріңіз", "Wrong username or password: check setCredentials()");
    case ERR_NOT_AUTHORIZED:
      return T("У этого пользователя нет доступа к брокеру или теме", "Бұл пайдаланушыда брокерге не тақырыпқа қолжетімділік жоқ", "This user has no access to the broker or the topic");
    case ERR_TIMEOUT:
      return T("Брокер не ответил на подключение за отведённое время: он перегружен, либо порт закрыт сетью (школьные сети часто закрывают 1883)",
               "Брокер қосылуға белгіленген уақытта жауап бермеді: ол жүктелген не желі портты жапқан (мектеп желілері 1883-ті жиі жабады)",
               "The broker did not answer in time: it is overloaded or the port is blocked by the network (school networks often block 1883)");
    case ERR_TOO_BIG:
      return T("Тема и сообщение не помещаются в буфер: короче сообщение или #define ALASH_MQTT_BUFFER 1024 до #include",
               "Тақырып пен хабар буферге сыймайды: қысқартыңыз не #include алдында #define ALASH_MQTT_BUFFER 1024",
               "The topic and message do not fit the buffer: shorten it or #define ALASH_MQTT_BUFFER 1024 before #include");
    case ERR_NOT_CONNECTED:
      return T("Публикация без подключения: вызывайте update() в loop() и публикуйте, когда connected() вернёт true",
               "Қосылусыз жариялау: loop() ішінде update() шақырыңыз, connected() true болғанда жариялаңыз",
               "Publishing while not connected: call update() in loop() and publish when connected() returns true");
    case ERR_LOST:
      return T("Связь с брокером пропала: библиотека переподключится сама", "Брокермен байланыс үзілді: кітапхана өзі қайта қосылады", "The link to the broker was lost: the library will reconnect by itself");
  }
  return F("");
}

void AlashMQTT::diagnose(Print &out) {
  out.println(T("AlashMQTT: диагностика", "AlashMQTT: диагностика", "AlashMQTT: diagnostics"));
  out.print(T("Брокер: ", "Брокер: ", "Broker: "));
  out.print(_host ? _host : "-");
  out.print(F(":"));
  out.println((int)_port);
  out.print(T("Имя клиента: ", "Клиент аты: ", "Client id: "));
  out.println(_id);
  out.println(T("Подключаюсь...", "Қосылып жатырмын...", "Connecting..."));
  if (connect(5000)) {
    out.println(T("Подключено. Публикую и подписываюсь можно.", "Қосылды. Жариялап, жазылуға болады.", "Connected. You can publish and subscribe."));
    disconnect();
  } else {
    out.println(errorText());
  }
  out.println(T("Для проверки без своего сервера: broker.hivemq.com, порт 1883 (публичный, не отправляйте личное).",
                "Өз серверіңізсіз тексеру үшін: broker.hivemq.com, порт 1883 (ашық, жеке деректерді жібермеңіз).",
                "To test without your own server: broker.hivemq.com, port 1883 (public, do not send private data)."));
}
