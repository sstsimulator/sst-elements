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

#include <components/node.h>
#include <components/operating_system.h>
#include <operating_system/launch/app_launch_request.h>
#include <operating_system/process/app.h>

namespace SST {
namespace Hg {

extern template class  HgBase<SST::Component>;
extern template SST::TimeConverter* HgBase<SST::Component>::time_converter_;

Node::Node(ComponentId_t id, Params &params)
    : SST::Hg::Component(id) {

  my_addr_ = getId();
  unsigned int verbose = params.find<unsigned int>("verbose",0);
  out_ = std::unique_ptr<SST::Output>(new SST::Output(sprintf("Node%d:",my_addr_), verbose, 0, Output::STDOUT));

  out_->debug(CALL_INFO, 1, 0, "loading hg.operatingsystem\n");
  os_ =  loadAnonymousSubComponent<OperatingSystem>("hg.operating_system", "os_slot", OS_SLOT,
                                                     SST::ComponentInfo::SHARE_STATS, params, this);

  int ncores_ = params.find<std::int32_t>("ncores", 1);
  int nsockets_ = params.find<std::int32_t>("nsockets",1);

//  // Tell the simulation not to end until we're ready
//  registerAsPrimaryComponent();
//  primaryComponentDoNotEndSim();
}

void
Node::setup()
{
  SST::Component::setup();
  os_->setup();
}

} // namespace Hg
} // namespace SST
