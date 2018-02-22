#include <iostream>
#include <string>
#include <vector>

#include <uvw.hpp>
using namespace std;

int main(int argc, char* argv[]) {
  uint16_t mode = (argc > 1) ? stoi(argv[1]) : 0;
  uint16_t port = (argc > 2) ? stoi(argv[2]) : 9999;
  string group = (argc > 3) ? argv[3] : "224.0.0.1";
  string interface = (argc > 4) ? argv[4] : "0.0.0.0";

  unsigned guess = 70000;
  unsigned top, bottom;
  vector<char> buffer;

  auto loop = uvw::Loop::getDefault();

  auto socket = loop->resource<uvw::UDPHandle>();

  socket->on<uvw::ErrorEvent>([&](const auto& error, auto& handle) {
    printf("didn't send %u\n", guess);
    top = guess;
    guess = guess - (guess - bottom) / 2;

    //
    socket->send(group, port, &buffer[0], guess);
  });

  socket->bind(interface, port);
  socket->multicastInterface(interface);
  switch (mode) {
    case 0:
      socket->multicastLoop(false);
      socket->multicastTtl(1);
      break;
    case 1:
      socket->multicastLoop(true);
      socket->multicastTtl(1);
      break;
    case 2:
      socket->multicastLoop(true);
      socket->multicastTtl(0);
      break;
    case 3:
      socket->multicastLoop(false);
      socket->multicastTtl(0);
      break;
  }
  // route back to this machine through the loopback
  // socket->multicastLoop(false);
  // 0 means don't send on the network; 1 means keep it to the subnet
  // socket->multicastTtl(0);

  socket->on<uvw::UDPDataEvent>([&](const auto& data, auto& handle) {
    printf("received %u\n", guess);

    if (bottom == guess) {
      socket->stop();
      socket->close();
      return;
    }

    bottom = guess;
    guess = guess + (top - guess) / 2;

    //
    socket->send(group, port, &buffer[0], guess);
  });

  socket->recv();

  buffer.resize(guess);
  bottom = 0;
  top = guess;
  socket->send(group, port, &buffer[0], guess);

  return loop->run();
}
