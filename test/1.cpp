#include <iostream>
#include <string>

#include <unistd.h>
#include <cstdlib>

#include <uvw.hpp>
#include "cb.hpp"

// https://en.wikipedia.org/wiki/Fletcher%27s_checksum
uint16_t fletcher16(const uint8_t *data, size_t len) {
  uint32_t c0, c1;
  unsigned int i;

  for (c0 = c1 = 0; len >= 5802; len -= 5802) {
    for (i = 0; i < 5802; ++i) {
      c0 = c0 + *data++;
      c1 = c1 + c0;
    }
    c0 = c0 % 255;
    c1 = c1 % 255;
  }
  for (i = 0; i < len; ++i) {
    c0 = c0 + *data++;
    c1 = c1 + c0;
  }
  c0 = c0 % 255;
  c1 = c1 % 255;
  return (c1 << 8 | c0);
}

template <typename T>
uint16_t sum(T *t) {
  return fletcher16((const uint8_t *)t, sizeof(T));
}

using namespace std;

#define N (120000)
struct State {
  unsigned n;
  char data[N];
};

int main() {
  cb::Broadcaster broadcaster;
  broadcaster.size(9000);
  broadcaster.start();

  cb::Receiver receiver;
  receiver.start();

  cout << "waiting for char from user" << endl;

  getchar();

  cout << "Calling stop on broadcaster" << endl;
  broadcaster.stop();

  cout << "Calling stop on receiver" << endl;
  receiver.stop();

  return 0;
}

int __main() {
  cb::Broadcaster broadcaster;
  broadcaster.size(9000);
  broadcaster.start();

  cb::Receiver receiver;
  receiver.start();

  sleep(1);

  State *state = new State();
  unsigned char *p = reinterpret_cast<unsigned char *>(state);
  for (unsigned c = 0; c < sizeof(State); c++) *p++ = (unsigned char)rand();
  state->n = 0;
  sprintf(state->data, "this is the truth");

  auto loop = uvw::Loop::getDefault();
  auto timer = loop->resource<uvw::TimerHandle>();
  timer->on<uvw::ErrorEvent>([](const auto &error, auto &handle) {
    cout << "timer failed: " << error.what() << endl;
  });

  State *other = new State();
  memset(other, 0, sizeof(State));

  bool haveReceived = true;

  string h1, h2;
  timer->on<uvw::TimerEvent>([&](const auto &event, auto &handle) {
    if (haveReceived) {
      h1 = sum(state);
      // cout << h1 << " -> " << endl;
      broadcaster.transmit(state);
      state->n++;
      haveReceived = false;
    }

    haveReceived = receiver.receive(other);
    if (haveReceived) {
      h2 = sum(other);
      cout << h1 << " -> " << endl;
      cout << h2 << " <- " << endl;
      if (h1 != h2) {
        fprintf(stderr, "checksum failed\n");
        exit(1);
      }
    }
  });

  timer->start(uvw::TimerHandle::Time{20}, uvw::TimerHandle::Time{20});

  thread t([&]() {
    loop->run();
    cout << "main libuv loop completed" << endl;
  });

  cout << "waiting for char from user" << endl;

  getchar();

  receiver.stop();
  broadcaster.stop();

  loop->stop();
  t.join();

  return 0;
}
