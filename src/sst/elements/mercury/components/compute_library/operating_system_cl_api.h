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
#include <mercury/components/compute_library/node_cl_fwd.h>

#include <cstdint>
#include <memory>

namespace SST {
namespace Hg {

extern template SST::TimeConverter HgBase<SST::SubComponent>::time_converter_;

class OperatingSystemCLAPI : public SST::Hg::OperatingSystemAPI {

public:

  /** Functions that block and must complete before returning */
  enum COMP_FUNC {
    COMP_TIME = 67, // the basic compute-for-some-time
    COMP_INSTR,
    COMP_EIGER
  };

  SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(SST::Hg::OperatingSystemCLAPI,
                                            SST::Hg::OperatingSystemAPI)

  OperatingSystemCLAPI(ComponentId_t id, SST::Params &params);

  virtual void execute(COMP_FUNC, Event *data, int nthr = 1) = 0;

  virtual void setParentNode(SST::Hg::NodeCL *parent) = 0;

  virtual NodeCL* nodeCL() const = 0;
};

} // namespace Hg
} // namespace SST
