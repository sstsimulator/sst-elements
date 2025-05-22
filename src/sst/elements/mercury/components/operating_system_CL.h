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

#include <mercury/components/operating_system.h>
#include <mercury/components/node_CL_fwd.h>
#include <sst/core/link.h>
#include <mercury/libraries/compute/compute_event.h>
#include <mercury/libraries/compute/compute_scheduler.h>

namespace SST {
namespace Hg {

extern template SST::TimeConverter HgBase<SST::SubComponent>::time_converter_;

class OperatingSystemCL : public OperatingSystem {

public:

  SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(SST::Hg::OperatingSystemCL, SST::Hg::OperatingSystem, SST::Hg::NodeCL*)

  SST_ELI_REGISTER_SUBCOMPONENT(
    SST::Hg::OperatingSystemCL,
    "hg",
    "OperatingSystemCL",
    SST_ELI_ELEMENT_VERSION(0,0,1),
    "Mercury Operating System using ComputeLibrary",
    SST::Hg::OperatingSystemCL
  )

  /** Functions that block and must complete before returning */
  enum COMP_FUNC {
    COMP_TIME = 67, //the basic compute-for-some-time
    COMP_INSTR,
    COMP_EIGER
  };

  OperatingSystemCL(SST::ComponentId_t id, SST::Params& params, NodeCL* parent);

  ~OperatingSystemCL()
  {
    delete compute_sched_;
  }

  void execute(COMP_FUNC, Event *data, int nthr = 1);

private:
  ComputeScheduler *compute_sched_;
  NodeCL* nodeCL_;
};

} // namespace Hg
} // namespace SST
