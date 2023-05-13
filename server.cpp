#include "reverse_offloading.h"

#include <cstdio>
#include <future>
#include <thread>

using namespace __llvm_libc;

rpc::Server server;

void run_server(std::future<void> run) {
  while (run.wait_for(std::chrono::nanoseconds(256)) ==
         std::future_status::timeout) {
    auto port = server.try_open();
    if (!port)
      continue;

    switch (port->get_opcode()) {
    case Opcode::EXECUTE: {
      uint64_t args_sizes[rpc::MAX_LANE_SIZE] = {0};
      void *args[rpc::MAX_LANE_SIZE] = {0};
      port->recv_n(args, args_sizes,
                   [](uint64_t size) { return new char[size]; });
      port->recv_and_send([&](rpc::Buffer *buffer, uint32_t id) {
        auto fn = reinterpret_cast<int (*)(void *, uint64_t)>(buffer->data[0]);
        buffer->data[0] = fn(args[id], args_sizes[id]);
      });
      break;
    }
    default:
      port->recv([](rpc::Buffer *) {});
      break;
    }

    port->close();
  }
}
