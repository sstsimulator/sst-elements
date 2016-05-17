// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
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
        Params& params, std::string name )
        : DetailedCompute( owner )
{
    std::stringstream linkName; 
    
    int id = 0;
    linkName << "detailed" << id++;
    while ( owner->isPortConnected( linkName.str() ) ) {
		printf("%s() connect port %s\n",__func__,linkName.str().c_str());
        Link* link = configureLink( linkName.str(), "0ps", 
            new Event::Handler<SingleThread>(
                    this,&SingleThread::eventHandler ) ); 
        assert(link);
        m_links.push_back( link );
		linkName.str("");
		linkName.clear();
        linkName << "detailed"<< id++;
    }

	// Can have 0 connected links if detailed model is not used 
	// for this endpoing
	
	assert( 2 > m_links.size() );
}

void SingleThread::eventHandler( SST::Event* ev )
{
    MirandaRspEvent* event = static_cast<MirandaRspEvent*>(ev);

	Entry* entry = static_cast<Entry*>((void*)event->key);
	entry->finiHandler();
	delete entry;
}

void SingleThread::start( std::string& name, Params& params,
                 std::function<int()> finiHandler )
{
    MirandaReqEvent* event = new MirandaReqEvent;
	
	event->key = (uint64_t) new Entry( finiHandler );

	event->generator = name;
	event->params = params;

	m_links[0]->send( 0, event );
}
