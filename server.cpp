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
      uintptr_t ptr;
      port->recv([&](rpc::Buffer *buffer) {
        ptr = reinterpret_cast<uintptr_t>(buffer->data[0]);
      });
      auto fn = reinterpret_cast<int (*)()>(ptr);
      auto result = fn();
      port->send([&](rpc::Buffer *buffer) { buffer->data[0] = result; });
      break;
    }
    default:
      port->recv([](rpc::Buffer *) {});
      break;
    }

    port->close();
  }
}
