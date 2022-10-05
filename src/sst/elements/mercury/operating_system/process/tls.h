/**
Copyright 2009-2021 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2021, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

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
