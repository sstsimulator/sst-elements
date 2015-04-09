//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// Rolf Riesen, October 2011
//

#include <sst_config.h>
#include <stdio.h>
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
NIC_model::send(CPUNicEvent *e, int dest_rank)
{

SST::SimTime_t current_time;
SST::SimTime_t event_delay;
int64_t nic_delay;
int64_t link_duration;


    current_time= (*NICtime_handler)(NICtime_obj);
    rtr->attach_route(e, dest_rank);

    // Calculate when this message can leave the NIC
    nstats->record_send(dest_rank, e->msg_len);
    nic_delay= get_NICparams(_m->NICparams[_nic], e->msg_len, _m->NICsend_fraction[_nic]);

    // How long will this message occupy the outbound link?
    link_duration= e->msg_len / _m->get_LinkBandwidth(_nic);
    // FIXME: I don't think we need that!
    link_duration= 0;

    if (current_time >= NextSendSlot)   {
	// NIC is not busy

	// We subtract LinkLatency because the configuration tool already moved
	// some latency onto that link to help SST with synchronization
	event_delay= nic_delay - link_duration - _m->get_LinkLatency(_nic);

// For now until we have figured this out and start moving latency back onto links
assert(_m->get_LinkLatency(_nic) == 0); // For now

//	if (event_delay < 0)   {
	if (false)   {      // NOTE: SimTime_t is unsigned and cannot be negative
	    event_delay= 0;
	}
	send_link->send(event_delay, e); 
	NextSendSlot= current_time + event_delay;

    } else   {
	// NIC is busy
	event_delay= NextSendSlot - current_time + _m->get_NICgap(_nic) +
	    nic_delay - link_duration - _m->get_LinkLatency(_nic);
//	if (event_delay < 0)   {
	if (false)   {      // NOTE: SimTime_t is unsigned and cannot be negative
	    event_delay= 0;
	}
	send_link->send(event_delay, e); 
	nstats->record_busy(NextSendSlot - current_time);
	NextSendSlot= current_time + event_delay;
    }

    return event_delay;

}  // end of send()



void
NIC_model::handle_rcv_events(SST::Event *sst_event)
{

CPUNicEvent *e;
int64_t nic_delay;
int64_t event_delay;
SST::SimTime_t current_time;


    current_time= (*NICtime_handler)(NICtime_obj);
    e= (CPUNicEvent *)sst_event;
    nstats->record_rcv(e->hops, e->congestion_cnt, e->congestion_delay,
	e->msg_len);

    // This message was delayed at the send side by _m->NICsend_fraction * the
    // message size dependent delay. Now it has to pay the remainder.
    nic_delay= get_NICparams(_m->NICparams[_nic], e->msg_len,
		    1.0 - _m->NICsend_fraction[_nic]);

    if (current_time >= NextRecvSlot)   {
	// Not busy
	event_delay= nic_delay;
    } else   {
	event_delay= NextRecvSlot - current_time + nic_delay;
    }
    _self_link->send(event_delay, e); 
    NextRecvSlot= current_time + event_delay;

}  /* end of handle_rcv_events() */



// Based on message length and the NIC parameter list, extract the
// startup costs of a message of this length (latency) and the
// duration msg_len bytes will occupy the NIC. Return the sum of
// those two values.
int64_t
NIC_model::get_NICparams(std::list<NICparams_t> params, int64_t msg_len, float send_fraction)
{

std::list<NICparams_t>::iterator k;
std::list<NICparams_t>::iterator previous;
double T, B;
double byte_cost;
int64_t latency;
double delay;


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
	    delay= ((double)(msg_len - previous->inflectionpoint) * byte_cost) + latency;
	    return (int64_t)(delay * send_fraction);
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
    delay= ((double)(msg_len - previous->inflectionpoint) * byte_cost) + latency;
    return (int64_t)(delay * send_fraction);

}  // end of get_NICparams()



// What is the delay for a given message length?
// memcpy and vector_op above us need to know.
SST::SimTime_t
NIC_model::delay(int bytes)
{

    return get_NICparams(_m->NICparams[_nic], bytes, 1.0);

}  // end of delay()



#ifdef SERIALIZATION_WORKS_NOW
BOOST_CLASS_EXPORT(NIC_model)
#endif // SERIALIZATION_WORKS_NOW
