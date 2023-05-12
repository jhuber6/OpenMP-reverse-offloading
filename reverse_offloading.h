#ifndef REVERSE_OFFLOADING_H
#define REVERSE_OFFLOADING_H

#include "include/RPC/rpc.h"
#include <future>

extern __llvm_libc::rpc::Server server;

extern __llvm_libc::rpc::Client client;
#pragma omp declare target to(client) device_type(nohost)

void run_server(std::future<void> run);

#endif
