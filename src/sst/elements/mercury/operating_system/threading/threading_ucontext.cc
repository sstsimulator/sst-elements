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
