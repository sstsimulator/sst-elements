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

#include <mercury/operating_system/process/thread.h>
#include <mercury/operating_system/process/thread_info.h>
#include <mercury/operating_system/threading/threading_interface.h>

#include <iostream>
#include <stdint.h>
#include <stddef.h>

extern "C" {

typedef void* fcontext_t;

struct fcontext_transfer_t
{
    fcontext_t ctx;
    void* data;
};

struct fcontext_stack_t
{
    void* sptr;
    size_t ssize;
};

typedef void (*pfn_fcontext)(fcontext_transfer_t);

fcontext_transfer_t sst_hg_jump_fcontext(fcontext_t const to, void * vp);

fcontext_t sst_hg_make_fcontext(void * sp, size_t size, pfn_fcontext corofn);

} //end extern C

namespace SST {
namespace Hg {

class ThreadingFContext : public ThreadContext
{
 private:
  static void start_fcontext_thread(fcontext_transfer_t t)
  {
    ThreadingFContext* fctx = (ThreadingFContext*) t.data;
    fctx->transfer_ = t.ctx;
    (*fctx->fxn_)(fctx->args_);
  }

 public:
  SST_ELI_REGISTER_DERIVED(
    ThreadContext,
    ThreadingFContext,
    "hg",
    "fcontext",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "uses fcontext for fast context switching")

  ~ThreadingFContext() override {}

  ThreadingFContext(){}

  ThreadContext* copy() const override {
    //parameters never actually used
    return new ThreadingFContext;
  }

  void initContext() override {}

  void destroyContext() override {}

  void startContext(void *stack, size_t sz,
      void (*func)(void*), void *args,
      ThreadContext*  /*from*/) override {
    fxn_ = func;
    void* stacktop = (char*) stack + sz;
    ctx_ = sst_hg_make_fcontext(stacktop, sz, start_fcontext_thread);
    args_ = args;
    ctx_ = sst_hg_jump_fcontext(ctx_, this).ctx;
  }

  void pauseContext(ThreadContext*  /*to*/) override {
    transfer_ = sst_hg_jump_fcontext(transfer_, nullptr).ctx;
  }

  void resumeContext(ThreadContext*  /*from*/) override {
    auto newctx = sst_hg_jump_fcontext(ctx_, nullptr).ctx;
    ctx_ = newctx;
  }

  void completeContext(ThreadContext*  /*to*/) override {
    sst_hg_jump_fcontext(transfer_, nullptr);
  }

  void jumpContext(ThreadContext*  /*to*/) override {
    sst_hg_abort_printf("error: fcontext interface does not support jump_context feature\n"
                      "must set SST_HG_THREADING=pth or ucontext");
  }

 private:
  fcontext_t ctx_;
  void* args_;
  void (*fxn_)(void*);
  fcontext_t transfer_;

};

} // end namespace Hg
} // end namespace SST

