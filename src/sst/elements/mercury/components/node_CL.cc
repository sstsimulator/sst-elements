// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <mercury/components/node_CL.h>

namespace SST {
namespace Hg {

extern template class  HgBase<SST::Component>;
extern template SST::TimeConverter* HgBase<SST::Component>::time_converter_;

NodeCL::NodeCL(ComponentId_t id, Params &params)
    : NodeBase(id,params) {
  out_->debug(CALL_INFO, 1, 0, "loading hg.OperatingSystemCL\n");
  osCL_ = loadUserSubComponent<OperatingSystemCL>(
      "os_slot", SST::ComponentInfo::SHARE_NONE, this);
  assert(osCL_);
  os_ = dynamic_cast<OperatingSystem*>(osCL_);
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

  unsigned int nranks = params.find<unsigned int>("nranks",-1);
  os_->set_nranks(nranks);

  int ncores_ = params.find<std::int32_t>("ncores", 1);
  int nsockets_ = params.find<std::int32_t>("nsockets",1);

  out_->debug(CALL_INFO, 1, 0, "instantiating memory model\n");
  mem_ = new MemoryModel(params,this);
  out_->debug(CALL_INFO, 1, 0, "instantiating instruction processor\n");
  proc_ = new InstructionProcessor(params,mem_,this);

  out_->debug(CALL_INFO, 1, 0, "exiting constructor\n");
}

} // namespace Hg
} // namespace SST
