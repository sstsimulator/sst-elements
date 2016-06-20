// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/subcomponent.h>
#include <sst/core/link.h>

#include "sst/elements/miranda/mirandaEvent.h"

#include "singleThread.h"

using namespace SST;
using namespace SST::Miranda;
using namespace SST::Thornhill;

SingleThread::SingleThread( Component* owner, 
        Params& params )
        : DetailedCompute( owner ), m_link(NULL)
{
    std::stringstream linkName; 

    std::string portName = params.find<std::string>( "portName", "detailed" );
    
    int id = 0;
    linkName << portName << id;
    while ( owner->isPortConnected( linkName.str() ) ) {
        if ( m_link ) {
            Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO,1,
              "Thornhill::SingleTread() more than 1 link detected `%s`\n",
              portName.c_str());
        }
        m_link = configureLink( linkName.str(), "0ps", 
            new Event::Handler<SingleThread>(
                    this,&SingleThread::eventHandler ) ); 
        assert(m_link);
		linkName.str("");
		linkName.clear();
        linkName << portName << ++id;
    }
}

void SingleThread::eventHandler( SST::Event* ev )
{
    MirandaRspEvent* event = static_cast<MirandaRspEvent*>(ev);

	Entry* entry = static_cast<Entry*>((void*)event->key);
	entry->finiHandler();
	delete entry;
}

void SingleThread::start( const std::deque< std::pair< std::string, SST::Params> >& generators,
                 std::function<int()> finiHandler )
{
    MirandaReqEvent* event = new MirandaReqEvent;
	
	event->key = (uint64_t) new Entry( finiHandler );

	event->generators = generators;

	m_link->send( 0, event );
}
