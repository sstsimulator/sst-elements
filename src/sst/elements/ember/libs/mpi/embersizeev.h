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


#ifndef _H_EMBER_SIZE_EVENT
#define _H_EMBER_SIZE_EVENT

#include "emberMPIEvent.h"

namespace SST {
namespace Ember {

class EmberSizeEvent : public EmberMPIEvent {

public:
	EmberSizeEvent( MP::Interface& api, Output* output,
                   EmberEventTimeStatistic* stat,
            Communicator comm, int* sizePtr ) :
        EmberMPIEvent( api, output, stat ),
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
