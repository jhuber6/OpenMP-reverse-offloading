//===-- Compile time architecture detection ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SUPPORT_MACROS_PROPERTIES_ARCHITECTURES_H
#define LLVM_LIBC_SUPPORT_MACROS_PROPERTIES_ARCHITECTURES_H

#if defined(__AMDGPU__)
#define LIBC_TARGET_ARCH_IS_AMDGPU
#endif

#if defined(__NVPTX__)
#define LIBC_TARGET_ARCH_IS_NVPTX
#endif

#if defined(LIBC_TARGET_ARCH_IS_NVPTX) || defined(LIBC_TARGET_ARCH_IS_AMDGPU)
#define LIBC_TARGET_ARCH_IS_GPU
#endif

#endif
