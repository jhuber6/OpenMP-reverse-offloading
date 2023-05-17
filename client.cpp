#include "reverse_offloading.h"

#include <cstdio>
#include <future>
#include <omp.h>
#include <thread>

using namespace __llvm_libc;

rpc::Client client;

void foo(int32_t thread_id, int32_t team_id, int32_t num_teams, int32_t *x,
         const char *str) {
  printf("Hello from thread %d and team %d!\n", thread_id, team_id);
  printf("\tDevice string: %s\n", str);
  *x = thread_id + team_id * num_teams;
}

struct foo_args {
  int32_t thread_id;
  int32_t team_id;
  int32_t num_teams;
  int32_t *x;
  void *str;
};

void omp_outlined(void *args, uint32_t id) {
  foo_args *fn_args = reinterpret_cast<foo_args *>(args);
  void *x = omp_map_lookup(fn_args->x, id);
  void *ptr = omp_map_lookup(fn_args->str, id);
  foo(fn_args->thread_id, fn_args->team_id, fn_args->num_teams,
      reinterpret_cast<int32_t *>(x), reinterpret_cast<const char *>(ptr));
}

void empty() {}

void omp_outlined_1(void *, uint32_t) { empty(); }

#pragma omp begin declare target
void *omp_device_ref;
void *omp_device_ref_1;
#pragma omp end declare target

void init_client() {
  omp_device_ref = reinterpret_cast<void *>(&omp_outlined);
#pragma omp target update to(omp_device_ref)
  omp_device_ref_1 = reinterpret_cast<void *>(&omp_outlined_1);
#pragma omp target update to(omp_device_ref_1)
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

void map_to_host(const void *data, uint64_t size) {
  rpc::Client::Port port = client.open<COPY_TO>();
  port.send_n(data, size);
  port.send([=](rpc::Buffer *buffer) {
    buffer->data[0] = reinterpret_cast<uintptr_t>(data);
  });
  port.close();
}
#pragma omp declare target to(map_to_host) device_type(nohost)

void map_from_host(void *data, uint64_t size) {
  rpc::Client::Port port = client.open<COPY_FROM>();
  port.send([=](rpc::Buffer *buffer) {
    buffer->data[0] = reinterpret_cast<uintptr_t>(data);
    buffer->data[1] = reinterpret_cast<uint64_t>(size);
  });
  port.recv_n(&data, &size, [&](uint64_t) { return data; });
  port.close();
}
#pragma omp declare target to(map_from_host) device_type(nohost)

static uint64_t strlen(const char *str) {
  uint64_t i = 0;
  while (str[i] != '\0')
    ++i;
  return i;
}
#pragma omp declare target to(strlen) device_type(nohost)

int run_client_basic() {
  static const char *strs[] = {
      "one", "two", "three",
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!?"};
  const char *str =
      strs[(omp_get_thread_num() + omp_get_num_teams() * omp_get_team_num()) %
           (sizeof(strs) / sizeof(char *))];
  int32_t x = 0;
  map_to_host(str, strlen(str));
  map_to_host(&x, sizeof(int32_t));
  foo_args args = {omp_get_thread_num(), omp_get_team_num(),
                   omp_get_num_teams(), &x,
                   const_cast<void *>(reinterpret_cast<const void *>(str))};
  omp_reverse_offload(omp_device_ref, &args, sizeof(foo_args));
  map_from_host(&x, sizeof(int32_t));
  return x;
}
#pragma omp declare target to(run_client_basic) device_type(nohost)

void run_client_empty() { omp_reverse_offload(omp_device_ref_1, nullptr, 0); }
#pragma omp declare target to(run_client_empty) device_type(nohost)
