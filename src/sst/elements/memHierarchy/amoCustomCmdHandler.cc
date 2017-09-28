// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "amoCustomCmdHandler.h"

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;

CustomCmdMemHandler::MemEventInfo AMOCustomCmdMemHandler::receive(MemEventBase* ev){
  CustomCmdMemHandler::MemEventInfo MEI(ev->getRoutingAddress(),false);
  return MEI;
}

CustomCmdInfo* AMOCustomCmdMemHandler::ready(MemEventBase* ev){
  CustomCmdInfo *CI = new CustomCmdInfo(ev->getID(),
                                        ev->getRqstr(),
                                        MemEvent::F_SUCCESS);
  return CI;
}

MemEventBase* AMOCustomCmdMemHandler::finish(MemEventBase *ev, uint64_t flags){
  MemEventBase *MEB = new MemEventBase(ev->getSrc(),
                                        ev->getCmd());
  return MEB;
}

// EOF
