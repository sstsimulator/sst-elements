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


#ifndef _H_EMBER_WAIT_EV
#define _H_EMBER_WAIT_EV

#include "emberMPIEvent.h"

namespace SST {
namespace Ember {

class EmberWaitEvent : public EmberMPIEvent {

public:
	EmberWaitEvent( MP::Interface& api, Output* output,
                   EmberEventTimeStatistic* stat,
       		MessageRequest* req, MessageResponse* resp = NULL ) :
       	EmberMPIEvent( api, output, stat ),
       	m_req( req ),
		m_respPtr( resp )
    { }

	~EmberWaitEvent() {}

    std::string getName() { return "Wait"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );

       	m_api.wait( *m_req, m_respPtr, functor );
    }

private:
    MessageRequest* 	m_req;
	MessageResponse*	m_respPtr;	
};

}
}

#endif
