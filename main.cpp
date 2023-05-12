#include <cstdio>
#include <future>
#include <omp.h>
#include <thread>

#include "include/RPC/rpc.h"

#include "reverse_offloading.h"

using namespace __llvm_libc;

uint32_t lane_size = gpu::get_lane_size();
#pragma omp declare target to(lane_size)

int main() {
#pragma omp target update from(lane_size)

  uint64_t rpc_buffer_size =
      rpc::Server::allocation_size(rpc::DEFAULT_PORT_COUNT, lane_size);

  void *shared_ptr =
      omp_alloc(rpc_buffer_size, llvm_omp_target_shared_mem_alloc);

  server.reset(rpc::DEFAULT_PORT_COUNT, lane_size, shared_ptr);
#pragma omp target is_device_ptr(shared_ptr)
  { client.reset(rpc::DEFAULT_PORT_COUNT, lane_size, shared_ptr); }

  std::promise<void> run;
  std::thread st(run_server, run.get_future());
  st.detach();

  uint64_t result = 0;
#pragma omp target map(from : result)
  {
    rpc::Client::Port port = client.open<0>();
    port.send([&](rpc::Buffer *buffer) { buffer->data[0] = 0xdeadbeef; });
    port.recv([&](rpc::Buffer *buffer) { result = buffer->data[0]; });
  }

  printf("Client got: %lx\n", result);
  run.set_value();
  omp_free(shared_ptr, llvm_omp_target_shared_mem_alloc);
}