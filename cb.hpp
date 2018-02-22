#ifndef __CB__
#define __CB__

#define t(x)                                                            \
  do {                                                                  \
    if (!x) {                                                           \
      fprintf(stderr, "ERROR| in %s at line %d\n", __FILE__, __LINE__); \
      fprintf(stderr, "ERROR| trying " #x);                             \
    }                                                                   \
  } while (0)

#include <string.h>
#include <set>
#include <string>
#include <type_traits>
#include <uvw.hpp>
#include <vector>
using namespace std;

#define SERVER_PORT (9999)
#define BROADCAST_PORT (10001)
#define BROADCAST_GROUP ("232.0.0.3")

namespace cb {

// if we could use/abuse portions of the IP header, that would be better...
struct Header {
  // 64b
  uint8_t sequenceNumber;
  uint8_t data[7];
  // 64b
  uint32_t dataSize;
  uint16_t fragmentSize;
  uint16_t fragmentIndex;  // which piece this is

  // packetSize = fragmentSize + sizeof(Header)
  // finalFragmentSize = dataSize % fragmentSize
  // fragmentCount = dataSize / fragmentSize (+1 if finalFragmentSize)
};

class Broadcaster {
  uint16_t port = BROADCAST_PORT;
  string group = BROADCAST_GROUP;
  string interface = "0.0.0.0";

  // libuv stuff
  shared_ptr<uvw::UDPHandle> sender;

  // data bookkeeping
  vector<char> buffer;
  uint64_t dataSize = 0;
  // uint32_t packetSize = 1400;

  // this should be automatic by default
  uint32_t packetSize = 65508;  // 65508 is the maximum size for IPv4 UDP
  uint32_t fragmentSize = 0;
  uint16_t packetCount = 0;
  uint16_t finalFragmentSize = 0;

 public:
  void start(shared_ptr<uvw::Loop> loop) {
    sender = loop->resource<uvw::UDPHandle>();

    sender->on<uvw::ErrorEvent>([](const auto& error, auto& handle) {
      cerr << "server failed: " << error.what() << endl;

      // handle transmission errors by backing off packet size
      //
      //

      exit(1);
    });

    sender->bind(interface, SERVER_PORT);
    t(sender->multicastInterface(interface));
    t(sender->multicastLoop(true));
    t(sender->multicastTtl(4));

    // listen
    sender->on<uvw::UDPDataEvent>([](const auto& data, auto& handle) {
      // TODO make this a proper ACK with type, time, and fragmentIndex
      if (data.length == 0)
        cout << "Broadcaster received ACK from " << data.sender.ip << endl;
      else
        cout << data.length << " bytes from " << data.sender.ip << endl;
    });

    sender->recv();
  }

  void loopback(bool value) { t(sender->multicastLoop(value)); }

  bool size(unsigned packetSize) {
    // TODO this should set the packet size, live
    // - (maybe) shut off automatic packet size determination
    // - recalculate fragmentSize, finalFragmentSize, packetCount,

    this->packetSize = packetSize;
    return false;
  }

  void transmit(char* data, size_t size) {
    printf("sizeof(Header): %lu\n", sizeof(Header));

    // (we hope) one-time setup of data headers and such
    //
    // TODO also detect when packet sizes change, maybe
    if (dataSize != size) {
      // if (size < packetSize) packetSize = sizeof(Header) + size;
      dataSize = size;
      printf("dataSize: %llu\n", dataSize);

      if (dataSize < packetSize - sizeof(Header))
        packetSize = dataSize + sizeof(Header);
      printf("packetSize: %u\n", packetSize);

      fragmentSize = packetSize - sizeof(Header);
      printf("fragmentSize: %u\n", fragmentSize);

      finalFragmentSize = dataSize % fragmentSize;
      printf("finalFragmentSize: %u\n", finalFragmentSize);

      packetCount = dataSize / fragmentSize;
      if (finalFragmentSize) packetCount++;
      printf("packetCount: %u\n", packetCount);

      buffer.resize(dataSize + packetCount * sizeof(Header));

      Header header{0};
      header.dataSize = dataSize;
      header.fragmentSize = fragmentSize;

      char* b = &buffer[0];
      for (unsigned i = 0; i < packetCount - 1; ++i) {
        memcpy(b, &header, sizeof(Header));
        header.fragmentIndex++;
        b += packetSize;
      }
      if (finalFragmentSize) header.fragmentSize = finalFragmentSize;
      memcpy(b, &header, sizeof(Header));
    }

    // TODO parallelize everything below with async
    //
    char* h = &buffer[0];
    char* b = &buffer[0] + sizeof(Header);
    char* d = &data[0];
    for (unsigned i = 0; i < packetCount - 1; ++i) {
      (*b)++;  // increment sequenceNumber
      memcpy(b, d, fragmentSize);
      sender->send(group, port, h, packetSize);

      h += packetSize;
      b += packetSize;
      d += fragmentSize;
    }
    //
    (*b)++;  // increment sequenceNumber
    if (finalFragmentSize) {
      memcpy(b, d, finalFragmentSize);
      sender->send(group, port, h, finalFragmentSize + sizeof(Header));
    } else {
      memcpy(b, d, fragmentSize);
      sender->send(group, port, h, packetSize);
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
  uint16_t port = BROADCAST_PORT;
  string group = BROADCAST_GROUP;
  string interface = "0.0.0.0";

  shared_ptr<uvw::UDPHandle> listener;
  vector<char> buffer;  // teh datahz
  set<uint16_t> dontHave;
  uint16_t fragmentCount;  // how many to expect
  uint16_t fragmentSize = 0;
  uint8_t sequenceNumber;  // which we heard last

 public:
  void start(shared_ptr<uvw::Loop> loop) {
    listener = loop->resource<uvw::UDPHandle>();

    listener->on<uvw::ErrorEvent>([](const auto& error, auto& handle) {
      cerr << "server failed: " << error.what() << endl;
      exit(1);
    });

    listener->on<uvw::UDPDataEvent>([this](const auto& data, auto& handle) {
      cout << "This is the receiver" << endl;
      cout << "got " << data.length << " bytes from " << data.sender.ip << endl;
      this->listener->send(data.sender.ip, SERVER_PORT, nullptr, 0);  // ack

      const Header* header = reinterpret_cast<const Header*>(data.data.get());

      if (header->dataSize != buffer.size()) {
        buffer.resize(header->dataSize);

        // XXX what if this packet happens to be
        fragmentSize = header->fragmentSize;
        uint16_t finalFragmentSize = header->dataSize % fragmentSize;
        fragmentCount = header->dataSize / fragmentSize;
        if (finalFragmentSize) fragmentCount++;

        for (uint16_t t = 0; t < fragmentCount; ++t) dontHave.insert(t);
        sequenceNumber = header->sequenceNumber;
      }

      if (header->sequenceNumber != sequenceNumber) {
        // we're on to a new thing; cut bait and start over
        //
        printf(
            "woops! we heard a new sequence number, so we're moving on...\n");
        sequenceNumber = header->sequenceNumber;
        dontHave.clear();
        for (uint16_t t = 0; t < fragmentCount; ++t) dontHave.insert(t);
      }

      if (dontHave.count(header->fragmentIndex)) {
        printf("we didn't have fragment %u; thanks!\n", header->fragmentIndex);
        memcpy(&buffer[header->fragmentIndex * fragmentSize],
               data.data.get() + sizeof(Header), header->fragmentSize);
        dontHave.erase(header->fragmentIndex);
      } else {
        // XXX duplicate received
      }

      if (dontHave.empty()) {
        printf("we have a complete package\n");
        // we have a complete package; tell someone
        //
        //
      }

      printf("sequenceNumber:%u\n", header->sequenceNumber);
      printf("dataSize:%u\n", header->dataSize);
      printf("fragmentSize:%hu\n", header->fragmentSize);
      printf("fragmentIndex:%hu\n", header->fragmentIndex);
      printf("packetSize:%lu\n", header->fragmentSize + sizeof(Header));
      printf("fragmentCount:%hu\n", fragmentCount);

      uint16_t finalFragmentSize = header->dataSize % header->fragmentSize;
      printf("finalFragmentSize:%hu\n", finalFragmentSize);
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
