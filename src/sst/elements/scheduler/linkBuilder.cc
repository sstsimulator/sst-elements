// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdio.h>

#include "sst_config.h"
#include "sst/core/element.h"

#include "linkBuilder.h"

#include "events/CommunicationEvent.h"
#include "nodeComponent.h"
#include "events/ObjectRetrievalEvent.h"

using namespace SST::Scheduler;


linkBuilder::linkBuilder(SST::ComponentId_t id, SST::Params & params) : Component( id )
{

}


void linkBuilder::connectGraph(Job* job)
{

}


void linkBuilder::disconnectGraph(Job* job)
{

}


void linkBuilder::initNodePtrRequests(SST::Event* event)
{

}


void linkBuilder::handleNewNodePtr(SST::Event* event)
{

}

