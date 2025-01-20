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

#include <mercury/common/errors.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/thread_info.h>
#include <mercury/operating_system/process/tls.h>
#include <mercury/operating_system/threading/thread_lock.h>
#include <mercury/operating_system/threading/stack_alloc.h>

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
  //configureStack(get_sst_hg_tls_thread_id(), stack, get_sst_hg_global_data(), get_sst_hg_tls_data());
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
  size_t stack_mod = ((size_t)stack) % sst_hg_global_stacksize;
  if (stack_mod != 0){
    sst_hg_throw_printf(Hg::ValueError,
        "user space thread stack is not aligned on %llu bytes",
        sst_hg_global_stacksize);
  }

  configureStack(phys_thread_id, stack, globalsMap, tlsMap);

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
//  void** globalsPtr = (void**) &tls[SST_HG_TLS_GLOBAL_MAP];
//  void* globalsMap = *globalsPtr;
//  void** tlsPtr = (void**) &tls[SST_HG_TLS_TLS_MAP];
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

