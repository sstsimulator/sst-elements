//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// Rolf Riesen, October 2011
//

#include <stdio.h>
#define __STDC_FORMAT_MACROS	(1)
#include <inttypes.h>		// For PRId64
#include <string>
#include "NIC_model.h"



const char *
type_name(NIC_model_t nic)
{
    switch (nic)   {
	case NoC:
	    return "NoC";
	case Net:
	    return "Net";
	case Far:
	    return "Far";
    }

    return "Unknown";

}  // end of type_name()



//
// Send a message into the Network on Chip with the appropriate
// delays.
//
SST::SimTime_t
NIC_model::send(SST::CPUNicEvent *e, int dest_rank, SST::SimTime_t CurrentSimTime)
{

SST::SimTime_t event_delay;
int64_t nic_delay;
int64_t link_duration;


    rtr->attach_route(e, dest_rank);

    // Calculate when this message can leave the NIC
    nstats->record_send(dest_rank, e->msg_len);
    nic_delay= get_NICparams(_m->NICparams[_nic], e->msg_len);

    // How long will this message occupy the outbound link?
    link_duration= e->msg_len / _m->get_LinkBandwidth(_nic);
    // FIXME: I don't think we need that!
    link_duration= 0;

    if (CurrentSimTime > NextSendSlot)   {
	// NIC is not busy

	// We subtract LinkLatency because the configuration tool already moved
	// some latency onto that link to help SST with synchronization
	event_delay= nic_delay - link_duration - _m->get_LinkLatency(_nic);

// For now until we have figured this out and start moving laency back onto links
assert(_m->get_LinkLatency(_nic) == 0); // For now
	if (event_delay < 0)   {
	    event_delay= 0;
	}
	send_link->Send(event_delay, e);
	NextSendSlot= CurrentSimTime + nic_delay + link_duration;
	// FIXME: Don't I need to subtract _m->get_LinkLatency(_nic)?

    } else   {
	// NIC is busy
	event_delay= NextSendSlot - CurrentSimTime + _m->get_NICgap(_nic) +
	    nic_delay - link_duration - _m->get_LinkLatency(_nic);
	if (event_delay < 0)   {
	    event_delay= 0;
	}
	send_link->Send(event_delay, e);
	nstats->record_busy(NextSendSlot - CurrentSimTime);
	// FIXME: check this
	NextSendSlot= CurrentSimTime + event_delay;
    }

    return event_delay;

}  // end of send()



// Based on message length and the NIC parameter list, extract the
// startup costs of a message of this length (latency) and the
// duration msg_len bytes will occupy the NIC. Return the sum of
// those two values.
int64_t
NIC_model::get_NICparams(std::list<NICparams_t> params, int64_t msg_len)
{

std::list<NICparams_t>::iterator k;
std::list<NICparams_t>::iterator previous;
double T, B;
double byte_cost;
int64_t latency;


    previous= params.begin();
    k= previous;
    k++;
    for (; k != params.end(); k++)   {
	if (k->inflectionpoint > msg_len)   {
	    T= k->latency - previous->latency;
	    B= k->inflectionpoint - previous->inflectionpoint;
	    byte_cost= T / B;
	    if (byte_cost < 0)   {
		byte_cost= 0;
	    }
	    // We want the values from the previous point
	    if (previous->latency < 0)   {
		latency= 0;
	    } else   {
		latency= previous->latency;
	    }
	    return (int64_t)((double)(msg_len - previous->inflectionpoint) * byte_cost) + latency;
	}
	previous++;
    }

    // We're beyond the list. Use the last two values in the list and
    // extrapolite.
    previous--;
    T= params.back().latency - previous->latency;
    B= params.back().inflectionpoint - previous->inflectionpoint;
    byte_cost= T / B;
    if (byte_cost < 0)   {
	byte_cost= 0;
    }
    if (previous->latency < 0)   {
	latency= 0;
    } else   {
	latency= previous->latency;
    }
    return (int64_t)((double)(msg_len - previous->inflectionpoint) * byte_cost) + latency;

}  // end of get_NICparams()



void
NIC_model::handle_rcv_events(SST::Event *sst_event)
{

SST::CPUNicEvent *e;


    e= (SST::CPUNicEvent *)sst_event;
    nstats->record_rcv(e->hops, e->congestion_cnt, e->congestion_delay,
	e->msg_len);

    // Turn this into a self event with 0 delay for now
    // FIXME: Take some of the total ping-pong latency and move it
    // here and subtract above on the send side
    _self_link->Send(0, e);

}  /* end of handle_rcv_events() */



#ifdef SERIALIZATION_WORKS_NOW
BOOST_CLASS_EXPORT(NIC_model)
#endif // SERIALIZATION_WORKS_NOW
