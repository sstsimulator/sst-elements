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


#ifndef _H_EMBER_MEMORY_EVENT
#define _H_EMBER_MEMORY_EVENT

#include "emberevent.h"

namespace SST {
namespace Ember {

class EmberMemAllocEvent : public EmberEvent {

public:
	EmberMemAllocEvent( Thornhill::MemoryHeapLink& api, 
			Output* output, Hermes::MemAddr* addr, size_t length) : 
			EmberEvent(output), m_api(api), m_addr(addr), m_length(length)
    {
		m_state = IssueFunctor;
	}  

	~EmberMemAllocEvent() {}

    std::string getName() { return "MemAlloc"; }

    void issue( uint64_t time, FOO* functor ) {

        m_output->debug(CALL_INFO, 2, 0, "length=%zu\n", m_length );
        EmberEvent::issue( time );
    
        std::function<void(uint64_t)> callback = [=](uint64_t value){
			m_addr->setSimVAddr( value );
            (*functor)(0);
            return 0;
        };

		m_api.alloc( m_length, callback );
    }

protected:
    Thornhill::MemoryHeapLink&  m_api; 
    Hermes::MemAddr* 	m_addr;
	size_t  			m_length;

};

}
}

#endif
