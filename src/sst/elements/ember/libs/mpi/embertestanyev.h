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


#ifndef _H_EMBER_TESTANY_EV
#define _H_EMBER_TESTANY_EV

#include "emberMPIEvent.h"

namespace SST {
namespace Ember {

class EmberTestanyEvent : public EmberMPIEvent {

public:
	EmberTestanyEvent( MP::Interface& api, Output* output,
                   EmberEventTimeStatistic* stat,
       		int count, MessageRequest* req, int* indx, int* flag,
			MessageResponse* resp = NULL ) :

       	EmberMPIEvent( api, output, stat ),
		m_cnt(count),
       	m_req( req ),
		m_indx(indx),
		m_flag( flag ),
		m_respPtr( resp )
    { }

	~EmberTestanyEvent() {}

    std::string getName() { return "Testany"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );

       	m_api.testany( m_cnt, m_req, m_indx, m_flag, m_respPtr, functor );
    }

private:
    MessageRequest* 	m_req;
	MessageResponse*	m_respPtr;
	int 				m_cnt;
	int*				m_indx;
	int*				m_flag;
};

}
}

#endif
