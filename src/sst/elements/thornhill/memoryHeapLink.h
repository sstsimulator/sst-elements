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

#ifndef _H_THORNHILL_MEMORY_HEAP_LINK
#define _H_THORNHILL_MEMORY_HEAP_LINK


#include <functional>
#include "sst/core/subcomponent.h"
#include "sst/core/link.h"
#include "sst/elements/thornhill/memoryHeapEvent.h"

namespace SST {
namespace Thornhill {

class MemoryHeapLink : public SubComponent {

  public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Thornhill::MemoryHeapLink)
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        MemoryHeapLink,
        "thornhill",
        "MemoryHeapLink",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
		SST::Thornhill::MemoryHeapLink
    )

	struct Entry {
		Entry( std::function<void(uint64_t)> _fini ) : fini( _fini ) {}
		std::function<void(uint64_t)> fini;
	};

  public:
    MemoryHeapLink( ComponentId_t id, Params& params ) : SubComponent(id)
	{
		m_link = configureLink( "memoryHeap", "0ps",
            new Event::Handler<MemoryHeapLink>(
                    this,&MemoryHeapLink::eventHandler ) );
        assert(m_link);
	}

    bool isConnected() {
        return (m_link);
    }

	void alloc( size_t length, std::function<void(uint64_t)> fini ) {
		Entry* entry = new Entry( fini );

		MemoryHeapEvent* event = new MemoryHeapEvent;
		event->key = (MemoryHeapEvent::Key) entry;
		event->type = MemoryHeapEvent::Alloc;
		event->length = length;

		m_link->send(0, event );
	}

	void free( SimVAddr addr, std::function<void(uint64_t)> fini ) {
		Entry* entry = new Entry( fini );

		MemoryHeapEvent* event = new MemoryHeapEvent;
		event->key = (MemoryHeapEvent::Key) entry;
		event->type = MemoryHeapEvent::Free;
		event->addr = addr;

		m_link->send(0, event );
	}

    ~MemoryHeapLink(){};

  private:
	void eventHandler( SST::Event* ev ) {
		MemoryHeapEvent* event = static_cast<MemoryHeapEvent*>(ev);
		Entry* entry = (Entry*) event->key;
		entry->fini(event->addr);
		delete entry;
		delete ev;
	}
    Link*  m_link;
};


}
}
#endif
