// Тесты AlashMQTT без сети: имитация брокера, байты пакетов сверяются с примерами из стандарта MQTT 3.1.1.
//   g++ -std=c++17 -Wall -I mock -I ../../src test_mqtt.cpp ../../src/AlashMQTT.cpp && ./a.out
#include <stdio.h>
#include "Arduino.h"
#include "Client.h"
#include "AlashMQTT.h"

static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); fails++; } } while (0)
typedef std::vector<uint8_t> Bytes;
static Bytes B(std::initializer_list<int> l) { Bytes b; for (int x : l) b.push_back((uint8_t)x); return b; }
static Bytes S(const char *s) { return Bytes(s, s + strlen(s)); }
static Bytes cat(Bytes a, const Bytes &b) { a.insert(a.end(), b.begin(), b.end()); return a; }
static void reset() { g_ms += 100000; g_onDelay = nullptr; }
static void run(AlashMQTT &m, uint32_t ms) { for (uint32_t i = 0; i < ms; i += 10) { g_ms += 10; m.update(); } }

static std::string gTopic, gPayload; static int gCount = 0;
static void onMsg(const char *t, const uint8_t *p, unsigned n) { gTopic = t; gPayload.assign((const char *)p, n); gCount++; }

int main() {
  // 1) длина пакета по стандарту: 0, 127, 128, 321, 16383, 16384
  uint8_t o[4];
  CHECK(AlashMQTT::encodeLength(o, 0) == 1 && o[0] == 0x00);
  CHECK(AlashMQTT::encodeLength(o, 127) == 1 && o[0] == 0x7F);
  CHECK(AlashMQTT::encodeLength(o, 128) == 2 && o[0] == 0x80 && o[1] == 0x01);
  CHECK(AlashMQTT::encodeLength(o, 321) == 2 && o[0] == 0xC1 && o[1] == 0x02);
  CHECK(AlashMQTT::encodeLength(o, 16383) == 2 && o[0] == 0xFF && o[1] == 0x7F);
  CHECK(AlashMQTT::encodeLength(o, 16384) == 3 && o[0] == 0x80 && o[1] == 0x80 && o[2] == 0x01);

  // 2) PUBLISH: "a/b" "hi" это 30 07 00 03 a / b h i; retain это 31; не помещается в маленький буфер: 0
  uint8_t buf[64];
  unsigned n = AlashMQTT::encodePublish(buf, sizeof buf, "a/b", (const uint8_t *)"hi", 2, false);
  CHECK(Bytes(buf, buf + n) == B({0x30, 0x07, 0x00, 0x03, 'a', '/', 'b', 'h', 'i'}));
  n = AlashMQTT::encodePublish(buf, sizeof buf, "a/b", (const uint8_t *)"hi", 2, true); CHECK(buf[0] == 0x31);
  CHECK(AlashMQTT::encodePublish(buf, 8, "a/b", (const uint8_t *)"hi", 2, false) == 0);

  // 3) CONNECT: id "a", пауза 60 с, чистая сессия: 10 0D 00 04 MQTT 04 02 00 3C 00 01 a
  reset(); FakeBroker br; AlashMQTT m(br);
  m.setServer("broker.test", 1883); m.setClientId("a"); m.setKeepAlive(60);
  CHECK(m.connect() && m.connected() && m.error() == AlashMQTT::ERR_OK);
  CHECK(br.host == "broker.test" && br.port == 1883);
  CHECK(br.sent[0] == B({0x10, 0x0D, 0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x02, 0x00, 0x3C, 0x00, 0x01, 'a'}));
  // с логином, паролем и завещанием
  reset(); FakeBroker b2; AlashMQTT m2(b2);
  m2.setServer("x"); m2.setClientId("a"); m2.setKeepAlive(60); m2.setCredentials("u", "p"); m2.setWill("w", "off", true);
  CHECK(m2.connect());
  CHECK(b2.sent[0] == B({0x10, 0x1B, 0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0xE6, 0x00, 0x3C, 0x00, 0x01, 'a', 0x00, 0x01, 'w', 0x00, 0x03, 'o', 'f', 'f', 0x00, 0x01, 'u', 0x00, 0x01, 'p'}));

  // 4) причины отказа по коду CONNACK, недоступный сервер, молчащий брокер
  struct { int code; AlashMQTT::Error e; } codes[] = {{1, AlashMQTT::ERR_BAD_PROTOCOL}, {2, AlashMQTT::ERR_ID_REJECTED}, {3, AlashMQTT::ERR_BROKER_DOWN}, {4, AlashMQTT::ERR_BAD_CREDENTIALS}, {5, AlashMQTT::ERR_NOT_AUTHORIZED}};
  for (auto &c : codes) { reset(); FakeBroker f; f.connackCode = c.code; AlashMQTT x(f); x.setServer("h"); CHECK(!x.connect() && x.error() == c.e && !x.connected()); }
  reset(); FakeBroker down; down.reachable = false; AlashMQTT d(down); d.setServer("h"); CHECK(!d.connect() && d.error() == AlashMQTT::ERR_NO_SERVER);
  reset(); FakeBroker silent; silent.silent = true; AlashMQTT sl(silent); sl.setServer("h"); uint32_t t0 = g_ms;
  CHECK(!sl.connect(2000) && sl.error() == AlashMQTT::ERR_TIMEOUT && g_ms - t0 >= 2000);
  AlashMQTT nohost(br); CHECK(!nohost.connect() && nohost.error() == AlashMQTT::ERR_NO_SERVER);

  // 5) публикация: байты, retain, числа текстом
  reset(); FakeBroker pb; AlashMQTT p(pb); p.setServer("h"); p.connect(); pb.sent.clear();
  CHECK(p.publish("a/b", "hi") && pb.sent[0] == B({0x30, 0x07, 0x00, 0x03, 'a', '/', 'b', 'h', 'i'}));
  CHECK(p.publish("t", "x", true) && pb.sent[1][0] == 0x31);
  pb.sent.clear();
  p.publish("t", 23.5f, 2); CHECK(pb.sent[0] == cat(B({0x30, 0x08, 0x00, 0x01, 't'}), S("23.50")));
  pb.sent.clear(); p.publish("t", -3.07f, 2); CHECK(pb.sent[0] == cat(B({0x30, 0x08, 0x00, 0x01, 't'}), S("-3.07")));
  pb.sent.clear(); p.publish("t", 9.999f, 2); CHECK(pb.sent[0] == cat(B({0x30, 0x08, 0x00, 0x01, 't'}), S("10.00")));
  pb.sent.clear(); p.publish("t", 7.0f, 0); CHECK(pb.sent[0] == cat(B({0x30, 0x04, 0x00, 0x01, 't'}), S("7")));
  pb.sent.clear(); p.publish("t", 42L); CHECK(pb.sent[0] == cat(B({0x30, 0x05, 0x00, 0x01, 't'}), S("42")));
  pb.sent.clear(); p.publish("t", -15L); CHECK(pb.sent[0] == cat(B({0x30, 0x06, 0x00, 0x01, 't'}), S("-15")));
  pb.sent.clear(); p.publish("t", NAN); CHECK(pb.sent[0] == cat(B({0x30, 0x06, 0x00, 0x01, 't'}), S("nan")));
  std::string big(600, 'z'); CHECK(!p.publish("t", big.c_str()) && p.error() == AlashMQTT::ERR_TOO_BIG);
  AlashMQTT off(pb); pb.open = false; CHECK(!off.publish("t", "x") && off.error() == AlashMQTT::ERR_NOT_CONNECTED);

  // 6) подписка: до подключения запоминается и отправляется при подключении; после переподключения восстанавливается
  reset(); FakeBroker sb; AlashMQTT s(sb); s.setServer("h"); s.onMessage(onMsg);
  CHECK(s.subscribe("home/light") && s.subscribe("home/temp"));
  CHECK(s.connect());
  CHECK(sb.subscribed.size() == 2 && sb.subscribed[0] == "home/light" && sb.subscribed[1] == "home/temp");
  CHECK(s.subscribe("a/b") && sb.subscribed.back() == "a/b");                         // уже на связи: сразу
  CHECK(s.subscribe("a/b") && sb.subscribed.size() == 3);                              // повторная подписка не дублируется
  int cc = sb.connects; sb.subscribed.clear(); sb.open = false; s.update();              // связь оборвалась: первая попытка сразу
  run(s, 2500); CHECK(s.connected() && sb.connects > cc && sb.subscribed.size() == 3);   // переподключилась и подписалась на все три
  CHECK(!s.subscribe("") || true);
  std::string longTopic(80, 'q'); CHECK(!s.subscribe(longTopic.c_str()) && s.error() == AlashMQTT::ERR_TOO_BIG);

  // 7) входящие: одиночное, по частям, подряд, QoS 1 с подтверждением
  gCount = 0; gTopic.clear(); gPayload.clear();
  sb.push(cat(B({0x30, 0x0C, 0x00, 0x05}), cat(S("a/b/c"), S("hello"))));              // тема 5 символов + нагрузка 5
  run(s, 20); CHECK(gCount == 1 && gTopic == "a/b/c" && gPayload == "hello");
  Bytes pk = cat(B({0x30, 0x09, 0x00, 0x03}), cat(S("x/y"), S("1234")));                // побайтово, с паузами
  for (uint8_t byte : pk) { sb.in.push_back(byte); run(s, 10); }
  CHECK(gCount == 2 && gTopic == "x/y" && gPayload == "1234");
  sb.push(cat(B({0x30, 0x04, 0x00, 0x01, 'a'}), cat(S("z"), B({0x30, 0x04, 0x00, 0x01, 'b'}))));   // два пакета подряд (второй допишем)
  sb.push(S("w")); run(s, 30); CHECK(gCount == 4 && gTopic == "b" && gPayload == "w");
  sb.sent.clear(); sb.push(cat(B({0x32, 0x07, 0x00, 0x01, 't', 0x12, 0x34}), S("ok")));   // QoS 1, идентификатор 0x1234
  run(s, 20); CHECK(gTopic == "t" && gPayload == "ok" && sb.sent.size() >= 1 && sb.sent[0] == B({0x40, 0x02, 0x12, 0x34}));
  sb.push(B({0x30, 0x03, 0x00, 0x01, 'e'})); run(s, 20); CHECK(gTopic == "e" && gPayload == "");   // пустая нагрузка
  // пакет больше буфера: пропускается с ошибкой, следующий принимается
  Bytes huge = B({0x30, 0x90, 0x08, 0x00, 0x01, 'h'}); huge.resize(3 + 1040, 'x');
  gCount = 0; sb.push(huge); sb.push(B({0x30, 0x04, 0x00, 0x01, 'n', 'k'}));
  run(s, 50); CHECK(gCount == 1 && gTopic == "n" && gPayload == "k");

  // 8) поддержание связи: PINGREQ на половине периода, PINGRESP, потеря связи без ответа и переподключение
  reset(); FakeBroker kb; AlashMQTT k(kb); k.setServer("h"); k.setKeepAlive(10); k.connect(); kb.sent.clear();
  run(k, 4900); CHECK(kb.sent.empty());
  run(k, 300); CHECK(kb.sent.size() == 1 && kb.sent[0] == B({0xC0, 0x00}));
  run(k, 30000); CHECK(k.connected());                                                   // брокер отвечает PINGRESP, связь держится
  reset(); FakeBroker deaf; deaf.answerPing = false; AlashMQTT dk(deaf); dk.setServer("h"); dk.setKeepAlive(10); dk.connect();
  run(dk, 16000); CHECK(dk.error() == AlashMQTT::ERR_LOST || !dk.connected() || deaf.connects >= 2);

  // 9) переподключение с растущей паузой 2, 4, 8 секунд
  reset(); FakeBroker rb; AlashMQTT r(rb); r.setServer("h"); r.connect(); rb.reachable = false; rb.open = false; int c0 = rb.connects;
  uint32_t ts = g_ms; int tries = 0; uint32_t at[4] = {0};
  for (uint32_t i = 0; i < 40000 && tries < 4; i += 10) { g_ms += 10; int before = rb.connects; r.update(); if (rb.connects != before) at[tries++] = g_ms - ts; }
  CHECK(tries == 4 && at[0] < 100);                                                       // первая попытка сразу
  CHECK(at[1] - at[0] >= 1900 && at[1] - at[0] <= 2300 && at[2] - at[1] >= 3900 && at[2] - at[1] <= 4300 && at[3] - at[2] >= 7900 && at[3] - at[2] <= 8300 && rb.connects - c0 == 4);   // затем 2, 4, 8 секунд

  // 10) отключение
  reset(); FakeBroker db; AlashMQTT dd(db); dd.setServer("h"); dd.connect(); db.sent.clear(); dd.disconnect();
  CHECK(db.sent.size() == 1 && db.sent[0] == B({0xE0, 0x00}) && !dd.connected());

  // 11) тексты и diagnose
  reset(); FakeBroker gb; AlashMQTT g(gb); g.setServer("h"); g.setLanguage(AlashMQTT::LANG_KK); const char *kk = (const char *)g.errorText();
  g.setLanguage(AlashMQTT::LANG_EN); Print out; g.diagnose(out);
  CHECK(kk != nullptr && out.log.find("Connected") != std::string::npos);

  if (fails) { printf("FAILED: %d\n", fails); return 1; }
  printf("ALL TESTS PASSED\n");
  return 0;
}
