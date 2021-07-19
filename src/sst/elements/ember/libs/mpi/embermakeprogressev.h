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


#ifndef _H_EMBER_MAKEPROGRESS_EVENT
#define _H_EMBER_MAKEPROGRESS_EVENT

#include "emberMPIEvent.h"

namespace SST {
namespace Ember {

class EmberMakeProgressEvent : public EmberMPIEvent {

public:
	EmberMakeProgressEvent( MP::Interface& api, Output* output,
                    EmberEventTimeStatistic* stat ) :
            EmberMPIEvent( api, output, stat ){}
	~EmberMakeProgressEvent() {}

    std::string getName() { return "Probe"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );
        m_api.makeProgress( functor );
    }
};

}
}

#endif
