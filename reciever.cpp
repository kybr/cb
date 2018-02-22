#include <uv.h>
#include <cstdlib>
#include <string>
#include <type_traits>
#include <vector>

#include "help.hpp"
using namespace std;

static void allocate(uv_handle_t*, size_t suggested_size, uv_buf_t* buf) {
  buf->base = static_cast<char*>(malloc(suggested_size));
  buf->len = suggested_size;
};

static void receive(uv_udp_t* _socket, ssize_t bytes, const uv_buf_t* buf,
                    const struct sockaddr* sender, unsigned flags) {
  if (bytes < 0) {
    fprintf(stderr, "recv error unexpected\n");
    uv_close((uv_handle_t*)_socket, nullptr);
    free(buf->base);
    return;
  }
  if (sender == nullptr) {
    fprintf(stderr, "no more data to read\n");
    return;
  }

  char buffer[17] = {0};
  uv_ip4_name((struct sockaddr_in*)sender, buffer, 16);
  printf("Received %ld byte%s from %s\n", bytes, bytes == 1 ? "" : "s", buffer);
  printf("  (%p, %zu)\n", buf->base, buf->len);
  for (unsigned i = 0; i < bytes; ++i)
    printf("%u: %x %c\n", i, buf->base[i], buf->base[i]);
  free(buf->base);
};

int main() {
  uv_loop_t* loop = uv_default_loop();
  uv_udp_t socket;
  ensure(uv_udp_init(loop, &socket));
  struct sockaddr_in address;
  ensure(uv_ip4_addr("0.0.0.0", 10001, &address));
  ensure(uv_udp_bind(&socket, (const sockaddr*)&address, 0));
  ensure(uv_udp_set_membership(&socket, "225.0.0.7", "0.0.0.0", UV_JOIN_GROUP));
  ensure(uv_udp_recv_start(&socket, allocate, receive));
  return uv_run(loop, UV_RUN_DEFAULT);
};
