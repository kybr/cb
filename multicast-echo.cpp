#include <iostream>
#include <string>

//
#include <uvw.hpp>
using namespace std;
using namespace uvw;

int main(int argc, char* argv[]) {
  uint16_t port = (argc > 1) ? stoi(argv[1]) : 9999;
  string group = (argc > 2) ? argv[2] : "224.0.0.1";
  string interface = (argc > 3) ? argv[3] : "0.0.0.0";

  auto loop = Loop::getDefault();
  auto socket = loop->resource<UDPHandle>();

  socket->on<ErrorEvent>([&](const auto& error, auto& handle) {
    cerr << "ERROR: " << error.what() << endl;
  });

  socket->bind(interface, port);
  socket->multicastInterface(interface);
  socket->multicastMembership(group, interface,
                              UDPHandle::Membership::JOIN_GROUP);
  socket->multicastLoop(false);

  socket->on<UDPDataEvent>([&](const auto& data, auto& handle) {
    // echo

    cout << "ECHO: " << data.sender.ip << ":" << data.sender.port << endl;
    socket->send(data.sender, nullptr, 0);
  });

  socket->recv();
  return loop->run();
}
