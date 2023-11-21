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

#include <mercury/common/component.h>

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

  SST_ELI_DOCUMENT_PARAMS({"verbose",
                           "Output verbose level", 0},
                          )

  SST_ELI_DOCUMENT_PORTS(
      {"network", "Dummy network port to connect nodes for testing", {} },
  )

  SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
      {"os_slot", "The operating system", "hg.operating_system"},
      {"nic_slot", "The nic", "hg.nic"},
      {"link_control_slot", "Slot for a link control", "SST::Interfaces::SimpleNetwork" }
      )

  Node(SST::ComponentId_t id, SST::Params &params);

  /**
   @return  A unique integer identifier
  */
  NodeId addr() const {
    return my_addr_;
  }

  void init(unsigned int phase) override;

  void setup() override;

  void endSim() {
    primaryComponentOKToEndSim();
  }

  SST::Hg::OperatingSystem* os() const {
    return os_;
  }

  int ncores() { return ncores_; }
  int nsockets() { return nsockets_; }

  void handle(Request* req);

  SST::Hg::NIC* nic() { return nic_; }

private:

  SST::Hg::NIC* nic_;
  SST::Hg::OperatingSystem* os_;
  SST::Interfaces::SimpleNetwork* link_control_;
  SST::Link* netLink_;
  std::unique_ptr<SST::Output> out_;
  NodeId my_addr_;
  int ncores_;
  int nsockets_;
};

} // namespace Hg
} // namespace SST
