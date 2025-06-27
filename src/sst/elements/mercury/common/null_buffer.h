// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// sentinel value that is a reserved address from mmap
// but points to no real data
// used to fake a real pointer, but cannot be accessed
extern void* sst_hg_nullptr;
// for cases in which send/recv buffers cannot alias
// this creates two pointers that won't overlap
extern void* sst_hg_nullptr_send;
extern void* sst_hg_nullptr_recv;
// the maximum pointer in the reserved mmap range
// all pointers between sst_hg_nullptr and this
// are not real data
extern void* sst_hg_nullptr_range_max;

static inline bool isNonNullBuffer(const void* buf){
  if (buf){
    //see if buffer falls in the reserved "null buffer" range
    return ( (buf < sst_hg_nullptr) || (buf >= sst_hg_nullptr_range_max) );
  } else {
    return false;
  }
}

static inline bool isNullBuffer(const void* buf){
  return !(isNonNullBuffer(buf));
}

#ifdef __cplusplus
}
#endif
