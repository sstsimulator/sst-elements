// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <common/errors.h>
#include <operating_system/threading/context_util.h>
#include <operating_system/threading/threading_interface.h>
#include <operating_system/threading/thread_lock.h>

#include <vector>

namespace SST {
namespace Hg {

//string is name, bool is whether valid with multithreading
static std::vector<std::pair<std::string,bool>> valid_threading_contexts;

static void fill_valid_threading_contexts(std::vector<std::pair<std::string,bool>>& contexts)
{
  contexts.emplace_back("fcontext", true);
#ifdef SSTMAC_HAVE_UCONTEXT
  contexts.emplace_back("ucontext", true);
#endif
#ifdef SSTMAC_HAVE_GNU_PTH
  contexts.emplace_back("pth", false);
#endif
#ifdef SSTMAC_HAVE_PTHREAD
  contexts.emplace_back("pthread", false);
#endif

  if (contexts.empty()){
    Hg::abort("No valid threading contexts found");
  }
}

std::string
ThreadContext::defaultThreading()
{
  static thread_lock fill_lock;
  fill_lock.lock();
  if (valid_threading_contexts.empty()){
    fill_valid_threading_contexts(valid_threading_contexts);
  }
  fill_lock.unlock();

  std::string default_threading;
#if SSTMAC_USE_MULTITHREAD
  for (auto& pair : valid_threading_contexts){
    if (pair.second){ //supports multithreading
      default_threading = pair.first;
      break;
    }
  }
  if (default_threading.size() == 0){
    //this did not get updated - so we don't have any multithreading-compatible thread interfaces
    sprockit::abort("OperatingSystem: there are no threading frameworks compatible "
                    "with multithreaded SST - must have ucontext");
  }
#else
  default_threading = valid_threading_contexts[0].first;
#endif
  const char *threading_pchar = getenv("SST_HG_THREADING");
  if (threading_pchar){
    default_threading = threading_pchar;
  }

  return default_threading;
}


// Intermediary to get around the brain-damaged prototype for makecontext.
void context_springboard(int func_ptr_a, int func_ptr_b,
                         int arg_ptr_a,  int arg_ptr_b)
{
  funcptr fptr(func_ptr_a, func_ptr_b);
  voidptr vptr(arg_ptr_a, arg_ptr_b);
  fptr.call(vptr);
}

} // end of namespace Hg
} // end of namespace SST
