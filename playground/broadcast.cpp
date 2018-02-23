#include <uvw.hpp>

#include <unistd.h>
#include <iostream>
#include <string>

#include "help.hpp"

using namespace std;

int main() {
  auto loop = uvw::Loop::getDefault();
  auto server = loop->resource<uvw::UDPHandle>();
  server->on<uvw::ErrorEvent>([](const auto &error, auto &handle) {
    cerr << "server failed: " << error.what() << endl;
    exit(1);
  });
  server->bind("0.0.0.0", 9999);
  ensure(server->multicastInterface("0.0.0.0"));
  ensure(server->multicastLoop(true));
  ensure(server->multicastTtl(23));

  // listen
  server->on<uvw::UDPDataEvent>([](const auto &data, auto &handle) {
    cout << "got " << data.length << " bytes from " << data.sender.ip << endl;
  });
  server->recv();

  auto timer = loop->resource<uvw::TimerHandle>();
  timer->on<uvw::ErrorEvent>([](const auto &error, auto &handle) {
    cout << "timer failed: " << error.what() << endl;
  });

  string message = "this is the truth";
  timer->on<uvw::TimerEvent>([&](const auto &event, auto &handle) {
    server->send("232.0.0.3", 10001, &message[0], message.length());
    printf("sent %s\n", message.c_str());
  });

  timer->start(uvw::TimerHandle::Time{200}, uvw::TimerHandle::Time{200});

  return loop->run();
}
