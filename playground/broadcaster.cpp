#include <unistd.h>
#include <uv.h>
#include <string>
#include <vector>

#include <unistd.h>
#include "help.hpp"
using namespace std;

int loopback = 1;

int main() {
  uv_loop_t* loop = uv_default_loop();
  uv_udp_t handle;
  ensure(uv_udp_init(loop, &handle));

  struct sockaddr_in local;
  ensure(uv_ip4_addr("0.0.0.0", 9999, &local));
  ensure(uv_udp_bind(&handle, (const struct sockaddr*)&local, 0));

  // XXX the socket must be bound (as above) for these to work
  ensure(uv_udp_set_multicast_ttl(&handle, 33));
  ensure(uv_udp_set_multicast_interface(&handle, "0.0.0.0"));
  ensure(uv_udp_set_multicast_loop(&handle, loopback));

  auto sent = [](uv_udp_send_t* req, int status) {
    printf("...packet sent\n");
  };

  uv_run(loop, UV_RUN_DEFAULT);

  struct sockaddr_in remote;
  uv_ip4_addr("225.0.0.7", 10001, &remote);

  while (true) {
    printf("sending packet...\n");
    string s = "this is the truth";
    uv_buf_t buffer = uv_buf_init(&s[0], s.length());
    printf("buffer is %zu bytes\n", buffer.len);
    uv_udp_send_t request;
    uv_udp_send(&request, &handle, &buffer, 1, (const struct sockaddr*)&remote,
                sent);
    usleep(300000);
  }

  return 0;
}
