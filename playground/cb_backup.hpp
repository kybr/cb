#ifndef __CB__
#define __CB__

#define ERROR(msg, code)                                        \
  do {                                                          \
    fprintf(stderr, "%s: [%s: %s]\n", msg, uv_err_name((code)), \
            uv_strerror((code)));                               \
    assert(0);                                                  \
  } while (0);

#include <uv.h>
#include <string>
#include <type_traits>
#include <vector>
using namespace std;

namespace cb {

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
  uint64_t dataSize = 0;
  uint32_t packetSize = 65508;
  uint32_t fragmentSize = 0;
  string port = "65531";
  string multicast = "225.0.0.7";
  string interface = "0.0.0.0";
  bool loopback_on = false;
  bool network_on = true;

  // libuv stuff
  uv_udp_t handle;
  vector<uv_buf_t> buffer;
  struct sockaddr_in destination;

 public:
  void spawn(uv_loop_t* loop) {
    uv_udp_init(loop, &handle);
    uv_udp_set_multicast_interface(&handle, interface.c_str());
    uv_ip4_addr(multicast.c_str(), stoi(port), &destination);
  }

  bool network() { return network_on; }
  void network(bool value) { network_on = value; }

  bool loopback() { return loopback_on; }
  void loopback(bool value) {
    if (uv_udp_set_multicast_loop(&handle, value ? 1 : 0) != 0)
      ;  // XXX check for failure
    loopback_on = value;
  }

  void transmit(char* data, size_t size) {
    if (!network_on) return;

    if (dataSize != size) {
      dataSize = size;
      fragmentSize = packetSize - sizeof(PacketHeader);
      unsigned finalFragmentSize = dataSize % fragmentSize;
      buffer.resize(dataSize / fragmentSize + (finalFragmentSize ? 1 : 0));
      PacketHeader header{0, (uint32_t)fragmentSize, 0,
                          (uint16_t)buffer.size()};
      for (unsigned i = 0; i < buffer.size(); ++i) {
        if (finalFragmentSize)
          if (i == buffer.size() - 1) header.fragmentSize = finalFragmentSize;
        memcpy(&buffer[i].base[0], &header, sizeof(PacketHeader));
        header.fragmentNumber++;
      }
    }

    // copy lots of data
    for (unsigned i = 0; i < buffer.size(); ++i) {
      reinterpret_cast<PacketHeader*>(&buffer[i].base[0])->frameNumber++;
      memcpy(&buffer[i].base[sizeof(PacketHeader)], &data[i * fragmentSize],
             packetSize);
    }

    int result = uv_udp_try_send(&handle, &buffer[0], buffer.size(),
                                 reinterpret_cast<sockaddr*>(&destination));
    if (result < 0) fprintf(stderr, "ERROR: uv_udp_try_send failed\n");
    fprintf(stderr, "%d bytes sent\n", result);
  }

  template <typename T>
  void transmit(T* t) {
    // XXX we wish we could check to see if T has any pointers!
    static_assert(std::is_trivially_copyable<T>::value,
                  "This type is not trivially copyable!");
    transmit(static_cast<char*>(t), sizeof(T));
  }
};

class Receiver {
  string port = "65531";
  string multicast = "225.0.0.7";
  string interface = "0.0.0.0";

  // libuv stuff
  uv_udp_t handle;
  vector<uv_buf_t> buffer;
  struct sockaddr_in address;

 public:
  void spawn(uv_loop_t* loop) {
    uv_udp_init(loop, &handle);
    uv_udp_set_membership(&handle, multicast.c_str(), interface.c_str(),
                          UV_JOIN_GROUP);
    // int uv_udp_set_multicast_interface(uv_udp_t* handle, const char*
    // interface_addr)
    uv_ip4_addr(interface.c_str(), stoi(port), &address);
    uv_udp_bind(&handle, reinterpret_cast<sockaddr*>(&address), 0);
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
