// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
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

/* start at 64 to avoid conflicts with other libs */
#define SST_HG_TLS_OFFSET 64
#define SST_HG_TLS_THREAD_ID SST_HG_TLS_OFFSET
#define SST_HG_TLS_GLOBAL_MAP SST_HG_TLS_THREAD_ID + sizeof(int)
#define SST_HG_TLS_TLS_MAP SST_HG_TLS_GLOBAL_MAP + sizeof(void*)
#define SST_HG_TLS_SANITY_CHECK SST_HG_TLS_TLS_MAP + sizeof(void*)
#define SST_HG_TLS_IMPLICIT_STATE SST_HG_TLS_SANITY_CHECK + sizeof(int)
#define SST_HG_TLS_END SST_HG_TLS_IMPLICIT_STATE + sizeof(void*)
#define SST_HG_TLS_SIZE (SST_HG_TLS_END - SST_HG_TLS_OFFSET)

#ifndef SST_HG_INLINE
#ifdef __STRICT_ANSI__
#define SST_HG_INLINE
#else
#define SST_HG_INLINE inline
#endif
#endif

//
#include <cstdint>
extern "C" int sst_hg_global_stacksize;
//#else
//#include <stdint.h>
//extern int sst_hg_global_stacksize;
//#endif


static SST_HG_INLINE uintptr_t get_sst_hg_tls(){
  int stack; int* stackPtr = &stack;
  uintptr_t localStorage = ((uintptr_t) stackPtr/sst_hg_global_stacksize)*sst_hg_global_stacksize;
  return localStorage;
}
