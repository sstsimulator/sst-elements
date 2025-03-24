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

//#include <sstmac/common/sstmac_config.h>
#include <mercury/operating_system/process/tls.h>
#include <mercury/common/unusedvariablemacro.h>

#ifndef SST_HG_INLINE
#ifdef __STRICT_ANSI__
#define SST_HG_INLINE
#else
#define SST_HG_INLINE inline
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern char* static_init_glbls_segment;
extern char* static_init_tls_segment;
void allocate_static_init_glbls_segment();
void allocate_static_init_tls_segment();

#ifdef __cplusplus
}
#endif

SST_HG_MAYBE_UNUSED
static SST_HG_INLINE char* get_sst_hg_global_data(){
  if (sst_hg_global_stacksize == 0){
    if (static_init_glbls_segment == 0){
      allocate_static_init_glbls_segment();
    }
    return static_init_glbls_segment;
  } else {
    char** globalMapPtr = (char**)(get_sst_hg_tls() + SST_HG_TLS_GLOBAL_MAP);
    return *globalMapPtr;
  }
}

SST_HG_MAYBE_UNUSED
static SST_HG_INLINE char* get_sst_hg_tls_data(){
  if (sst_hg_global_stacksize == 0){
    if (static_init_tls_segment == 0){
      allocate_static_init_tls_segment();
    }
    return static_init_tls_segment;
  } else {
    char** globalMapPtr = (char**)(get_sst_hg_tls() + SST_HG_TLS_TLS_MAP);
    return *globalMapPtr;
  }
}

SST_HG_MAYBE_UNUSED
static SST_HG_INLINE int get_sst_hg_tls_thread_id(){
  int* idPtr = (int*)(get_sst_hg_tls() + SST_HG_TLS_THREAD_ID);
  return *idPtr;
}

#undef SST_HG_INLINE
