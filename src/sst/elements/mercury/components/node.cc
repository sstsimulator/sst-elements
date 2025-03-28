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

#include <mercury/components/node.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/launch/app_launch_request.h>
#include <mercury/operating_system/process/app.h>

namespace SST {
namespace Hg {

extern template class  HgBase<SST::Component>;
extern template SST::TimeConverter* HgBase<SST::Component>::time_converter_;

Node::Node(ComponentId_t id, Params &params)
    : SST::Hg::NodeBase(id,params) {
  out_->debug(CALL_INFO, 1, 0, "loading hg.operating_system\n");
  os_ = loadUserSubComponent<OperatingSystem>(
      "os_slot", SST::ComponentInfo::SHARE_NONE, this);
  assert(os_);

  link_control_ = loadUserSubComponent<SST::Interfaces::SimpleNetwork>(
      "link_control_slot", SST::ComponentInfo::SHARE_NONE, 1);
  if (link_control_) {
    out_->debug(CALL_INFO, 1, 0, "loading hg.NIC\n");
    nic_ = loadUserSubComponent<NIC>("nic_slot", SST::ComponentInfo::SHARE_NONE, this);
    assert(nic_);
    nic_->set_link_control(link_control_);
  }
  else {
    // assume basic tests
    // (unused but needs to be there or multithread termination breaks)
    netLink_ = configureLink("network");
  }

  unsigned int nranks = params.find<unsigned int>("nranks", -1);
  os_->set_nranks(nranks);
  unsigned int npernode = params.find<unsigned int>("npernode", 1);
  os_->set_npernode(npernode);
}

} // namespace Hg
} // namespace SST
