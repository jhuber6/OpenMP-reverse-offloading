cmake_minimum_required(VERSION 3.13.4)
project(reverse LANGUAGES C CXX)

if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "[Cc]lang") OR
   (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "17.0.0"))
  message(FATAL_ERROR "Compiler must be at least 'clang 17.0.0'")
endif()

find_package(Threads REQUIRED)

set(COMPILER_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wunused
    -Wnon-virtual-dtor
    -Woverloaded-virtual
    -Wcast-align
    -Wnull-dereference

    -Wno-unknown-cuda-version
    -Wno-gnu-empty-struct
    -Wno-c99-extensions
    -Wno-flexible-array-extensions)

set(OFFLOADING_FLAGS
    -fopenmp-offload-mandatory
    -fopenmp-cuda-mode
    -foffload-lto
    -fopenmp
    --offload-arch=native)

add_executable(reverse main.cpp client.cpp server.cpp)
set_target_properties(reverse PROPERTIES CXX_STANDARD 17)
target_include_directories(reverse PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(reverse PRIVATE ${COMPILER_WARNINGS} ${OFFLOADING_FLAGS})
target_link_options(reverse PRIVATE -Wno-unknown-cuda-version)
target_link_libraries(reverse PRIVATE 
  Threads::Threads
  "--offload-link" 
  omp 
  omptarget 
  omptarget.devicertl)

