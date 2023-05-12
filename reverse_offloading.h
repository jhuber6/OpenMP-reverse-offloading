#ifndef REVERSE_OFFLOADING_H
#define REVERSE_OFFLOADING_H

#include "include/RPC/rpc.h"
#include <future>

enum Opcode : uint16_t {
  NOOP = 0,
  EXECUTE = 1,
};

extern __llvm_libc::rpc::Server server;

void run_server(std::future<void> run);

extern __llvm_libc::rpc::Client client;
#pragma omp declare target to(client) device_type(nohost)

extern void *omp_region_id;
#pragma omp declare target to(omp_region_id)

void run_client();
#pragma omp declare target to(run_client) device_type(nohost)

#endif
