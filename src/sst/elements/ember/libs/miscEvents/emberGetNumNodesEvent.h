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


#ifndef _H_EMBER_LIBS_MISC_GETNUMNODES_EV
#define _H_EMBER_LIBS_MISC_GETNUMNODES_EV

#include <sst/elements/hermes/miscapi.h>
#include "emberMiscEvent.h"

using namespace SST::Hermes;

namespace SST {
namespace Ember {

class EmberGetNumNodesEvent : public EmberMiscEvent {
public:
	EmberGetNumNodesEvent( Misc::Interface& api, Output* output, int* ptr, EmberEventTimeStatistic* stat = NULL ) :
        EmberMiscEvent( api, output, stat ),
        m_ptr(ptr)
    { }

	~EmberGetNumNodesEvent() {}

    std::string getName() { return "GetNumNodes"; }

    virtual void issue( uint64_t time, Callback* callback )
    {
        EmberEvent::issue( time );
        //m_output->verbose(CALL_INFO, 2, 0, "\n");
        m_api.getNumNodes( m_ptr, callback );
    }

private:
	int* m_ptr;
};

}
}

#endif
