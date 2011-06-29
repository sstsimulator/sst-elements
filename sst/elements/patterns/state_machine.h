//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _STATE_MACHINE_H
#define _STATE_MACHINE_H

#include <list>
#include <vector>
#include <stdint.h>	// For uint32_t

const int START_START_EVENT= 0;


class State_machine   {
    public:
	State_machine(const int rank) :
	    // my_rank is only needed for debug output
	    my_rank(rank)
	{
	}

        ~State_machine() {}


	typedef struct state_event_t   {
	    int event;
	    double data;
	} state_event_t;

	uint32_t SM_create(void *obj, void (*handler)(void *obj, state_event_t event));
	void SM_call(int machineID, int return_event);
	void SM_return(void);
	uint32_t SM_current_tag(void);

	// Comm_pattern needs to call handle_state_events()
	friend class Comm_pattern;

    private:

	void handle_state_events(uint32_t tag, int event);
	void deliver_missed_events(void);

	typedef struct SM_t   {
	    void (*handler)(void *obj, state_event_t event);
	    void *obj;
	    uint32_t tag;
	    std::list <state_event_t>missed_events;
	} SM_t;
	std::vector <SM_t>SM;
	std::vector <int>SMstack;

	int currentSM;
	int lastSM;

	// For debugging
	const int my_rank;

};

#endif // _STATE_MACHINE_H
