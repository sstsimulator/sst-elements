//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _STATE_MACHINE_H
#define _STATE_MACHINE_H

#include <stdlib.h>	// For exit()
#include <list>
#include <vector>
#include <stdint.h>	// For uint32_t
#include <assert.h>

const int SM_START_EVENT= 0;
const int SM_MAX_DATA_FIELDS= 2;

#define _sm_abort( name, fmt, args...)\
{\
    fprintf(stderr, "%s::%s():%d:ABORT: "fmt, #name, __FUNCTION__, __LINE__, ## args ); \
    exit(-1); \
}



// This object is the event that gets passed among state machines
// It contains the event ID (number) and some data for those
// state machines that need to pass parameters to each other
// FIXME: It would be nice if SM_MAX_DATA_FIELDS wasn't hard-coded
class state_event   {
    public:
	state_event(void)
	{
	    // Initialize things
	    payload_size= sizeof(packed_data);
	    payload= &packed_data;
	    for (int i= 0; i < SM_MAX_DATA_FIELDS; i++)   {
		packed_data.Fdata[i]= 0.0;
		packed_data.Idata[i]= 0;
	    }
	    restart= false;
	    event= -1;
	}

	void set_Fdata(double F1, double F2= 0.0)   {
	    packed_data.Fdata[0]= F1;
	    packed_data.Fdata[1]= F2;
	}

	double get_Fdata(int pos= 0)   {
	    assert(pos < SM_MAX_DATA_FIELDS);
	    return packed_data.Fdata[pos];
	}

	void set_Idata(double I1, double I2= 0.0)   {
	    packed_data.Idata[0]= I1;
	    packed_data.Idata[1]= I2;
	}

	long long get_Idata(int pos= 0)   {
	    assert(pos < SM_MAX_DATA_FIELDS);
	    return packed_data.Idata[pos];
	}

	int payload_size;
	void *payload;
	int event;

	// For runtime debugging. Before returning from a SM call, we make sure the
	// event we are returning has this flag set.
	bool restart;

    private:
	// Some state machine specific data that travels in the event
	// Most state machines wont need this. Others may use any way they wish
	struct packed_data   {
	    double Fdata[SM_MAX_DATA_FIELDS];
	    long long Idata[SM_MAX_DATA_FIELDS];
	} packed_data;
};



#define state_transition(event, new_state) {\
	    state= new_state;\
	    self_event_send(event);\
	}



class State_machine   {
    public:
	State_machine(const int rank) :
	    // my_rank is only needed for debug output
	    my_rank(rank)
	{
	}

        ~State_machine() {}


	// Each SM has some (or none) SM-specific data that gets sent with each
	// event. That data is stored here and can be updated with the state_event
	// object methods.
	state_event SM_data;


	uint32_t SM_create(void *obj, void (*handler)(void *obj, state_event event));
	void SM_call(int machineID, state_event start_event, state_event return_event);
	void SM_return(state_event return_event);
	uint32_t SM_current_tag(void);

	// Comm_pattern needs to call handle_state_events()
	friend class Comm_pattern;

    private:

	void handle_state_events(uint32_t tag, state_event event);
	void deliver_missed_events(void);

	typedef struct SM_t   {
	    void (*handler)(void *obj, state_event event);
	    void *obj;
	    uint32_t tag;
	    std::list <state_event>missed_events;
	} SM_t;
	std::vector <SM_t>SM;
	std::vector <int>SMstack;

	int currentSM;
	int lastSM;

	// For debugging
	const int my_rank;

};

#endif // _STATE_MACHINE_H
