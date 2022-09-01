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
                    "with multithreaded SST - must have ucontext or Boost::context");
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
