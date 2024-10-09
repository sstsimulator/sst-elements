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

#pragma once

#include <operating_system/process/compute_scheduler.h>
#include <operating_system/process/thread.h>

namespace SST {
namespace Hg {

class SimpleComputeScheduler : public ComputeScheduler
{
 public:
  SST_ELI_REGISTER_DERIVED(
    ComputeScheduler,
    SimpleComputeScheduler,
    "hg",
    "simple",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "Compute scheduler that migrates work to any open core")

  SimpleComputeScheduler(SST::Params& params, OperatingSystem* os,
                         int ncore, int nsocket)
    : ComputeScheduler(params, os, ncore, nsocket),
      ncore_active_(0) 
  {}
  
  void reserveCores(int ncore, Thread* thr) override;
  
  void releaseCores(int ncore, Thread* thr) override;

 private:
  std::list<std::pair<int,Thread*>> pending_threads_;
  int ncore_active_;
};

} // end namespace Hg
} // end namespace SST
