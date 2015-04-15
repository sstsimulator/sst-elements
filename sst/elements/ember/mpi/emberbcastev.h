// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
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
    EmberBcastEvent( MP::Interface& api, Output* output, Histo* histo,
            Addr mydata, uint32_t count, PayloadDataType dtype,
            int root, Communicator group ) :
        EmberMPIEvent( api, output, histo ),
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
    Addr                m_mydata;
    uint32_t            m_count;
    PayloadDataType     m_dtype;
    int                 m_root;
    Communicator        m_group;
};

}
}

#endif
