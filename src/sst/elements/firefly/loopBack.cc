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


#include "sst_config.h"
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/link.h>

#include <sstream>

#include "loopBack.h"

using namespace SST;
using namespace SST::Firefly;

LoopBack::LoopBack(ComponentId_t id, Params& params ) :
        Component( id )
{
    int nicsPerNode = params.find<int>("nicsPerNode", 1 );
    int numCores = params.find<int>("numCores", 1 )/nicsPerNode;

	for ( int j = 0; j < nicsPerNode; j++ ) {
		std::ostringstream nic;
		nic <<  j;
		for ( int i = 0; i < numCores; i++ ) {
			std::ostringstream core;
			core <<  i;

			Event::Handler<LoopBack,int>* handler =
                new Event::Handler<LoopBack,int>( this, &LoopBack::handleCoreEvent, j*numCores + i );

			Link* link = configureLink("nic"+ nic.str() + "core" + core.str(), "1 ns", handler );
			assert(link);
			m_links.push_back( link );
		}
	}
}

void LoopBack::handleCoreEvent( Event* ev, int src ) {
    LoopBackEventBase* event = static_cast<LoopBackEventBase*>(ev);
    int dest = event->core;
    event->core = src;
    m_links[dest]->send(0,ev );
}
