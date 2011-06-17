//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/*



*/
#include "sst/core/serialization/element.h"
#include "barrier.h"



void
Barrier_pattern::handle_events(int sst_event)
{

barrier_events_t event;


    event= (barrier_events_t)sst_event;

    switch (state)   {
	case BARRIER_STATE_START:
	    cp->SM_return();
	    break;
    }

    // Don't call unregisterExit()
    // Only "main" patterns should do that; i.e., patterns that use other
    // patterns like this one

}  /* end of handle_events() */
