// https://skypjack.github.io/uvw/index.html
// https://github.com/skypjack/uvw
#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <uvw.hpp>

int main() {
  const std::string address = "0.0.0.0";
  const unsigned int port = 65535;

  int byteCount = 0;
  bool countUp = true;

  auto loop = uvw::Loop::getDefault();

  auto client = loop->resource<uvw::UDPHandle>();
  client->on<uvw::ErrorEvent>([&](const auto &error, auto &handle) {
    std::cerr << "client failed: " << error.what() << std::endl;
    byteCount--;
    countUp = false;
  });

  client->on<uvw::SendEvent>([](const auto &event, auto &handle) {
    std::cout << "sent " << std::endl;
    fflush(stdout);
  });

  auto timer = loop->resource<uvw::TimerHandle>();
  timer->on<uvw::ErrorEvent>([](const auto &error, auto &handle) {
    std::cout << "timer failed: " << error.what() << std::endl;
    fflush(stdout);
  });
  timer->on<uvw::TimerEvent>([&](const auto &event, auto &handle) {
    auto dataSend = std::unique_ptr<char[]>(new char[byteCount]);
    client->send(address, port, dataSend.get(), byteCount);
    std::cout << byteCount << std::endl;
    fflush(stdout);
    if (countUp) byteCount += 100;
  });
  timer->start(uvw::TimerHandle::Time{20}, uvw::TimerHandle::Time{10});

  return loop->run();
}
