/*
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
*/

/*
** This file contains common routines used by all pattern generators.
*/
#include <sst_config.h>
#include <stdio.h>
#include <inttypes.h>		// For PRId64
#include <string>
#include "pattern_common.h"



void
Patterns::init(SST::Params& params, SST::Link *self_link,
	NIC_model *model[NUM_NIC_MODELS],
	SST::Link *nvram_link, SST::Link *storage_link, MachineInfo *m,
	int my_rank)
{

    _my_rank= my_rank;
    _m= m;
    msg_seq= 1;

    for (int i= Net; i <= Far; i++)   {
	nic[i]= model[i];
    }
    
    my_self_link= self_link;
    my_nvram_link= nvram_link;
    my_storage_link= storage_link;

}  /* end of init() */



// Save the lower 27 bits of the event ID for the rank number
#define RANK_FIELD		(27)



void
Patterns::self_event_send(int event, int32_t tag, SST::SimTime_t delay)
{

CPUNicEvent *e;


    // Create an event and fill in the event info
    e= new CPUNicEvent();
    e->SetRoutine(event);
    e->router_delay= 0;
    e->hops= 0;
    e->msg_len= 0;
    assert(tag >= 0);
    e->tag= tag;
    e->dest= _my_rank;
    e->msg_id= (msg_seq++ << RANK_FIELD) | _my_rank;

    my_self_link->send(delay, e); 

}  // end of self_event_send()



//
// This function sends an event.
// No actual data is sent except for what is in payload.
// The data is out of band and used internally.
//
void
Patterns::event_send(int dest_rank, int event, int32_t tag, uint32_t msg_len,
	const char *payload, int payload_len, int blocking)
{

CPUNicEvent *e;
SST::SimTime_t delay;


    // Create an event and fill in the event info
    e= new CPUNicEvent();
    e->SetRoutine(event);
    e->router_delay= 0;
    e->hops= 0;
    e->msg_len= msg_len;
    assert(tag >= 0);
    e->tag= tag;
    e->dest= dest_rank;
    e->msg_id= (msg_seq++ << RANK_FIELD) | _my_rank;

    // If there is a payload, attach it
    if (payload)   {
	e->AttachPayload(payload, payload_len);
    }

    if (dest_rank == _my_rank)   {
	// No need to go through the network for this
	// FIXME: Shouldn't this involve some sort of delay?
	fprintf(stderr, "Event send to my self!\n"); //  Does this get used?
	my_self_link->send(e); 
	if (blocking >= 0)   {
	    self_event_send(blocking, tag, 0);
	}
	return;
    }

    /* Is dest within our NoC? */
    if (_m->myNode() == _m->destNode(dest_rank))   {
	/* Route locally */
	delay= nic[NoC]->send(e, dest_rank);

    } else   {
	/* Route off chip */

	if (_m->FarLinkExists(_m->destNode(dest_rank)))   {
	    // We have a far link to that destination node. Use it
	    delay= nic[Far]->send(e, dest_rank);
	} else   {
	    // Send it through the network
	    delay= nic[Net]->send(e, dest_rank);
	}
    }
    if (blocking >= 0)   {
	// Send a send completion event to ourselves
	self_event_send(blocking, tag, delay);
    }

}  /* end of event_send() */



/*
** Send a chunk of data to our stable storage device.
*/
void
Patterns::storage_write(int data_size, int return_event)
{

CPUNicEvent *e;
int my_core;
uint64_t delay;


    // Create an event and fill in the event info
    e= new CPUNicEvent();
    assert(0);  // FIXME: We need to SetRoutine()
    // e->SetRoutine(BIT_BUCKET_WRITE_START);
    e->router_delay= 0;
    e->hops= 0;
    e->msg_len= data_size;
    e->dest= -1;
    e->msg_id= (msg_seq++ << RANK_FIELD) | _my_rank;
    e->return_event= return_event;

    // Storage request go out on port 0 of the aggregator
    e->route.push_back(0);

    // And then again on port 0 at the node aggregator
    e->route.push_back(0);

    // Coming back: First hop is to the right node
    e->reverse_route.push_back(_m->myNode() + 1);	// Port 0 is for the SSD

    // From the aggregator, pick the right core
    my_core= _my_rank % _m->get_cores_per_node();
    e->reverse_route.push_back(my_core + 1);	// Port 0 goes off node

    // Send the write request
    delay= 0; // FIXME; Need to figure this out
    my_storage_link->send(delay, e); 

}  /* end of storage_write() */



/*
** Send a chunk of data to our local NVRAM
*/
void
Patterns::nvram_write(int data_size, int return_event)
{

CPUNicEvent *e;
int my_core;
uint64_t delay;


    // Create an event and fill in the event info
    e= new CPUNicEvent();
    assert(0);  // FIXME: We need to SetRoutine()
    // e->SetRoutine(BIT_BUCKET_WRITE_START);
    e->router_delay= 0;
    e->hops= 0;
    e->msg_len= data_size;
    e->dest= -1;
    e->msg_id= (msg_seq++ << RANK_FIELD) | _my_rank;
    e->return_event= return_event;

    // NVRAM requests go out on port 0 of the aggregator
    e->route.push_back(0);

    // Coming back: Only one hop back to us
    my_core= _my_rank % _m->get_cores_per_node();
    e->reverse_route.push_back(my_core + 1);	// Port 0 goes off node

    // Send the write request
    delay= 0; // FIXME; Need to figure this out
    my_nvram_link->send(delay, e); 

}  /* end of nvram_write() */



// We assume a memcpy is the same as a "messsage send" between two cores
// Someday we may have to distinguish between cores and sockets.
SST::SimTime_t
Patterns::memdelay(int bytes)
{

    return nic[NoC]->delay(bytes);

}  // end of memdelay()



#ifdef SERIALIZATION_WORKS_NOW
BOOST_CLASS_EXPORT(Patterns)
#endif // SERIALIZATION_WORKS_NOW
