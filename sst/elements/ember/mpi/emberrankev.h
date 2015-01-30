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


#ifndef _H_EMBER_RANK_EVENT
#define _H_EMBER_RANK_EVENT

#include "emberMPIEvent.h"

namespace SST {
namespace Ember {

class EmberRankEvent : public EmberMPIEvent {

public:
	EmberRankEvent( MP::Interface& api, Output* output, Histo* histo,
            Communicator comm, uint32_t* rankPtr ) :
        EmberMPIEvent( api, output, histo ),
        m_comm( comm ),
        m_rankPtr( rankPtr)
    {}

	~EmberRankEvent() {}

    std::string getName() { return "Rank"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );

        m_api.rank( m_comm, m_rankPtr, functor );
    }

private:
    Communicator m_comm;
    uint32_t*    m_rankPtr;
    
};

}
}

#endif
