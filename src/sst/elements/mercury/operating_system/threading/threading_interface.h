// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#pragma once

#include <sst/core/component.h>

#include <errno.h>
#include <cstring>
#include <iostream>

namespace SST {
namespace Hg {

/**
 * @brief The thread_context class
 */
class ThreadContext
{
 public:
  SST_ELI_DECLARE_BASE(ThreadContext)
  SST_ELI_DECLARE_DEFAULT_INFO()
  SST_ELI_DECLARE_DEFAULT_CTOR()

  virtual ~ThreadContext() {}

  virtual ThreadContext* copy() const = 0;

  virtual void initContext() = 0;

  virtual void destroyContext() = 0;

  /**
   * @brief start_context
   * @param physical_thread_id  An optional ID for
   * @param stack
   * @param stacksize
   * @param args
   * @param globals_storage
   * @param from
   */
  virtual void startContext(void *stack, size_t stacksize,
                void (*func)(void*), void *args,
                ThreadContext* from) = 0;

  virtual void resumeContext(ThreadContext* from) = 0;

  virtual void pauseContext(ThreadContext* to) = 0;

  /**
   * @brief jump_context
   * Jump directly from one context to another.
   * This bypasses the safety of the start/pause/resume pattern
   * @param to
   */
  virtual void jumpContext(ThreadContext* to){
    to->resumeContext(this);
  }

  /**
   * @brief complete_context Perform all cleanup operations to end this context
   * @param to
   */
  virtual void completeContext(ThreadContext* to) = 0;

  static std::string defaultThreading();

 protected:
  ThreadContext() {}

};

} // end of namespace Hg
} // end of namespace SST

