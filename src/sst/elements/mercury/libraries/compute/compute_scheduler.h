// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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

#include <mercury/operating_system/libraries/library.h>
#include <mercury/components/operating_system_CL_fwd.h>
#include <mercury/libraries/compute/compute_scheduler_api.h>
#include <mercury/operating_system/process/thread.h>
#include <list>

namespace SST {
namespace Hg {

//class ComputeScheduler : public ComputeSchedulerAPI, public Library
class ComputeScheduler : public ComputeSchedulerAPI
{
public:
  // SST_ELI_REGISTER_DERIVED(Library, ComputeScheduler, "hg", "ComputeScheduler",
  //                          SST_ELI_ELEMENT_VERSION(1, 0, 0),
  //                          "provides compute scheduler")

  ComputeScheduler(SST::Params& params, SST::Hg::OperatingSystemCL* os);

  ~ComputeScheduler() {}

  int ncores() const override {
    return ncores_;
  }

  int nsockets() const override {
    return nsockets_;
  }

  /**
   * @brief reserve_core
   * @param thr   The physical thread requesting to compute
   */
  void reserveCores(int ncore, Thread* thr) override;

  void releaseCores(int ncore, Thread* thr) override;

private:
  int ncores_;
  int nsockets_;
  int ncore_active_;
  SST::Hg::OperatingSystemCL* os_;
  std::list<std::pair<int,Thread*>> pending_threads_;
};

} // end namespace Hg
} // end namespace SST
