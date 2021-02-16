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


#ifndef _H_EMBER_LIBS_MISC_MALLOC_EV
#define _H_EMBER_LIBS_MISC_MALLOC_EV

#include <sst/elements/hermes/miscapi.h>
#include "emberMiscEvent.h"

using namespace SST::Hermes;

namespace SST {
namespace Ember {

class EmberMallocEvent : public EmberMiscEvent {
public:
	EmberMallocEvent( Misc::Interface& api, Output* output, Hermes::MemAddr* addr,
		   size_t length, bool backed, EmberEventTimeStatistic* stat = NULL ) :
        EmberMiscEvent( api, output, stat ), m_addr(addr), m_length(length), m_backed(backed)
    { }

	~EmberMallocEvent() {}

    std::string getName() { return "Malloc"; }

    virtual void issue( uint64_t time, Callback* callback )
    {
        EmberEvent::issue( time );
		m_output->verbose(CALL_INFO, 2, EVENT_MASK, "length=%zu backed=%d\n",m_length, m_backed );
        m_api.malloc( m_addr, m_length, m_backed, callback );
    }

private:
	Hermes::MemAddr* m_addr;
	size_t m_length;
	bool m_backed;
};

}
}

#endif
