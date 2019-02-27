#ifndef __CB__
#define __CB__

#define test(x)                                                         \
  do {                                                                  \
    if (!x) {                                                           \
      fprintf(stderr, "ERROR| in %s at line %d\n", __FILE__, __LINE__); \
      fprintf(stderr, "ERROR| trying " #x);                             \
    }                                                                   \
  } while (0)

#ifdef __INFO__
#define info(x, ...)                                                 \
  do {                                                               \
    fprintf(stdout, "INFO| in %s at line %d\n", __FILE__, __LINE__); \
    fprintf(stdout, "INFO| " x, ##__VA_ARGS__);                      \
  } while (0)
#else
#define info(x, ...) \
  do {               \
  } while (0)
#endif

#define warn(x, ...)                                                 \
  do {                                                               \
    fprintf(stderr, "WARN| in %s at line %d\n", __FILE__, __LINE__); \
    fprintf(stderr, "WARN| " x, ##__VA_ARGS__);                      \
  } while (0)

#include <string.h>  // memcpy

#include <memory>
#include <set>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include <uvw.hpp>

#define SERVER_PORT (9999)
#define BROADCAST_PORT (10001)
#define BROADCAST_GROUP ("232.0.0.3")

namespace cb {

struct Header {
  // 64b
  uint8_t sequenceNumber;
  uint8_t flag[7];
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
  std::string group = BROADCAST_GROUP;
  std::string interface = "0.0.0.0";

  // libuv stuff
  std::shared_ptr<uvw::UDPHandle> socket;
  std::shared_ptr<uvw::Loop> loop;
  std::thread t;

  // data bookkeeping
  std::vector<char> buffer;
  uint64_t dataSize = 0;
  // uint32_t packetSize = 1400;

  // this should be automatic by default
  uint32_t packetSize = 65508;  // 65508 is the maximum size for IPv4 UDP
  uint32_t fragmentSize = 0;
  uint16_t packetCount = 0;
  uint16_t finalFragmentSize = 0;

 public:
  void start() {
    loop = uvw::Loop::getDefault();
    socket = loop->resource<uvw::UDPHandle>();

    socket->on<uvw::ErrorEvent>([](const auto& error, auto& handle) {
      warn("server failed: %s", error.what());
      exit(1);

      // handle transmission errors by backing off packet size
      //
      //
    });

    socket->bind(interface, SERVER_PORT);
    test(socket->multicastInterface(interface));
    test(socket->multicastLoop(true));
    test(socket->multicastTtl(4));

    // listen
    socket->on<uvw::UDPDataEvent>([](const auto& data, auto& handle) {
      // TODO make this a proper ACK with type, time, and fragmentIndex
      if (data.length == 0)
        info("Broadcaster received ACK from %s\n", data.sender.ip.c_str());
      else
        info("Receiver %u bytes from %s\n", data.length,
             data.sender.ip.c_str());
    });

    socket->recv();

    // start the broadcaster in a thread
    t = std::thread([&]() {
      loop->run();
      std::cout << "Broadcaster main thread completed" << std::endl;
    });
    std::cout << "Broadcaster started in thread" << std::endl;
  }

  void stop() {
    loop->stop();
    std::cout << "Broadcaster loop stopped; Waiting for join" << std::endl;
    t.join();
  }

  void loopback(bool value) { test(socket->multicastLoop(value)); }

  bool size(unsigned packetSize) {
    // TODO this should set the packet size, live
    // - (maybe) shut off automatic packet size determination
    // - recalculate fragmentSize, finalFragmentSize, packetCount,

    this->packetSize = packetSize;
    return false;
  }

  void transmit(char* data, size_t size) {
    info("sizeof(Header): %lu\n", sizeof(Header));

    // (we hope) one-time setup of data headers and such
    //
    // TODO also detect when packet sizes change, maybe
    if (dataSize != size) {
      // if (size < packetSize) packetSize = sizeof(Header) + size;
      dataSize = size;
      info("dataSize: %llu\n", dataSize);

      if (dataSize < packetSize - sizeof(Header))
        packetSize = dataSize + sizeof(Header);
      info("packetSize: %u\n", packetSize);

      fragmentSize = packetSize - sizeof(Header);
      info("fragmentSize: %u\n", fragmentSize);

      finalFragmentSize = dataSize % fragmentSize;
      info("finalFragmentSize: %u\n", finalFragmentSize);

      packetCount = dataSize / fragmentSize;
      if (finalFragmentSize) packetCount++;
      info("packetCount: %u\n", packetCount);

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
      (*h)++;  // increment sequenceNumber
      memcpy(b, d, fragmentSize);
      socket->send(group, port, h, packetSize);

      h += packetSize;
      b += packetSize;
      d += fragmentSize;
    }
    //
    (*h)++;  // increment sequenceNumber
    if (finalFragmentSize) {
      memcpy(b, d, finalFragmentSize);
      socket->send(group, port, h, finalFragmentSize + sizeof(Header));
    } else {
      memcpy(b, d, fragmentSize);
      socket->send(group, port, h, packetSize);
    }
  }

  template <typename T>
  void transmit(T* t) {
    // XXX we wish we could check to see if T has any pointers!
    // XXX use macro to define State tuple/struct
    static_assert(std::is_trivially_copyable<T>::value,
                  "This type is not trivially copyable!");
    transmit(reinterpret_cast<char*>(t), sizeof(T));
  }
};  // namespace cb

class Receiver {
  uint16_t port = BROADCAST_PORT;
  std::string group = BROADCAST_GROUP;
  std::string interface = "0.0.0.0";

  std::shared_ptr<uvw::UDPHandle> socket;
  std::shared_ptr<uvw::Loop> loop;
  std::thread t;

  std::set<uint16_t> dontHave;
  uint16_t fragmentCount;  // how many to expect
  uint16_t fragmentSize = 0;
  uint8_t sequenceNumber;  // which we heard last

  std::mutex m;              // protects these (below)
  std::vector<char> buffer;  // teh datahz
  bool hasData = false;

 public:
  void start() {
    loop = uvw::Loop::getDefault();
    // loop = make_unique<uvw::Loop>(uvw::Loop::getDefault());
    socket = loop->resource<uvw::UDPHandle>();

    socket->on<uvw::ErrorEvent>([](const auto& error, auto& handle) {
      warn("server failed: %s", error.what());
      exit(1);
    });

    socket->on<uvw::UDPDataEvent>([this](const auto& data, auto& handle) {
      m.lock();  // XXX lock this whole method?

      info("This is the receiver\n");
      info("Received %u bytes from %s\n", data.length, data.sender.ip.c_str());

      const Header* header = reinterpret_cast<const Header*>(data.data.get());

      // if this is the first time we got a packet or
      // if the packet size been changed....
      //
      if (header->dataSize != buffer.size()) {
        //
        //
        buffer.resize(header->dataSize);

        // XXX what if this packet happens to be the final packet of a series.
        // in that case, this next line will be a lie.
        fragmentSize = header->fragmentSize;
        uint16_t finalFragmentSize = header->dataSize % fragmentSize;
        fragmentCount = header->dataSize / fragmentSize;
        if (finalFragmentSize) fragmentCount++;

        // reset the set of necessary fragments
        //
        dontHave.clear();
        for (uint16_t t = 0; t < fragmentCount; ++t) dontHave.insert(t);

        sequenceNumber = header->sequenceNumber;
      }

      if (header->sequenceNumber != sequenceNumber) {
        // never gets here
        // printf("%u started\n", header->sequenceNumber);
        // we're on to a new thing; cut bait and start over
        //
        info("woops! we heard a new sequence number, so we're moving on...\n");
        sequenceNumber = header->sequenceNumber;
        dontHave.clear();
        for (uint16_t t = 0; t < fragmentCount; ++t) dontHave.insert(t);
      }

      if (dontHave.count(header->fragmentIndex)) {
        // printf("%u/%u\n", 1 + header->fragmentIndex, fragmentCount);
        info("we didn't have fragment %u; thanks!\n", header->fragmentIndex);
        memcpy(&buffer[header->fragmentIndex * fragmentSize],
               data.data.get() + sizeof(Header), header->fragmentSize);
        dontHave.erase(header->fragmentIndex);
      } else {
        // XXX duplicate received
      }

      if (dontHave.empty()) {
        // printf("%u done\n", header->sequenceNumber);
        info("we have a complete package\n");
        hasData = true;
      }

      m.unlock();

      info("sequenceNumber:%u\n", header->sequenceNumber);
      info("dataSize:%u\n", header->dataSize);
      info("fragmentSize:%hu\n", header->fragmentSize);
      info("fragmentIndex:%hu\n", header->fragmentIndex);
      info("packetSize:%lu\n", header->fragmentSize + sizeof(Header));
      info("fragmentCount:%hu\n", fragmentCount);
      info("finalFragmentSize:%hu\n", header->dataSize % header->fragmentSize);

      //
      //
      //
      this->socket->send(data.sender.ip, SERVER_PORT, nullptr, 0);  // ack
    });

    socket->bind(interface, port);
    socket->multicastMembership(group, interface,
                                uvw::UDPHandle::Membership::JOIN_GROUP);
    socket->recv();

    // start the receiver in a thread
    t = std::thread([&]() {
      loop->run();
      std::cout << "Received main thread completed" << std::endl;
    });
    std::cout << "Receiver started in thread" << std::endl;
  }

  void stop() {
    loop->stop();
    std::cout << "Receiver loop stopped; Waiting for join" << std::endl;
    t.join();
  }

  bool receive(char* data, size_t size) {
    // assume there's no new data
    //
    bool gotNewData = false;

    // if we get the lock
    //
    if (m.try_lock()) {
      // both _newData_  and _buffer_ are protected by the lock
      //
      if (hasData) {
        hasData = false;
        gotNewData = true;  // this is the only way this can be true
        memcpy(data, &buffer[0], size);
      }

      // always let go quickly
      //
      m.unlock();
    }
    return gotNewData;
  }

  template <typename T>
  bool receive(T* t) {
    // XXX we wish we could check to see if T has any pointers!
    static_assert(std::is_trivially_copyable<T>::value,
                  "This type is not trivially copyable!");
    return receive(reinterpret_cast<char*>(t), sizeof(T));
  }
};

}  // namespace cb

#endif
