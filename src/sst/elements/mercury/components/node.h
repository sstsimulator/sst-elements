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

#include <mercury/components/node_base.h>
#include <sst/core/timeConverter.h>
#include <sst/core/link.h>
#include <mercury/components/operating_system_fwd.h>
#include <mercury/components/nic.h>
#include <mercury/common/request_fwd.h>
#include <mercury/common/node_address.h>
#include <cstdint>
#include <memory>

namespace SST {
namespace Hg {

// Components inherit from SST::Component
class Node : public NodeBase {
public:
  /*
   *  SST Registration macros register Components with the SST Core and
   *  document their parameters, ports, etc.
   *  SST_ELI_REGISTER_COMPONENT is required, the documentation macros
   *  are only required if relevant
   */
  // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
  SST_ELI_REGISTER_COMPONENT(
      SST::Hg::Node,      // Component class
      "hg", // Component library (for Python/library lookup)
      "Node",    // Component name (for Python/library lookup)
      SST_ELI_ELEMENT_VERSION(
          0, 0, 1),   // Version of the component (not related to SST version)
      "Simple Mercury node", // Description
      COMPONENT_CATEGORY_UNCATEGORIZED // Category
  )

  Node(SST::ComponentId_t id, SST::Params &params);
};

} // namespace Hg
} // namespace SST
