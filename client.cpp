#include "reverse_offloading.h"

#include <cstdio>
#include <future>
#include <thread>

using namespace __llvm_libc;

rpc::Client client;

void *omp_region_id;

void run_client() {
  uint64_t result;
  rpc::Client::Port port = client.open<EXECUTE>();
  port.send([&](rpc::Buffer *buffer) {
    buffer->data[0] = reinterpret_cast<uintptr_t>(omp_region_id);
  });
  port.recv([&](rpc::Buffer *buffer) { result = buffer->data[0]; });
  port.close();
}
