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


#ifndef _H_EMBER_SIZE_EVENT
#define _H_EMBER_SIZE_EVENT

#include "emberMPIEvent.h"

namespace SST {
namespace Ember {

class EmberSizeEvent : public EmberMPIEvent {

public:
	EmberSizeEvent( MP::Interface& api, Output* output, Histo* histo,
            Communicator comm, int* sizePtr ) :
        EmberMPIEvent( api, output, histo ),
        m_comm( comm ),
        m_sizePtr( sizePtr )
    {}

	~EmberSizeEvent() {}

    std::string getName() { return "Size"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );

        m_api.size( m_comm, m_sizePtr, functor );
    }

private:
    Communicator m_comm;
    int*         m_sizePtr;
    
};

}
}

#endif
