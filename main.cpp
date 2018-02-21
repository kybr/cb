#include <iostream>
#include <string>

#include <uvw.hpp>
#include "cb.hpp"

using namespace std;

#define N (1000)
struct State {
  unsigned n;
  char data[N];
};

int main() {
  auto loop = uvw::Loop::getDefault();
  cb::Broadcaster b;
  cb::Receiver r;
  b.start(loop);
  r.start(loop);

  State* state = new State();
  sprintf(state->data, "this is the truth");
  b.transmit(state);

  return loop->run();
}