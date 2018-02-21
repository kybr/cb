#ifndef __CB__
#define __CB__

#define t(x)                                                            \
  do {                                                                  \
    if (!x) {                                                           \
      fprintf(stderr, "ERROR| in %s at line %d\n", __FILE__, __LINE__); \
      fprintf(stderr, "ERROR| trying " #x);                             \
    }                                                                   \
  } while (0)

#include <string>
#include <type_traits>
#include <uvw.hpp>
#include <vector>
using namespace std;

namespace cb {

// if we could use/abuse portions of the IP header, that would be better...
struct PacketHeader {
  // identifies this packet as "belonging" to a particular frame
  uint64_t frameNumber;

  // bytes in this fragment; only the final fragment might be different
  uint32_t fragmentSize;

  // page "fragmentNumber" of "fragmentCount"
  uint16_t fragmentNumber;
  uint16_t fragmentCount;
};

class Broadcaster {
  uint16_t port = 10001;
  string group = "232.0.0.3";
  string interface = "0.0.0.0";

  // libuv stuff
  shared_ptr<uvw::UDPHandle> sender;

  // data bookkeeping
  vector<char*> buffer;
  uint64_t dataSize = 0;
  uint32_t packetSize = 65508;
  uint32_t fragmentSize = 0;

 public:
  void start(shared_ptr<uvw::Loop> loop) {
    sender.reset(loop->resource<uvw::UDPHandle>());

    sender->on<uvw::ErrorEvent>([](const auto& error, auto& handle) {
      cerr << "server failed: " << error.what() << endl;
      exit(1);
    });

    sender->bind(interface, 9999);
    t(sender->multicastInterface(interface));
    t(sender->multicastLoop(true));
    t(sender->multicastTtl(23));

    // listen
    sender->on<uvw::UDPDataEvent>([](const auto& data, auto& handle) {
      cout << "got " << data.length << " bytes from " << data.sender.ip << endl;
    });

    sender->recv();
  }

  void loopback(bool value) { t(sender->multicastLoop(value)); }

  void transmit(char* data, size_t size) {
    // (we hope) one-time setup of data headers and such
    //
    if (dataSize != size) {
      dataSize = size;
      fragmentSize = packetSize - sizeof(PacketHeader);
      unsigned finalFragmentSize = dataSize % fragmentSize;
      PacketHeader header{0, fragmentSize, 0, (uint16_t)buffer.size()};

      // probably should not delete, but reuse some block of memory
      for (unsigned i = 0; i < buffer.size(); ++i) delete[] buffer[i];
      buffer.resize(dataSize / fragmentSize + (finalFragmentSize ? 1 : 0));

      for (unsigned i = 0; i < buffer.size(); ++i) {
        if (finalFragmentSize)
          if (i == buffer.size() - 1) header.fragmentSize = finalFragmentSize;
        buffer[i] = new char[packetSize];  // probably bad cache performance
        memcpy(&buffer[i], &header, sizeof(PacketHeader));
        header.fragmentNumber++;
      }
    }

    // copy lots of data
    for (unsigned i = 0; i < buffer.size(); ++i) {
      reinterpret_cast<PacketHeader*>(&buffer[i])->frameNumber++;
      memcpy(&buffer[i] + sizeof(PacketHeader), &data[i * fragmentSize],
             fragmentSize);
    }

    for (unsigned i = 0; i < buffer.size(); ++i) {
      sender->send(group, port, buffer[i], packetSize);
    }
  }

  template <typename T>
  void transmit(T* t) {
    // XXX we wish we could check to see if T has any pointers!
    static_assert(std::is_trivially_copyable<T>::value,
                  "This type is not trivially copyable!");
    transmit(reinterpret_cast<char*>(t), sizeof(T));
  }
};  // namespace cb

class Receiver {
  uint16_t port = 10001;
  string group = "232.0.0.3";
  string interface = "0.0.0.0";
  shared_ptr<uvw::UDPHandle> listener;

 public:
  void start(shared_ptr<uvw::Loop> loop) {
    listener.reset(loop->resource<uvw::UDPHandle>());

    listener->on<uvw::ErrorEvent>([](const auto& error, auto& handle) {
      cerr << "server failed: " << error.what() << endl;
      exit(1);
    });

    listener->on<uvw::UDPDataEvent>([this](const auto& data, auto& handle) {
      cout << "got " << data.length << " bytes from " << data.sender.ip << endl;
      this->listener->send(data.sender.ip, 9999, nullptr, 0);  // ack
    });

    listener->bind(interface, port);
    listener->multicastMembership(group, interface,
                                  uvw::UDPHandle::Membership::JOIN_GROUP);
    listener->recv();
  }

  int receive(char* data, size_t size) {
    //
    //
    //
    //
    return 0;
  }

  template <typename T>
  int receive(T* t) {
    // XXX we wish we could check to see if T has any pointers!
    static_assert(std::is_trivially_copyable<T>::value,
                  "This type is not trivially copyable!");
    receive(static_cast<char*>(t), sizeof(T));
  }
};

}  // namespace cb

#endif
