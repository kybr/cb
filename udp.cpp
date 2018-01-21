// https://skypjack.github.io/uvw/index.html
// https://github.com/skypjack/uvw
#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <uvw.hpp>

int main(int argc, char *argv[]) {
  const std::string address = argv[1];
  const unsigned int port = atoi(argv[2]);

  auto loop = uvw::Loop::getDefault();

  auto server = loop->resource<uvw::UDPHandle>();
  server->on<uvw::ErrorEvent>([](const auto &error, auto &handle) {
    std::cerr << "server failed: " << error.what() << std::endl;
    exit(1);
  });
  server->on<uvw::UDPDataEvent>([](const auto &data, auto &handle) {
    std::cout << "got " << data.length << std::endl;
    fflush(stdout);
  });
  server->bind(address, port);
  server->recv();  // actually starts listening

  auto client = loop->resource<uvw::UDPHandle>();
  client->on<uvw::ErrorEvent>([](const auto &error, auto &handle) {
    std::cerr << "client failed: " << error.what() << std::endl;
    exit(1);
  });
  client->on<uvw::SendEvent>([](const auto &event, auto &handle) {
    std::cout << "sent " << std::endl;
    fflush(stdout);
  });

  int n = 0;
  auto timer = loop->resource<uvw::TimerHandle>();
  timer->on<uvw::ErrorEvent>([](const auto &error, auto &handle) {
    std::cout << "timer failed: " << error.what() << std::endl;
    fflush(stdout);
  });
  timer->on<uvw::TimerEvent>(
      [&client, &address, &port, &n](const auto &event, auto &handle) {
        auto dataSend = std::unique_ptr<char[]>(new char[n]);
        client->send(address, port, dataSend.get(), n);
        std::cout << n << std::endl;
        fflush(stdout);
        n += 100;
      });
  timer->start(uvw::TimerHandle::Time{20}, uvw::TimerHandle::Time{10});

  return loop->run();
}
