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


#ifndef _H_EMBER_WAITALL_EV
#define _H_EMBER_WAITALL_EV

#include "emberMPIEvent.h"

namespace SST {
namespace Ember {

class EmberWaitallEvent : public EmberMPIEvent {

public:
	EmberWaitallEvent( MP::Interface& api, Output* output,
                      EmberEventTimeStatistic* stat,
           int count, MessageRequest* m_req, MessageResponse** m_resp  ) :
        EmberMPIEvent( api, output, stat ),
        m_count(count),
        m_req(m_req),
        m_resp(m_resp)
    { }

	~EmberWaitallEvent() {}

    std::string getName() { return "Waitall"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );

        m_api.waitall( m_count, m_req, m_resp, functor );
    }

private:
    int m_count;
    MessageRequest* m_req;
    MessageResponse** m_resp;
};

}
}

#endif
