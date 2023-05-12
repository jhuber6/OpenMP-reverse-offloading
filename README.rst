====================================================================
OpenMP reverse offloading using shared memory remote procedure calls
====================================================================

This repository is a demo of how we can achieve reverse offloading in OpenMP 
using a shared memory remote procedure call mechanism. That is, we can execute 
functions on the host originating from the device. This uses a standalone 
version of the RPC library provided by the LLVM libc project at time of writing.

.. contents::
  :local:

Building and Running
--------------------

To build, simply run CMake and set the compiler to use a ``clang`` that is at 
least compatible with version ``17.0.0``.

.. code-block:: shell
  
  $ cmake ../ -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
  $ make -j
