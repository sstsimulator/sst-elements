//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _BARRIER_PATTERN_H
#define _BARRIER_PATTERN_H

#include "comm_pattern.h"



// The Barrier pattern generator can be in these states and deals
// with these events.
typedef enum {BARRIER_STATE_START} barrier_state_t;
typedef enum {BARRIER_DONE} barrier_events_t;



class Barrier_pattern   {
    public:
	Barrier_pattern(Comm_pattern * const& current_pattern)   {
	    cp= current_pattern;
	    state= BARRIER_STATE_START;
	}

        ~Barrier_pattern() {}

	uint32_t install_handler(void)
	{
	    uint32_t SMbarrier;

	    SMbarrier= cp->SM_create((void *)this, Barrier_pattern::wrapper_handle_events);
	    return SMbarrier;
	}


    private:
	// Wrapping a pointer to a non-static member function like this is from
	// http://www.newty.de/fpt/callback.html
	void handle_events(int sst_event);
	static void wrapper_handle_events(void *obj, int sst_event)
	{
	    Barrier_pattern* mySelf = (Barrier_pattern*) obj;
	    mySelf->handle_events(sst_event);
	}

	// We need to remember how to upcall into our parent object
	Comm_pattern *cp;

	barrier_state_t state;

};

#endif // _BARRIER_PATTERN_H
