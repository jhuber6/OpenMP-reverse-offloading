#include "reverse_offloading.h"

#include <cstdio>
#include <future>
#include <omp.h>
#include <thread>

using namespace __llvm_libc;

rpc::Client client;

void foo(int32_t thread_id, int32_t team_id) {
  printf("Hello from thread %d and team %d!\n", thread_id, team_id);
}

struct foo_args {
  int32_t thread_id;
  int32_t block_id;
};

void omp_outlined(void *args, uint64_t) {
  foo_args *fn_args = reinterpret_cast<foo_args *>(args);
  foo(fn_args->thread_id, fn_args->block_id);
}

#pragma omp begin declare target
void *omp_device_ref;
#pragma omp end declare target

void init_client() {
  omp_device_ref = reinterpret_cast<void *>(&omp_outlined);
#pragma omp target update to(omp_device_ref)
}

void omp_reverse_offload(void *fn, void *args, uint64_t args_size) {
  rpc::Client::Port port = client.open<EXECUTE>();
  port.send_n(args, args_size);
  port.send_and_recv(
      [&](rpc::Buffer *buffer) {
        buffer->data[0] = reinterpret_cast<uintptr_t>(fn);
      },
      [](rpc::Buffer *) {});
  port.close();
}
#pragma omp declare target to(omp_reverse_offload) device_type(nohost)

void run_client() {
  foo_args args = {omp_get_thread_num(), omp_get_team_num()};
  omp_reverse_offload(omp_device_ref, &args, sizeof(foo_args));
}
#pragma omp declare target to(run_client) device_type(nohost)
