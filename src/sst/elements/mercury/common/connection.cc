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

#include <mercury/common/component.h>
#include <sst/core/params.h>
#include <mercury/common/connection.h>
#include <mercury/common/event_link.h>

namespace SST {
namespace Hg {

ConnectableComponent::ConnectableComponent(uint32_t cid, SST::Params& params)
  : Component(cid)
{

}

void
ConnectableSubcomponent::initInputLink(int src_outport, int dst_inport)
{
  std::string name = SST::Hg::sprintf("input%d", dst_inport);
  SST::Link* link = configureLink(name, HgBase::timeConverter(), payloadHandler(dst_inport));
  if (!link){
    sst_hg_abort_printf("cannot find input link for port %s on connectable component %s",
                      name.c_str(), getName().c_str());
  }
  EventLink *idk = new EventLink(name, TimeDelta(), link);
  EventLink::ptr ev_link{new EventLink(name, TimeDelta(), link)};
  connectInput(src_outport, dst_inport, std::move(ev_link));
}

void
ConnectableSubcomponent::initOutputLink(int src_outport, int dst_inport)
{
  std::string name = SST::Hg::sprintf("output%d", src_outport);
  SST::Link* link = configureLink(name, HgBase::timeConverter(), creditHandler(src_outport));
  if (!link){
    sst_hg_abort_printf("cannot find output link for port %s on connectable component named %s",
                      name.c_str(), getName().c_str());
  }
  EventLink::ptr ev_link{new EventLink(name, TimeDelta(), link)};
  connectOutput(src_outport, dst_inport, std::move(ev_link));
}


void
ConnectableComponent::initInputLink(int src_outport, int dst_inport)
{
  std::string name = SST::Hg::sprintf("input%d", dst_inport);
  SST::Link* link = configureLink(name, HgBase::timeConverter(), payloadHandler(dst_inport));
  if (!link){
    sst_hg_abort_printf("cannot find input link for port %s on connectable component named %s",
                      name.c_str(), getName().c_str());
  }
  EventLink::ptr ev_link{new EventLink(name, TimeDelta(), link)};
  connectInput(src_outport, dst_inport, std::move(ev_link));
}

void
ConnectableComponent::initOutputLink(int src_outport, int dst_inport)
{
  std::string name = SST::Hg::sprintf("output%d", src_outport);
  SST::Link* link = configureLink(name, HgBase::timeConverter(), creditHandler(src_outport));
  if (!link){
    sst_hg_abort_printf("cannot find output link for port %s on connectable component %s",
                      name.c_str(), getName().c_str());
  }
  EventLink::ptr ev_link{new EventLink(name, TimeDelta(), link)};
  connectOutput(src_outport, dst_inport, std::move(ev_link));
}


}
}
