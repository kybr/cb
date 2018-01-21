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

  auto loop = uvw::Loop::getDefault();

  auto server = loop->resource<uvw::UDPHandle>();
  server->on<uvw::ErrorEvent>([](const auto &error, auto &handle) {
    std::cerr << "server failed: " << error.what() << std::endl;
    exit(1);
  });

  auto client = loop->resource<uvw::UDPHandle>();

  server->on<uvw::UDPDataEvent>([&](const auto &data, auto &handle) {
    auto dataSend = std::unique_ptr<char[]>(new char[data.length]);
    memcpy(dataSend.get(), data.data.get(), data.length);
    client->send(data.sender.ip, data.sender.port, dataSend.get(), data.length);
    std::cout << "echoing " << data.length << " bytes." << std::endl;
    fflush(stdout);
  });

  server->bind(address, port);
  server->recv();
  std::cout << "Listening on " << address << ":" << port << std::endl;

  return loop->run();
}
