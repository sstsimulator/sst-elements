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


#ifndef _H_EMBER_COMM_DESTROY_EV
#define _H_EMBER_COMM_DESTROY_EV

#include "emberevent.h"

namespace SST {
namespace Ember {

class EmberCommDestroyEvent : public EmberMPIEvent {

public:
    EmberCommDestroyEvent( MP::Interface& api, Output* output,
                          EmberEventTimeStatistic* stat,
        Communicator comm )
      : EmberMPIEvent( api, output, stat ),
        m_comm( comm)
    {}
    ~EmberCommDestroyEvent() {}

   std::string getName() { return "CommDestroy"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );
        m_api.comm_destroy( m_comm, functor );
    }

private:
	Communicator m_comm;
};

}
}

#endif
