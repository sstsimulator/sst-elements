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


#include "sst_config.h"
#include <sst/core/link.h>

#include <sstream>

#include "sst/elements/thornhill/memoryHeapEvent.h"

#include "memoryHeap.h"

using namespace SST;
using namespace SST::Thornhill;

MemoryHeap::MemoryHeap(ComponentId_t id, Params& params ) :
        Component( id ), m_currentVaddr( 0x100000 )
{
    int nodeId = params.find<int>("nid", -1);
    assert( -1 != nodeId );

    std::stringstream linkName;

    char buffer[100];
    snprintf(buffer,100,"@t:%d:MemoryHeap::@p():@l ",nodeId);

    m_output.init(buffer,
        params.find<uint32_t>("verboseLevel",0),
        params.find<uint32_t>("verboseMask",-1),
        Output::STDOUT);

    int num = 0;
    linkName << "detailed" << num;
    while ( isPortConnected( linkName.str() ) ) {
        m_output.verbose(CALL_INFO,1,1,"connect port %s\n",
										linkName.str().c_str());
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

    switch ( event->type ) {
      case MemoryHeapEvent::Alloc:
        event->addr = m_currentVaddr;
        m_output.verbose(CALL_INFO,1,1,
			"Alloc length=%zu addr=%" PRIx64 "\n",
									event->length,event->addr);
        m_currentVaddr += event->length;
        break; 
      case MemoryHeapEvent::Free:
        m_output.verbose(CALL_INFO,1,1,"free addr=%" PRIx64 "\n", event->addr);
		assert(0);
    }

	m_links[src]->send(0,event);
}
