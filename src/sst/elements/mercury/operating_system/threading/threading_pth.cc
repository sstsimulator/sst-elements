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

#include <mercury/common/errors.h>
#include <mercury/operating_system/process/thread_info.h>
#include <mercury/operating_system/threading/threading_interface.h>

#include <pth.h>

namespace SST {
namespace Hg {

class ThreadingPth : public ThreadContext
{
 public:
  SST_ELI_REGISTER_DERIVED(
    ThreadContext,
    ThreadingPth,
    "hg",
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

