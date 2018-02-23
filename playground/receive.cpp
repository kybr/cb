#include <uvw.hpp>

#include <unistd.h>

#include <iostream>
#include <string>

using namespace std;

int main() {
  auto loop = uvw::Loop::getDefault();
  auto server = loop->resource<uvw::UDPHandle>();
  server->on<uvw::ErrorEvent>([](const auto &error, auto &handle) {
    cerr << "server failed: " << error.what() << endl;
    exit(1);
  });

  server->on<uvw::UDPDataEvent>([&server](const auto &data, auto &handle) {
    cout << "got " << data.length << " bytes from " << data.sender.ip << endl;
    server->send(data.sender.ip, 9999, nullptr, 0);  // ack
  });

  server->bind("0.0.0.0", 10001);
  server->multicastMembership("232.0.0.3", "0.0.0.0",
                              uvw::UDPHandle::Membership::JOIN_GROUP);
  server->recv();

  cout << "Listening..." << endl;
  return loop->run();
}
