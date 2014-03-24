// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/link.h>
//#include <sst/core/timeLord.h>

#include <sstream>

#include "loopBack.h"

using namespace SST;
using namespace SST::Firefly;

LoopBack::LoopBack(ComponentId_t id, Params& params ) :
        Component( id )
{
    int numCores = params.find_integer("numCores", 1 );
    m_handler = new Event::Handler<LoopBack>(
                this, &LoopBack::handleCoreEvent );

    assert(m_handler);

    for ( int i = 0; i < numCores; i++ ) {
        std::ostringstream tmp;
        tmp <<  i;
        Link* link = configureLink("core" + tmp.str(), "1 ns", m_handler );
        assert(link);
        m_links.push_back( link );
    }
}

LoopBack::~LoopBack()
{
    delete m_handler;

    for ( unsigned int i = 0; i < m_links.size(); i++ ) {
        m_links[i]->setFunctor(NULL); 
    }
}

void LoopBack::handleCoreEvent( Event* ev ) {
    LoopBackEvent* event = static_cast<LoopBackEvent*>(ev);
    m_links[event->dest]->send(0,ev );
}

