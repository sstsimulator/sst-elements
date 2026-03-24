// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S. Government retains certain
// rights in this software.
//
// Copyright (c) 2009-2025, NTESS. All rights reserved.

#include <mercury/operating_system/libraries/pthread/hg_pthread_runner.h>
#include <mercury/operating_system/process/app.h>

namespace SST {
namespace Hg {

HgPthreadRunner::HgPthreadRunner(SoftwareId id, App* parent,
                                 start_fxn start_routine, void* arg,
                                 OperatingSystemAPI* os,
                                 int detach_state)
  : Thread(parent->params(), id, os),
    start_routine_(start_routine),
    arg_(arg)
{
  parent_app_ = parent;
  if (detach_state == HG_PTHREAD_CREATE_DETACHED) {
    setDetachState(DETACHED);
  } else {
    setDetachState(JOINABLE);
  }
}

void
HgPthreadRunner::run()
{
  start_routine_(arg_);
}

} // namespace Hg
} // namespace SST
