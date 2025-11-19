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

#include <mercury/common/component.h>
#include <sst/core/timeConverter.h>
#include <mercury/components/node_base.h>
#include <mercury/components/compute_library/operating_system_cl_api.h>
#include <mercury/libraries/compute/instruction_processor.h>
#include <mercury/libraries/compute/memory_model.h>
#include <cstdint>
#include <memory>

namespace SST {
namespace Hg {

class NodeCL : public NodeBase {

public:
  /*
   *  SST Registration macros register Components with the SST Core and
   *  document their parameters, ports, etc.
   *  SST_ELI_REGISTER_COMPONENT is required, the documentation macros
   *  are only required if relevant
   */
  // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
  SST_ELI_REGISTER_COMPONENT(
      SST::Hg::NodeCL,      // Component class
      "hg", // Component library (for Python/library lookup)
      "NodeCL",    // Component name (for Python/library lookup)
      SST_ELI_ELEMENT_VERSION(
          0, 0, 1),   // Version of the component (not related to SST version)
      "Mercury Node including ComputeLibrary", // Description
      COMPONENT_CATEGORY_UNCATEGORIZED // Category
  )

  NodeCL(SST::ComponentId_t id, SST::Params &params);

  ~NodeCL() {
    delete proc_;
    delete mem_;
  }

  int ncores() { return ncores_; }
  int nsockets() { return nsockets_; }
  InstructionProcessor* proc() { return proc_; }

  std::string toString() override { return sprintf("HgNodeCL%d:",my_addr_); }

private:

  int ncores_;
  int nsockets_;
  InstructionProcessor* proc_;
  MemoryModel* mem_;
  OperatingSystemCLAPI* osCL_;
};

} // namespace Hg
} // namespace SST
