#include <omp.h>

#include "include/RPC/rpc.h"

int main() {
  int result = 0;
#pragma omp target map(from : result)
  { result = omp_is_initial_device(); }
  return result;
}
