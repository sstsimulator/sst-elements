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

#include <iostream>

namespace SST {
namespace Hg {

extern template class  HgBase<SST::Component>;
extern template SST::TimeConverter HgBase<SST::Component>::time_converter_;

NodeBase::NodeBase(ComponentId_t id, Params &params)
    : SST::Hg::Component(id), nic_(0) {
  my_addr_ = params.find<unsigned int>("logicalID",-1);
  unsigned int verbose = params.find<unsigned int>("verbose",0);
  out_ = std::unique_ptr<SST::Output>(new SST::Output(sprintf("Node%d:",my_addr_), verbose, 0, Output::STDOUT));

  nranks_ = params.find<int>("nranks", 1);
  npernode_ = params.find<int>("npernode", 1);

  // Tell the simulation not to end until we're ready
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
}

void
NodeBase::init(unsigned int phase)
{
  os_->setNumRanks(nranks_);
  os_->setRanksPerNode(npernode_);
  os_->init(phase);
  if (nic_) nic_->init(phase);
}

void
NodeBase::setup()
{
  SST::Component::setup();
  os_->setup();
  if (nic_) nic_->setup();
}

void
NodeBase::handle(Request* req)
{
  os_->handleRequest(req);
}

} // namespace Hg
} // namespace SST
