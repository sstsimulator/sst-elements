// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_WAIT_EV
#define _H_EMBER_WAIT_EV

#include "emberMPIEvent.h"

using namespace SST::Hermes;

namespace SST {
namespace Ember {

class EmberWaitEvent : public EmberMPIEvent {

public:
	EmberWaitEvent( MessageInterface& api, Output* output, Histo* histo,
       		MessageRequest* req, MessageResponse* resp = NULL ) :
       	EmberMPIEvent( api, output, histo ),
       	m_req( req ),
		m_respPtr( resp )
    { }

	~EmberWaitEvent() {}

    void issue( uint64_t time, FOO* functor ) {

        m_output->verbose(CALL_INFO, 1, 0, "%p %p \n",m_req, m_respPtr);

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
