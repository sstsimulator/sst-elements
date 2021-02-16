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


#ifndef _H_EMBER_COMM_CREATE_EV
#define _H_EMBER_COMM_CREATE_EV

#include "emberevent.h"

namespace SST {
namespace Ember {

class EmberCommCreateEvent : public EmberMPIEvent {

public:
    EmberCommCreateEvent( MP::Interface& api, Output* output,
                         EmberEventTimeStatistic* stat,
        Communicator oldComm, std::vector<int>& ranks, Communicator* newComm )
      : EmberMPIEvent( api, output, stat ),
        m_oldComm( oldComm),
        m_ranks(ranks),
        m_newComm(newComm)
    {}
    ~EmberCommCreateEvent() {}

   std::string getName() { return "CommCreate"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );
        m_api.comm_create( m_oldComm, m_ranks.size(), &m_ranks[0],
                                                    m_newComm, functor );
    }

private:
	Communicator m_oldComm;
    std::vector<int>& m_ranks;
    Communicator* m_newComm;
};

}
}

#endif
