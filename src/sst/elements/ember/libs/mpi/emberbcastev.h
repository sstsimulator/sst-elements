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


#ifndef _H_EMBER_BCAST_EV
#define _H_EMBER_BCAST_EV

#include "emberMPIEvent.h"

namespace SST {
namespace Ember {

class EmberBcastEvent : public EmberMPIEvent {
public:
    EmberBcastEvent( MP::Interface& api, Output* output,
                    EmberEventTimeStatistic* stat,
            const Hermes::MemAddr& mydata,
            uint32_t count, PayloadDataType dtype,
            int root, Communicator group ) :
        EmberMPIEvent( api, output, stat ),
        m_mydata(mydata),
        m_count(count),
        m_dtype(dtype),
        m_root( root ),
        m_group(group)
    {}

	~EmberBcastEvent() {}

    std::string getName() { return "Bcast"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );

        m_api.bcast( m_mydata, m_count, m_dtype, m_root, m_group, functor );
    }

private:
    Hermes::MemAddr     m_mydata;
    uint32_t            m_count;
    PayloadDataType     m_dtype;
    int                 m_root;
    Communicator        m_group;
};

}
}

#endif
