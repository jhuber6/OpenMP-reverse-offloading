#ifndef REVERSE_OFFLOADING_H
#define REVERSE_OFFLOADING_H

#include "include/RPC/rpc.h"
#include <future>

enum Opcode : uint16_t {
  NOOP = 0,
  EXECUTE = 1,
  COPY = 2,
};

extern __llvm_libc::rpc::Server server;

void *omp_map_lookup(void *in, uint32_t id);

void run_server(std::future<void> run);

extern __llvm_libc::rpc::Client client;
#pragma omp declare target to(client) device_type(nohost)

void init_client();

void run_client();
#pragma omp declare target to(run_client) device_type(nohost)

#endif
