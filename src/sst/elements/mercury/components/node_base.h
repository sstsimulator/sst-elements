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
#include <sst/core/link.h>
#include <mercury/components/operating_system_api_fwd.h>
#include <mercury/components/nic.h>
#include <mercury/common/request_fwd.h>
#include <mercury/common/node_address.h>
#include <cstdint>
#include <memory>

namespace SST {
namespace Hg {

class NodeBase : public SST::Hg::Component {
public:

  // Base class for node components
  SST_ELI_REGISTER_COMPONENT_BASE(SST::Hg::NodeBase)

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

  NodeBase(SST::ComponentId_t id, SST::Params &params);

  virtual ~NodeBase() {}

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

  SST::Hg::OperatingSystemAPI* os() const {
    return os_;
  }

  void handle(Request* req);

  SST::Hg::NIC* nic() { return nic_; }

  virtual std::string toString() { return sprintf("HgNode%d:",my_addr_); }

protected:

  int nranks_;
  int npernode_;
  SST::Hg::NIC* nic_;
  SST::Hg::OperatingSystemAPI* os_;
  SST::Interfaces::SimpleNetwork* link_control_;
  SST::Link* netLink_;
  std::unique_ptr<SST::Output> out_;
  NodeId my_addr_;
};

} // namespace Hg
} // namespace SST
