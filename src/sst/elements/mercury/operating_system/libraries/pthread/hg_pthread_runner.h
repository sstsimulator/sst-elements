// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S. Government retains certain
// rights in this software.
//
// Copyright (c) 2009-2025, NTESS. All rights reserved.

#ifndef SST_HG_LIBRARIES_PTHREAD_HG_PTHREAD_RUNNER_H
#define SST_HG_LIBRARIES_PTHREAD_RUNNER_H

#include <mercury/operating_system/process/thread.h>
#include <mercury/operating_system/process/software_id.h>
#include <mercury/operating_system/libraries/pthread/hg_pthread_impl.h>

namespace SST {
namespace Hg {

class HgPthreadRunner : public Thread
{
 public:
  typedef void* (*start_fxn)(void*);

  HgPthreadRunner(SoftwareId id, App* parent,
                  start_fxn start_routine, void* arg,
                  OperatingSystemAPI* os,
                  int detach_state);

  void run() override;

 private:
  start_fxn start_routine_;
  void* arg_;
};

} // namespace Hg
} // namespace SST

#endif /* SST_HG_LIBRARIES_PTHREAD_HG_PTHREAD_RUNNER_H */
