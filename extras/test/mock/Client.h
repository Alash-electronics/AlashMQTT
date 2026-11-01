// Имитация Client (сетевого соединения) и простого брокера MQTT: отвечает на CONNECT / SUBSCRIBE / PINGREQ, записывает всё полученное.
#pragma once
#include "Arduino.h"
class Client {
 public:
  virtual ~Client() {}
  virtual int connect(const char *host, uint16_t port) = 0;
  virtual size_t write(const uint8_t *buf, size_t size) = 0;
  virtual int available() = 0;
  virtual int read() = 0;
  virtual void stop() = 0;
  virtual uint8_t connected() = 0;
};
struct FakeBroker : public Client {
  bool reachable = true, open = false, silent = false;
  uint8_t connackCode = 0; bool answerPing = true, requireAuth = false;
  std::string expectUser, expectPass;
  std::deque<uint8_t> in;                          // что "пришло" от брокера
  std::vector<std::vector<uint8_t>> sent;          // пакеты, которые отправила библиотека
  std::string host; uint16_t port = 0; int connects = 0, stops = 0;
  std::vector<std::string> subscribed;
  int connect(const char *h, uint16_t p) override { connects++; host = h; port = p; if (!reachable) return 0; open = true; in.clear(); return 1; }
  void stop() override { open = false; stops++; }
  uint8_t connected() override { return open; }
  int available() override { return (int)in.size(); }
  int read() override { if (in.empty()) return -1; uint8_t b = in.front(); in.pop_front(); return b; }
  size_t write(const uint8_t *b, size_t n) override {
    if (!open) return 0;
    sent.emplace_back(b, b + n);
    uint8_t type = b[0] & 0xF0;
    if (type == 0x10 && !silent) { uint8_t c = connackCode; in.insert(in.end(), {0x20, 0x02, 0x00, c}); }
    else if (type == 0x80) {                                                            // SUBSCRIBE: запомним тему, ответим SUBACK
      size_t p = 2; unsigned tl = (b[p + 2] << 8) | b[p + 3];
      subscribed.push_back(std::string((const char *)b + p + 4, tl));
      in.insert(in.end(), {0x90, 0x03, b[p], b[p + 1], 0x00});
    } else if (type == 0xC0 && answerPing) in.insert(in.end(), {0xD0, 0x00});
    return n;
  }
  void push(const std::vector<uint8_t> &pkt) { in.insert(in.end(), pkt.begin(), pkt.end()); }
};
