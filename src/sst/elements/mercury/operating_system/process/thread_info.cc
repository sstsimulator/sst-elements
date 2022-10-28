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

#include <common/errors.h>
#include <components/operating_system.h>
#include <operating_system/process/thread_info.h>
#include <operating_system/process/tls.h>
#include <operating_system/threading/thread_lock.h>
#include <operating_system/threading/stack_alloc.h>
//#include <process/global.h>
//#include <common/sstmac_config.h>
//#include <sprockit/thread_safe_new.h>
//#include <sstmac/skeleton_tls.h>

#include <iostream>
#include <string.h>
#include <stdint.h>
#include <list>

namespace SST {
namespace Hg {

extern template class  HgBase<SST::Component>;
extern template class  HgBase<SST::SubComponent>;

//#if (SST_HG_TLS_OFFSET != SPKT_TLS_OFFSET)
//#error sprockit and sstmac do not agree on stack TLS offset
//#endif

static const int tls_sanity_check = 42042042;

static thread_lock globals_lock;

static void configureStack(int thread_id, void* stack, void* globals, void* tlsMap)
{
  //essentially treat this as thread-local storage
  char* tls = (char*) stack;
  int* thr_id_ptr = (int*) &tls[SST_HG_TLS_THREAD_ID];
  *thr_id_ptr = thread_id;

  int* sanity_ptr = (int*) &tls[SST_HG_TLS_SANITY_CHECK];
  *sanity_ptr = tls_sanity_check;

  //this is dirty - so dirty, but it works
  void** globalPtr = (void**) &tls[SST_HG_TLS_GLOBAL_MAP];
  *globalPtr = globals;

  void** tlsPtr = (void**) &tls[SST_HG_TLS_TLS_MAP];
  *tlsPtr = tlsMap;

  void** statePtr = (void**) &tls[SST_HG_TLS_IMPLICIT_STATE];
  *statePtr = nullptr;
}

extern "C" void* sst_hg_alloc_stack(int sz, int md_sz)
{
  if (md_sz >= SST_HG_TLS_OFFSET){
    sst_hg_abort_printf("Cannot have stack metadata larger than %d - requested %d",
                      SST_HG_TLS_OFFSET, md_sz);
  }
  if (sz > SST::Hg::OperatingSystem::stacksize()){
    sst_hg_abort_printf("Cannot allocate stack larger than %d - requested %d",
                      SST::Hg::OperatingSystem::stacksize(), sz);
  }
  void* stack = SST::Hg::StackAlloc::alloc();
  //configureStack(get_sstmac_tls_thread_id(), stack, get_sstmac_global_data(), get_sstmac_tls_data());
  return stack;
}

extern "C" void sst_hg_free_stack(void* ptr)
{
  abort("sst_hg_free_stack");
//  SST::Hg::StackAlloc::free(ptr);
}

void
ThreadInfo::registerUserSpaceVirtualThread(int phys_thread_id, void *stack,
                                           void* globalsMap, void* tlsMap,
                                           bool isAppStartup,
                                           bool isThreadStartup)
{
  //abort("abort ThreadInfo::registerUserSpaceVirtualThread");
//  size_t stack_mod = ((size_t)stack) % sst_hg_global_stacksize;
//  if (stack_mod != 0){
//    spkt_throw_printf(sprockit::ValueError,
//        "user space thread stack is not aligned on %llu bytes",
//        sst_hg_global_stacksize);
//  }

//  configureStack(phys_thread_id, stack, globalsMap, tlsMap);

//  //there is no parent user-space thread...
//  static std::vector<char> fake_globals(1e6);
//  static std::vector<char> fake_tls(1e6);
//  void** currentGlobalsPtr = (void**) &fake_globals[0];
//  void** currentTlsPtr = (void**) &fake_tls[0];

//  globals_lock.lock();
//  if (globalsMap && isAppStartup){
//    void* currentGlobals = *currentGlobalsPtr;
//    *currentGlobalsPtr = globalsMap;
//    GlobalVariable::glblCtx.addActiveSegment(globalsMap);
//    GlobalVariable::glblCtx.callInitFxns(globalsMap);
//    *currentGlobalsPtr = currentGlobals;
//  }

//  if (tlsMap && isThreadStartup){
//    void* currentTls = *currentTlsPtr;
//    *currentTlsPtr = tlsMap;
//    GlobalVariable::tlsCtx.addActiveSegment(tlsMap);
//    GlobalVariable::tlsCtx.callInitFxns(tlsMap);
//    *currentTlsPtr = currentTls;
//  }
//  globals_lock.unlock();
}

void
ThreadInfo::deregisterUserSpaceVirtualThread(void* stack)
{
//  globals_lock.lock();
//  char* tls = (char*) stack;
//  void** globalsPtr = (void**) &tls[SSTMAC_TLS_GLOBAL_MAP];
//  void* globalsMap = *globalsPtr;
//  void** tlsPtr = (void**) &tls[SSTMAC_TLS_TLS_MAP];
//  void* tlsMap = *tlsPtr;
//  if (globalsMap){
//    GlobalVariable::glblCtx.removeActiveSegment(globalsMap);
//  }
//  if (tlsMap){
//    GlobalVariable::tlsCtx.removeActiveSegment(tlsMap);
//  }
//  globals_lock.unlock();
}

} // end namespace Hg
} // end namespace SST

