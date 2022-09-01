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
#include <operating_system/process/thread_info.h>
#include <operating_system/threading/threading_interface.h>

#include <pth.h>

namespace SST {
namespace Hg {

class ThreadingPth : public ThreadContext
{
 public:
  SST_ELI_REGISTER_DERIVED(
    ThreadContext,
    ThreadingPth,
    "macro",
    "pth",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "uses GNU Pth for fast context switching")

  /** nothing */
  ThreadingPth()
  {
  }

  virtual ~ThreadingPth() {}

  ThreadContext* copy() const override {
    return new ThreadingPth;
  }

  void initContext() override {
    if (pth_uctx_create(&context_) != TRUE) {
      spkt_throw_printf(sprockit::OSError,
          "threading_pth::init_context: %s",
          ::strerror(errno));
    }
  }

  void destroyContext() override {
    if (pth_uctx_destroy(context_) != TRUE) {
        spkt_throw_printf(sprockit::OSError,
          "threading_pth::destroy_context: %s",
          ::strerror(errno));
    }
  }

  void startContext(void *stack, size_t stacksize,
                     void(*func)(void*), void *args, ThreadContext* from) override {
    if (stacksize < (16384)) {
      sprockit::abort("threading_pth::start_context: PTH does not accept stacks smaller than 16KB");
    }
    initContext();
    int retval = pth_uctx_make(context_, (char*) stack, stacksize, NULL, func, args, NULL);
    if (retval != TRUE) {
      spkt_throw_printf(sprockit::OSError,
          "threading_pth::start_context: %s",
          ::strerror(errno));
    }
    resumeContext(from);
  }

  void resumeContext(ThreadContext* from) override {
    ThreadingPth* frompth = static_cast<ThreadingPth*>(from);
    if (pth_uctx_switch(frompth->context_, context_) != TRUE) {
      spkt_throw_printf(sprockit::OSError,
        "threading_pth::swap_context: %s",
        strerror(errno));
    }
  }

  void pauseContext(ThreadContext *to) override {
    ThreadingPth* topth = static_cast<ThreadingPth*>(to);
    if (pth_uctx_switch(context_, topth->context_) != TRUE) {
      spkt_throw_printf(sprockit::OSError,
        "threading_pth::swap_context: %s",
        strerror(errno));
    }
  }

  void completeContext(ThreadContext *to) override {
    pauseContext(to);
  }

  typedef pth_uctx_t threadcontext_t;

 private:
  threadcontext_t context_;
};

} // end namespace Hg
} // end namespace SST

