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

#include <sstmac_config.h>

#ifdef SSTMAC_HAVE_UCONTEXT

#include <sstmac/software/process/thread_info.h>
#include <sstmac/software/threading/threading_interface.h>
#include <sstmac/software/threading/context_util.h>
#include <ucontext.h>

namespace sstmac {
namespace sw {

class ThreadingUContext : public ThreadContext
{
 public:
  SST_ELI_REGISTER_DERIVED(
    ThreadContext,
    ThreadingUContext,
    "macro",
    "ucontext",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "uses ucontext for portable linux context switching")

  ThreadingUContext(){}

  void initContext() override {
    if (getcontext(&context_) != 0) {
      spkt_abort_printf("ThreadingUContext::init_context: %s", ::strerror(errno));
    }
  }

  ThreadContext* copy() const override {
    return new ThreadingUContext;
  }


  void destroyContext() override {}

  void startContext(void* stack, size_t stacksize,
                     void(*func)(void*), void *args,
                     ThreadContext* from) override {

    funcptr funcp(func);
    voidptr voidp(args);
    context_.uc_stack.ss_sp = stack;
    context_.uc_stack.ss_size = stacksize;
    initContext();

    context_.uc_link = NULL;

    makecontext(&context_, (void
          (*)()) (context_springboard), 4, funcp.fpair.a, funcp.fpair.b,
          voidp.vpair.a, voidp.vpair.b);

    swapContext(from, this);
  }

  void pauseContext(ThreadContext* to) override {
    swapContext(this, to);
  }

  void resumeContext(ThreadContext* from) override {
    swapContext(from, this);
  }

  void completeContext(ThreadContext* to) override {
    swapContext(this, to);
  }

 private:
  static void swapContext(ThreadContext* from, ThreadContext* to) {
    ThreadingUContext* fromctx = static_cast<ThreadingUContext*>(from);
    ThreadingUContext* toctx = static_cast<ThreadingUContext*>(to);
    if (swapcontext(&fromctx->context_, &toctx->context_) == -1) {
      spkt_abort_printf("ThreadingUContext::swapContext: %s", strerror(errno));
    }
  }

  ucontext_t context_;
};

}
}

#endif
