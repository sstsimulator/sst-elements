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


#ifndef _H_EMBER_COMM_CREATE_EV
#define _H_EMBER_COMM_CREATE_EV

#include "emberevent.h"

namespace SST {
namespace Ember {

class EmberCommCreateEvent : public EmberMPIEvent {

public:
    EmberCommCreateEvent( MP::Interface& api, Output* output, Histo* histo,
        Communicator oldComm, std::vector<int>& ranks, Communicator* newComm ) 
      : EmberMPIEvent( api, output, histo ),
        m_oldComm( oldComm), 
        m_ranks(ranks), 
        m_newComm(newComm)
    {}
    ~EmberCommCreateEvent() {}

   std::string getName() { return "CommCreate"; }

    void issue( uint64_t time, FOO* functor ) {

        m_output->verbose(CALL_INFO, 2, 0, "\n");

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
