#include <cassert>
#include <chrono>
#include <cstdio>
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

  init_client();

  std::atomic<int32_t> run = 0;
  std::thread st(run_server, &run);
  st.detach();

  printf("Running basic functionality\n");
  int32_t results[16] = {0};
#pragma omp target teams num_teams(4) map(from : results[ : 16])
#pragma omp parallel num_threads(4)
  {
    results[omp_get_thread_num() + omp_get_num_teams() * omp_get_team_num()] =
        run_client_basic();
  }

  for (int i = 0; i < 16; ++i)
    if (results[i] != 4)
      printf("Return value %d did not match 4\n", results[i]);

  int repetitions = 10000;
  auto begin = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < repetitions; i++) {
#pragma omp target
    ;
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto latency = end - begin;
  printf("Average latency for kernel launch %ld",
         std::chrono::duration_cast<std::chrono::nanoseconds>(latency).count() /
             repetitions);

  printf("Checking RPC latency\n");
  constexpr int TEAMS = 8;
  for (int i = 1; i <= TEAMS; ++i) {
    auto begin = std::chrono::high_resolution_clock::now();
#pragma omp target teams num_teams(i)
#pragma omp parallel num_threads(lane_size)
    {
      for (int i = 0; i < repetitions; ++i)
        run_client_empty();
    }
    auto end = std::chrono::high_resolution_clock::now();
    printf("Average latency of RPC call relative to kernel launch with %d "
           "teams: %f\n ",
           i,
           static_cast<double>(
               std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin)
                   .count() /
               repetitions) /
               (std::chrono::duration_cast<std::chrono::nanoseconds>(latency)
                    .count() /
                repetitions));
  }

  for (uint64_t size = 1024; size <= 1024 * 1024; size *= 2) {
    uint8_t *data = new uint8_t[size];
    auto begin = std::chrono::high_resolution_clock::now();
#pragma omp target teams num_teams(1) map(to : data[ : size])
#pragma omp parallel num_threads(1)
    {
      for (int i = 0; i < repetitions / 100; ++i)
        streaming(data, size);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> fsec = end - begin;
    printf("Average bandwidth to transfer %ld KiB using the RPC: %f MiB/s\n",
           size / 1024,
           ((size / (1024.0 * 1024.0)) * repetitions / 100) / fsec.count());
  }

  run.store(1);
  omp_free(shared_ptr, llvm_omp_target_shared_mem_alloc);
}
