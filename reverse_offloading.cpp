#include "reverse_offloading.h"

#include <cstdio>
#include <future>
#include <thread>

using namespace __llvm_libc;

rpc::Server server;

rpc::Client client;
#pragma omp declare target to(client) device_type(nohost)

void run_server(std::future<void> run) {
  while (run.wait_for(std::chrono::nanoseconds(256)) ==
         std::future_status::timeout) {
    auto port = server.try_open();
    if (!port)
      continue;

    switch (port->get_opcode()) {
    default:
      port->recv([](rpc::Buffer *buffer) {
        printf("Server got: %lx\n", buffer->data[0]);
      });
      port->send([](rpc::Buffer *buffer) { buffer->data[0] = 0xfeed5eed; });
    }
  }
}
