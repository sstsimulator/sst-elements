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

#include <operating_system/process/thread.h>
#include <operating_system/process/thread_info.h>
#include <operating_system/threading/threading_interface.h>

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

fcontext_transfer_t sstmac_jump_fcontext(fcontext_t const to, void * vp);

fcontext_t sstmac_make_fcontext(void * sp, size_t size, pfn_fcontext corofn);

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
    "macro",
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
    ctx_ = sstmac_make_fcontext(stacktop, sz, start_fcontext_thread);
    args_ = args;
    ctx_ = sstmac_jump_fcontext(ctx_, this).ctx;
  }

  void pauseContext(ThreadContext*  /*to*/) override {
    transfer_ = sstmac_jump_fcontext(transfer_, nullptr).ctx;
  }

  void resumeContext(ThreadContext*  /*from*/) override {
    auto newctx = sstmac_jump_fcontext(ctx_, nullptr).ctx;
    ctx_ = newctx;
  }

  void completeContext(ThreadContext*  /*to*/) override {
    sstmac_jump_fcontext(transfer_, nullptr);
  }

  void jumpContext(ThreadContext*  /*to*/) override {
    sst_hg_abort_printf("error: fcontext interface does not support jump_context feature\n"
                      "must set SSTMAC_THREADING=pth or ucontext");
  }

 private:
  fcontext_t ctx_;
  void* args_;
  void (*fxn_)(void*);
  fcontext_t transfer_;

};

} // end namespace Hg
} // end namespace SST

