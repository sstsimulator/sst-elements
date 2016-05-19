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


#include "sst_config.h"
#include <sst/core/link.h>

#include <sstream>

#include "sst/elements/thornhill/memoryHeapEvent.h"

#include "memoryHeap.h"

using namespace SST;
using namespace SST::Thornhill;

MemoryHeap::MemoryHeap(ComponentId_t id, Params& params ) :
        Component( id ), m_currentVaddr( 0x100000000 )
{
    std::stringstream linkName;
    int num = 0;
    linkName << "detailed" << num;
    while ( isPortConnected( linkName.str() ) ) {
        //printf("%s() connect port %s\n",__func__,linkName.str().c_str());
       	Link* link = configureLink( linkName.str(), "0ps",
            new Event::Handler<MemoryHeap,int>(
                    this,&MemoryHeap::eventHandler, num ) ); 
        assert(link);
		m_links.push_back(link);
        linkName.str("");
        linkName.clear();
        linkName << "detailed"<< ++num;
    }
}

void MemoryHeap::eventHandler( Event* ev, int src ) {

	MemoryHeapEvent* event = static_cast<MemoryHeapEvent*>(ev);
    printf("%s %d\n",__func__,src);

    switch ( event->type ) {
      case MemoryHeapEvent::Alloc:
        event->addr = m_currentVaddr;
        printf("%s Alloc length=%lu addr=%#lx\n",__func__,event->length,event->addr);
        m_currentVaddr += event->length;
        break; 
      case MemoryHeapEvent::Free:
		assert(0);
    }

	m_links[src]->send(0,event);
}
