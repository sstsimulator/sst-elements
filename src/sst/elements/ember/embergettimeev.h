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


#ifndef _H_EMBER_GETTIME_EV
#define _H_EMBER_GETTIME_EV

#include <sst/elements/hermes/msgapi.h>
#include "emberevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Ember {

class EmberGetTimeEvent : public EmberEvent {
public:
	EmberGetTimeEvent( Output* output, uint64_t* ptr ) :
        EmberEvent(output),
        m_timePtr(ptr)
    {}

	~EmberGetTimeEvent() {} 

    std::string getName() { return "GetTime"; }

    virtual void issue( uint64_t time, FOO* functor ) 
    {
        m_output->debug(CALL_INFO, 2, 0, "\n");
        EmberEvent::issue( time );
        *m_timePtr = time;
    }

private:
	uint64_t* m_timePtr; 

};

}
}

#endif
