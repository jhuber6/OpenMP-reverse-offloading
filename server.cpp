#include "reverse_offloading.h"

#include <cstdio>
#include <future>
#include <map>
#include <thread>

using namespace __llvm_libc;

static std::map<uintptr_t, uintptr_t> pointer_map;

rpc::Server server;

void *omp_map_lookup(void *in, uint32_t) {
  return reinterpret_cast<void *>(pointer_map[reinterpret_cast<uintptr_t>(in)]);
}

void run_server(std::atomic<int32_t> *run) {
  for (;;) {
    if (run->load())
      return;
    auto port = server.try_open();
    if (!port)
      continue;

    switch (port->get_opcode()) {
    case Opcode::EXECUTE: {
      uint64_t args_sizes[rpc::MAX_LANE_SIZE] = {0};
      void *args[rpc::MAX_LANE_SIZE] = {0};
      port->recv_n(args, args_sizes,
                   [](uint64_t size) { return new char[size]; });
      bool nowait = false;
      port->recv([&](rpc::Buffer *buffer, uint32_t id) {
        auto fn = reinterpret_cast<int (*)(void *, uint64_t)>(buffer->data[0]);
        buffer->data[0] = fn(args[id], id);
        nowait = buffer->data[1];
      });
      if (!nowait)
        port->send([&](rpc::Buffer *) {});
      break;
    }
    case Opcode::COPY_TO: {
      uint64_t sizes[rpc::MAX_LANE_SIZE] = {0};
      void *data[rpc::MAX_LANE_SIZE] = {0};
      port->recv_n(data, sizes, [](uint64_t size) { return new char[size]; });
      port->recv([&](rpc::Buffer *buffer, uint32_t id) {
        pointer_map[reinterpret_cast<uintptr_t>(buffer->data[0])] =
            reinterpret_cast<uintptr_t>(data[id]);
      });
      break;
    }
    case Opcode::COPY_FROM: {
      uint64_t sizes[rpc::MAX_LANE_SIZE] = {0};
      void *data[rpc::MAX_LANE_SIZE] = {0};
      port->recv([&](rpc::Buffer *buffer, uint32_t id) {
        data[id] = reinterpret_cast<void *>(
            pointer_map[reinterpret_cast<uintptr_t>(buffer->data[0])]);
        sizes[id] = buffer->data[1];
      });
      port->send_n(data, sizes);
      break;
    }
    case Opcode::STREAM: {
      uint64_t sizes[rpc::MAX_LANE_SIZE] = {0};
      void *data[rpc::MAX_LANE_SIZE] = {0};
      port->recv_n(data, sizes, [](uint64_t size) { return new char[size]; });
      for (uint64_t i = 0; i < rpc::MAX_LANE_SIZE; ++i)
        if (data[i])
          delete[] reinterpret_cast<uint8_t *>(data[i]);
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
