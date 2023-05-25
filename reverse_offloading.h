#ifndef REVERSE_OFFLOADING_H
#define REVERSE_OFFLOADING_H

#include "include/RPC/rpc.h"
#include <atomic>

enum Opcode : uint16_t {
  NOOP = 0,
  EXECUTE = 1,
  COPY_TO = 2,
  COPY_FROM = 3,
  STREAM = 4,
};

extern __llvm_libc::rpc::Server server;

void *omp_map_lookup(void *in, uint32_t id);

void run_server(std::atomic<int32_t> *run);

extern __llvm_libc::rpc::Client client;
#pragma omp declare target to(client) device_type(nohost)

void init_client();

void run_client_basic(int *x);
#pragma omp declare target to(run_client_basic) device_type(nohost)

void run_client_empty();
#pragma omp declare target to(run_client_empty) device_type(nohost)

void streaming(void *data, uint64_t size);
#pragma omp declare target to(streaming) device_type(nohost)

#endif
