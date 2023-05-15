#include "reverse_offloading.h"

#include <cstdio>
#include <future>
#include <map>
#include <thread>

using namespace __llvm_libc;

static std::map<std::pair<uintptr_t, uint32_t>, uintptr_t> pointer_map;

rpc::Server server;

void *omp_map_lookup(void *in, uint32_t id) {
  return reinterpret_cast<void *>(
      pointer_map[std::make_pair(reinterpret_cast<uintptr_t>(in), id)]);
}

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
        buffer->data[0] = fn(args[id], id);
      });
      break;
    }
    case Opcode::COPY: {
      uint64_t sizes[rpc::MAX_LANE_SIZE] = {0};
      void *data[rpc::MAX_LANE_SIZE] = {0};
      port->recv_n(data, sizes, [](uint64_t size) { return new char[size]; });
      port->recv([&](rpc::Buffer *buffer, uint32_t id) {
        pointer_map[std::make_pair(reinterpret_cast<uintptr_t>(buffer->data[0]),
                                   id)] = reinterpret_cast<uintptr_t>(data[id]);
      });
      break;
    }
    default: {
      port->recv([](rpc::Buffer *) {});
      break;
    }
    }

    port->close();
  }
}
