// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#pragma once

#include <common/component.h>

#include <sst/core/timeConverter.h>
#include <sst/core/link.h>
#include <components/operating_system_fwd.h>
#include <common/node_address.h>
#include <cstdint>
#include <memory>

namespace SST {
namespace Hg {

// Components inherit from SST::Component
class Node : public SST::Hg::Component {
public:
  /*
   *  SST Registration macros register Components with the SST Core and
   *  document their parameters, ports, etc.
   *  SST_ELI_REGISTER_COMPONENT is required, the documentation macros
   *  are only required if relevant
   */
  // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
  SST_ELI_REGISTER_COMPONENT(
      Node,      // Component class
      "hg", // Component library (for Python/library lookup)
      "node",    // Component name (for Python/library lookup)
      SST_ELI_ELEMENT_VERSION(
          0, 0, 1),   // Version of the component (not related to SST version)
      "Mercury Node", // Description
      COMPONENT_CATEGORY_UNCATEGORIZED // Category
  )

  SST_ELI_DOCUMENT_PARAMS({"eventsToSend",
                           "How many events this component should send.", NULL},
                          {"eventSize",
                           "Payload size for each event, in bytes.", "16"})

  SST_ELI_DOCUMENT_PORTS(
      {"detailed%(num_vNics)d", "Port connected to the detailed model", {}},
      {"nic", "Port connected to the nic", {}},
      {"loop", "Port connected to the loopBack", {}},
      {"memoryHeap", "Port connected to the memory heap", {}},
  )

  SST_ELI_DOCUMENT_STATISTICS(
      {"EventSizeReceived", "Records the payload size of each event received",
       "bytes", 1})

  SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
      {"OS_SLOT", "The operating system", "hg.node"}
      )

  enum {
    NIC_SLOT,
    MEMORY_SLOT,
    OS_SLOT
  } SubcomponentSlots;

  Node(SST::ComponentId_t id, SST::Params &params);

  /**
   @return  A unique integer identifier
  */
  NodeId addr() const {
    return my_addr_;
  }

  void setup() override;

  int ncores() { return ncores_; }
  int nsockets() { return nsockets_; }

private:

  SST::Hg::OperatingSystem* os_;

  std::unique_ptr<SST::Output> out_;

  NodeId my_addr_;
  int ncores_;
  int nsockets_;

  //sw::AppLauncher* app_launcher_;
};

} // namespace Hg
} // namespace SST
