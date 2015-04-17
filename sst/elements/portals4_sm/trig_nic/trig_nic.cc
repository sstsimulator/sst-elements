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


// #include <sys/stat.h>
// #include <fcntl.h>
#include "sst_config.h"
#include "sst/core/serialization.h"
#include "trig_nic.h"

#include <string.h>		       // for memcpy()

#include <sst/core/params.h>

// Need to find a better place to put this (this duplicates what is in
// portals_types.h).
#define HEADER_SIZE 64

using namespace SST;
using namespace SST::Portals4_sm;

trig_nic::trig_nic( ComponentId_t id, Params& params ) :
    RtrIF(id,params),
    msg_latency(40),
    ptl_latency(20),
    ptl_msg_latency(10),
    rtr_q_size(0),
    rtr_q_max_size(4),
    latency_ct_post(10),
    latency_ct_host_update(20),
    additional_atomic_latency(10),
    currently_clocking(true),
    dma_in_progress(false),
    rr_dma(false),
    new_dma(true),
    send_atomic_from_cache(true),
    next_out_msg_handle(0)
{

    clock_handler_ptr = new Clock::Handler<trig_nic>(this, &trig_nic::clock_handler);
    registerClock( frequency, clock_handler_ptr, false  );
 
    if ( params.find("latency") == params.end() ) {
	_abort(trig_nic,"couldn't find NIC latency\n");
    }

    if ( params.find("timing_set") == params.end() ) {
	_abort(trig_nic,"couldn't find timing set\n");
    }
    timing_set = strtol( params[ "timing_set" ].c_str(), NULL, 0 );

    setTimingParams(timing_set);

    nextToRtr = NULL;

    registerTimeBase("1ns");
    
    cpu_link = configureLink( "cpu", new Event::Handler<trig_nic>(this,&trig_nic::processCPUEvent) );

    // Setup a direct self link for messages headed to the router
    self_link = configureSelfLink( "self" );
    
    // Set up the portals link to delay incoming commands the
    // appropriate time.
    ptl_link = configureSelfLink("self_ptl", new Event::Handler<trig_nic>(this,&trig_nic::processPtlEvent) );

    // Set up the dma link
    dma_link = configureSelfLink("self_dma", new Event::Handler<trig_nic>(this,&trig_nic::processDMAEvent) );
    
    // Initialize all the portals elements

    // Portals table entries
    // For now initialize four portal table entries
    for ( int i = 0; i < MAX_PT_ENTRIES; i++ ) {
	ptl_entry_t* entry = new ptl_entry_t();
	ptl_table[i] = entry;
	entry->priority_list = new me_list_t;
	entry->overflow = new me_list_t;
	entry->overflow_headers = new overflow_header_list_t;
    }
	
    for (int i = 0; i < MAX_CT_EVENTS; i++ ) {
	ptl_ct_events[i].allocated = false;
    }  
    send_recv = false;
}


bool trig_nic::clock_handler ( Cycle_t cycle ) {
//     printf("clock_handler()\n");
    // All we do is look at the two queues and copy stuff across
//    bool work_done = false;
    if ( !toNicQ_empty( 0 ) ) {
//	work_done = true;
        RtrEvent* event = toNicQ_front( 0 );
        toNicQ_pop( 0 );

// 	if (m_id == 1) printf("Received packet @ %llu\n",getCurrentSimTimeNano());
	
	// OK, need to figure out how to route this.  If this is a
	// portals packet, it needs to go to the portals unit.  If
	// not, it goes straight to the host.

	// The first entry tells us if this a portals packet, which will
	// change the latency through the NIC
        bool portals = event->packet.payload[0] & PTL_HDR_PORTALS;
        bool head_packet = event->packet.payload[0] & PTL_HDR_HEAD_PACKET;

	// Either way we have to create a trig_nic_event, so do that
	// now.
	trig_nic_event* nic_event = new trig_nic_event();
	nic_event->src = event->packet.srcNum;
	nic_event->dest = event->packet.destNum;
	nic_event->ptl_op = PTL_NIC_PROCESS_MSG;
	nic_event->portals = portals;
	nic_event->head_packet = head_packet;
	
	nic_event->stream = event->packet.payload[1];
	
	// Copy over the payload
	memcpy(nic_event->ptl_data,&event->packet.payload[2],16*sizeof(uint32_t));
	
	delete event;

	if (portals) {
	    ptl_link->send(ptl_unit_latency,nic_event); 
	}
	else {
	    cpu_link->send(portals ? 0 : msg_latency,nic_event); 
	}
    }  

    // Need to send out any packets that need to go to the network.
    // We'll round robin the dma and pio to the network.
    bool adv_pio = false;
    bool adv_dma = false;

    // FIXME.  Works, but needs to be optimized.  Figure out which to
    // advance.  If the pio is headed to the router than we can
    // advance the dma regardless.
    adv_dma = (pio_q.empty() || rr_dma) && !dma_q.empty() && (rtr_q_size < rtr_q_max_size);
    adv_pio = (dma_q.empty() || !rr_dma) && !pio_q.empty() && (rtr_q_size < rtr_q_max_size);

    rr_dma = !rr_dma;


    if ( adv_pio ) {
//	work_done = true;
        trig_nic_event* ev = pio_q.front();
	if ( ev->ptl_op == PTL_NO_OP || ev->ptl_op == PTL_DMA ) {
	    if ( ev->head_packet ) {
		uint16_t handle = get_next_out_msg_handle();
		ev->updateMsgHandle(handle);
		out_msg_q[handle] = ev->msg_info;
		ev->msg_info = NULL;
// 		printf("%5d: handle = %d\n",m_id,handle);
	    }
	}
	    
	if ( ev->ptl_op == PTL_NO_OP ) {
	    
	    // This is a message destined for the router
	    // See if there is room in the output q
 	    if (rtr_q_size < rtr_q_max_size) {
		rtr_q_size++;
		pio_q.pop();
		
		self_link->send(ptl_msg_latency,ev); 

		// Need to return credits to the host
		trig_nic_event* event = new trig_nic_event;
		event->ptl_op = PTL_CREDIT_RETURN;
		event->data_length = 1;
		cpu_link->send(10,event); 
	    }
// 	    else {
// 		printf("qqq: %5d: no room in rtr queue, doing nothing\n",m_id);
// 	    }
	}
	else if ( ev->ptl_op == PTL_DMA ) {
	    // Start a DMA operation

	    // The header (and first few bytes of data) comes with the
	    // PIO packet.  I'm going to cheat a bit and just send
	    // this to the dma_q at let it be sent when it hits the
	    // front.  This maintains ordering, but makes the logic
	    // easier.  We also need to send the dma request to the
	    // dma_req_q.
	    pio_q.pop();

	    // Need to get the DMA structure from the event
	    bool empty = dma_req_q.empty();
// 	    dma_req_q.push(ev->operation->data.dma);
 	    dma_req_q.push(ev->data.dma);
	    if ( empty ) {
		dma_link->send(1,NULL); 
	    }
//  	    delete ev->operation;
	    
	    // Now clean up the event and send to dma_q
	    ev->ptl_op = PTL_NO_OP;
	    ev->portals = true;

	    dma_hdr_q.push(ev);
	    
// 		self_link->send(ptl_msg_latency,ev); 

	    // Need to return credits to the host
	    trig_nic_event* event = new trig_nic_event;
	    event->ptl_op = PTL_CREDIT_RETURN;
	    event->data_length = 1;
	    cpu_link->send(10,event); 

	}	    
	else {
	    // This is a portals command, send it to the "portals unit"
	    // through the ptl_link
	    pio_q.pop();
	    ptl_link->send(ptl_latency,ev); 

	    // Need to return credits to the host
	    trig_nic_event* event = new trig_nic_event;
	    event->ptl_op = PTL_CREDIT_RETURN;
	    event->data_length = 1;
	    cpu_link->send(10,event); 
	}
	
    }

    
    // Check to see if we have anything in the dma_q to send out
    if ( adv_dma ) {
//	work_done = true;
	    
	// See if there is room in the output q
	if (rtr_q_size < rtr_q_max_size) {
	rtr_q_size++;

	trig_nic_event* ev;
	// If new_dma is true, then we need to send the header first
	if ( new_dma ) {
	    if ( dma_hdr_q.empty() ) {
		printf("There should be a dma header, but there isn't\n");
		abort();
	    }
	    // Need to add special case for Atomic (will really just
	    // be for TriggeredAtomic).  Need to copy payload into
	    // single packet instead of multiple packets.
	    ev = dma_hdr_q.front();
	    dma_hdr_q.pop();
	    ev->ptl_op = PTL_NO_OP;
	    if ( ev->stream == PTL_HDR_STREAM_TRIG_ATOMIC ) {
		// Also need to get the data from the other event and
		// pop it from the dma_q.
		trig_nic_event* ev2 = dma_q.front();
		dma_q.pop();
		memcpy(&ev->ptl_data[sizeof(ptl_header_t)],ev2->ptl_data,8);
		self_link->send(ptl_msg_latency,ev); 
		delete ev2;
	    }
	    else {
		self_link->send(ptl_msg_latency,ev); 
		new_dma = false;
	    }
	}
	else {
	    ev = dma_q.front();
	    dma_q.pop();
            ev->ptl_op = PTL_NO_OP;
	    
	    // 	    if ( ev->operation->data.dma->end ) {
	    if ( ev->data.dma->end ) {
	        // Since this is the end, the next one through will
	        // need to send it's header first
	        new_dma = true;
 	        if ( ev->data.dma->ct_handle != PTL_CT_NONE && ev->stream == PTL_HDR_STREAM_GET ) {
		    scheduleCTInc(ev->data.dma->ct_handle,1,latency_ct_post);
		}
 	        if ( ev->data.dma->eq_handle != PTL_EQ_NONE && ev->stream == PTL_HDR_STREAM_GET ) {
		    scheduleEQ(ev->data.dma->eq_handle,ev->data.dma->event);		    
		}
	    }
	    // Don't need the dma data structure any more
 	    delete ev->data.dma;

	    self_link->send(ptl_msg_latency,ev); 
	}
	
// 	// If this is the end of the dma and there's a CT attached, we
// 	// need to increment the CT
// 	if ( !ev->head_packet ) {
// 	}
	

      }
    }

    
    // Check to see if something is ready to go out to the rtr, but only
    // if there isn't an event left over because the buffer was full
    if ( nextToRtr == NULL ) {
        trig_nic_event* to_rtr;
        if ( ( to_rtr = static_cast< trig_nic_event* >( self_link->recv() ) ) ) { 
// 	    printf("%5d:  Got something on my self link\n",m_id);
            nextToRtr = new RtrEvent();
            nextToRtr->type = RtrEvent::Packet;
            nextToRtr->packet.vc = 0;
            nextToRtr->packet.srcNum = m_id;
            nextToRtr->packet.destNum = to_rtr->dest;
            nextToRtr->packet.sizeInFlits = 8;

	    
	    nextToRtr->packet.payload[0] = 0;
            if (to_rtr->portals) nextToRtr->packet.payload[0] |= PTL_HDR_PORTALS;
            if (to_rtr->head_packet) nextToRtr->packet.payload[0] |= PTL_HDR_HEAD_PACKET;
	    nextToRtr->packet.payload[1] = to_rtr->stream;

	    memcpy(&nextToRtr->packet.payload[2],to_rtr->ptl_data,16*sizeof(uint32_t));
 	    delete to_rtr;
        }
    }
    if ( nextToRtr != NULL ) {
//	work_done = true;

	// 	printf("OK, really sending an event to router\n");
        if ( send2Rtr(nextToRtr) ) {
            // Successfully sent, so set nextToRtr to null so it will look
            // for new ones (otherwise, it will try to send the same one
            // again next time)
	    nextToRtr = NULL;
	    rtr_q_size--;
        }
    }
//     if ( !work_done) {
// 	currently_clocking = false;
// 	return true;
//     }
    return false;
}

void trig_nic::processCPUEvent( Event *e) {
//     printf("processCPUEvent\n");
    trig_nic_event* ev = static_cast<trig_nic_event*>(e);

//      if (m_id == 0 && ev->ptl_op == PTL_NO_OP) printf("NIC received packet @ %llu\n",getCurrentSimTimeNano());
//     if (m_id == 0 ) printf("NIC received a %d type packet @ %llu\n",ev->ptl_op,getCurrentSimTimeNano());
    
    // For now, just seperate into the DMA_Q and PIO_Q.
    if ( ev->ptl_op == PTL_DMA_RESPONSE ) {
	dma_q.push(ev);
    }
    else {
	pio_q.push(ev);
    }

    if ( !currently_clocking ) {
	registerClock( frequency, clock_handler_ptr, false  );
	currently_clocking = true;
    }
    
}

void
trig_nic::setTimingParams(int set) {
    if ( set == 1 ) {
	ptl_msg_latency = 25;
	ptl_unit_latency = 50;
	latency_ct_post = 25;
	latency_ct_host_update = 25;
    }
    if ( set == 2 ) {
	ptl_msg_latency = 75;
	ptl_unit_latency = 150;
	latency_ct_post = 75;
	latency_ct_host_update = 25;
    }
    if ( set == 3 ) {
	ptl_msg_latency = 100;
	ptl_unit_latency = 200;
	latency_ct_post = 100;
	latency_ct_host_update = 25;
    }
}

void trig_nic::processPtlEvent( Event *e ) {
//     printf("processPtlEvent\n");
    trig_nic_event* ev = static_cast<trig_nic_event*>(e);

    if ( !currently_clocking ) {
	registerClock( frequency, clock_handler_ptr, false  );
	currently_clocking = true;
    }    

    // See what portals command was sent
    switch (ev->ptl_op) {
	
    // Append an ME
    // TESTED
    case PTL_NIC_ME_APPEND_PRIORITY:
	{
	    // Add the int_me structure to the priority list for the
	    // portal table entry
// 	    printf("accessing ptl_table[%d]\n",ev->data.me->pt_index);
// 	    printf("%p\n",ev->data.me);
// 	    ptl_table[operation->data.me->pt_index]->priority_list->push_back(operation->data.me);

	    if ( ev->data.me->me.options & PTL_ME_MANAGE_LOCAL ) {
		printf("Locallaly managed offsets not supported for MEs on priority list\n");
		abort();
	    }

	    // Need to see if a header has already arrived for this ME
	    overflow_header_list_t* list = ptl_table[ev->data.me->pt_index]->overflow_headers;
	    overflow_header_list_t::iterator iter, curr;
	    bool found = false;
	    for ( iter = list->begin(); iter != list->end(); ) {
		curr = iter++;
		ptl_int_header_t* hdr = *curr;
		if ( (( hdr->header.match_bits ^ ev->data.me->me.match_bits ) & ~ev->data.me->me.ignore_bits ) == 0 ) {
		    found = true;
		    list->erase(curr);
		    // Found a match
		    // NOTE: for now, only USE_ONCE MEs are allowed to match
		    // in the overflow headers.
		    if ( !(ev->data.me->me.options & PTL_ME_USE_ONCE) ) {
			printf("Persistant ME matched an overflow header.  This is currently unsupported.\n");
			abort();
		    }

		    ptl_size_t len_rem = ev->data.me->me.length - hdr->header.remote_offset;
		    if ( len_rem < hdr->header.length ) {
			printf("Unsupported case in MEAppend:  ME matched overflow header, ");
			printf("but there's not enough room in ME buffer (offset: %ld, %ld, %ld)\n", 
                               (long) hdr->header.remote_offset, (long) len_rem, (long)hdr->header.length );
			abort();
		    }

		    // Generatate PUT_OVERFLOW event and send to host
		    ptl_event_t* ptl_event = NULL;
		    if ( ev->data.me->eq_handle != PTL_EQ_NONE ) {
			ptl_event = new ptl_event_t;
			ptl_event->type = PTL_EVENT_PUT_OVERFLOW;
			ptl_event->initiator = hdr->src;
			ptl_event->pt_index = hdr->header.pt_index;
// 			ptl_event->uid = ;
// 			ptl_event->jid = ;
			ptl_event->match_bits = hdr->header.match_bits;
			ptl_event->rlength = hdr->header.length;
//                      ptl_event->mlength = hdr->header.length;
			ptl_event->mlength = hdr->mlength;
 			ptl_event->remote_offset = hdr->header.remote_offset;
			ptl_event->start = (uint8_t*)ev->data.me->me.start + hdr->offset;
			ptl_event->user_ptr = ev->data.me->user_ptr;
			ptl_event->hdr_data = hdr->header.header_data;
			ptl_event->ni_fail_type = PTL_OK;

			scheduleEQ(ev->data.me->eq_handle,ptl_event);
		    }

		    // Need to increment the CT
		    ptl_size_t increment = (ev->data.me->me.options & PTL_ME_EVENT_CT_BYTES) ? hdr->mlength : 1;
		    scheduleCTInc( ev->data.me->me.ct_handle, increment, latency_ct_post );
		    // Done with event and ME structures
		    delete ev->data.me;
// 		    delete ev;

		    // Need to check for AUTO_FREE events
		    hdr->me->header_count--;
		    if ( hdr->me->header_count == 0 && !hdr->me->active ) {
			printf("AUTO_FREE\n");
			// Generate AUTO_FREE event
			
			if ( hdr->me->eq_handle != PTL_EQ_NONE ) {
			    ptl_event = new ptl_event_t;
			    ptl_event->type = PTL_EVENT_AUTO_FREE;
			    ptl_event->pt_index = hdr->header.pt_index;
			    ptl_event->user_ptr = hdr->me->user_ptr;
			    ptl_event->ni_fail_type = PTL_OK;
			    
			    scheduleEQ(ev->data.me->eq_handle,ptl_event);
			}
			// Done with the ME, delete it
			delete hdr->me;
		    }
		    // Done with hdr
		    delete hdr;
		    break;
		}
		
	    }	
	    
	    if ( !found ) ptl_table[ev->data.me->pt_index]->priority_list->push_back(ev->data.me);
	    delete ev;
	}
	break;

    case PTL_NIC_ME_APPEND_OVERFLOW:
	// Add the int_me structure to the priority list for the
	// portal table entry
// 	printf("accessing ptl_table[%d]\n",ev->data.me->pt_index);
// 	printf("%p\n",ev->data.me);
// 	ptl_table[operation->data.me->pt_index]->priority_list->push_back(operation->data.me);
	if ( ev->data.me->me.options & PTL_ME_USE_ONCE ) {
	    printf("Use Once MEs not supported on overflow list\n");
	    abort();
	}
	ptl_table[ev->data.me->pt_index]->overflow->push_back(ev->data.me);
	delete ev;
	break;

    // TESTED
    case PTL_NIC_PROCESS_MSG:
    {
	if ( ev->head_packet ) {

	    // Need to walk the list and look for a match
	    ptl_header_t header;
	  
	    memcpy(&header,ev->ptl_data,sizeof(ptl_header_t));

	    // need to see if this is a get response, in which case it
	    // is not matched
	    bool found = false;
	    bool overflow = false;
	    ptl_int_me_t* match_me;
	    ptl_size_t mlength;
	    ptl_size_t moffset;
	    if ( header.op == PTL_OP_GET_RESP ) {
		// There won't be any data in the head packet, so
		// just set up the stream entry
		// Get the outstanding msg info
		ptl_int_msg_info_t* msg_info = out_msg_q[header.out_msg_index];
		out_msg_q.erase(header.out_msg_index);

		MessageStream* ms = new MessageStream();
		ms->start = msg_info->get_start;
		ms->current_offset = 0;
		ms->remaining_length = header.length;
		ms->ct_handle = msg_info->ct_handle;
		ms->ct_increment = 1;  // FIXME
		ms->remaining_mlength = header.length;

		// Prepare the event to be delivered when this is done
		ptl_event_t* ptl_event = NULL;
		if ( msg_info->eq_handle != PTL_EQ_NONE ) {
		    ptl_event = new ptl_event_t;
		    ptl_event->type = PTL_EVENT_REPLY;
		    ptl_event->initiator = ev->src;
// 		    ptl_event->pt_index = header.pt_index;
// 		    ptl_event->uid = ;
// 		    ptl_event->jid = ;
// 		    ptl_event->match_bits = header.match_bits;
// 		    ptl_event->rlength = header.length;
 		    ptl_event->mlength = header.length;
// 		    ptl_event->remote_offset = header.remote_offset;
// 		    ptl_event->start = match_me->me.start;
		    ptl_event->user_ptr = msg_info->user_ptr;
// 		    ptl_event->hdr_data = header.header_data;
		    ptl_event->ni_fail_type = PTL_OK;
		}

		ms->eq_handle = msg_info->eq_handle;
		ms->event = ptl_event;

		int map_key = ev->src | ev->stream;
		
		streams[map_key] = ms;		  
		delete msg_info;
		delete ev;
		return;
	    }
	    else if (header.op == PTL_OP_ACK) {
		ptl_int_msg_info_t* msg_info = out_msg_q[header.out_msg_index];
		out_msg_q.erase(header.out_msg_index);

		bool ack_disabled = (header.header_data & PTL_ME_ACK_DISABLE);
		
		// Generate the ACK event
		if ( msg_info->eq_handle != PTL_EQ_NONE && !ack_disabled && msg_info->ack_req == PTL_ACK_REQ ) {   
		    ptl_event_t* ptl_event = new ptl_event_t;
		    ptl_event->type = PTL_EVENT_ACK;
// 		    ptl_event->initiator = ev->src;
// 		    ptl_event->pt_index = header.pt_index;
// 		    ptl_event->uid = ;
// 		    ptl_event->jid = ;
// 		    ptl_event->match_bits = header.match_bits;
// 		    ptl_event->rlength = header.length;
		    ptl_event->mlength = header.length;
 		    ptl_event->remote_offset = header.remote_offset;
// 		    ptl_event->start = match_me->me.start;
		    ptl_event->user_ptr = msg_info->user_ptr;
// 		    ptl_event->hdr_data = header.header_data;
		    ptl_event->ni_fail_type = PTL_OK;
		    scheduleEQ(msg_info->eq_handle,ptl_event);
		}
		
		if ( msg_info->ack_req == PTL_ACK_REQ || msg_info->ack_req == PTL_CT_ACK_REQ ) {
		    scheduleCTInc( msg_info->ct_handle, 1, latency_ct_post );
		}
		delete msg_info;
		delete ev;
		return;
	    }
	    else {  // Actually need to look for a match

		// Search priority list
		match_me = match_header(ptl_table[header.pt_index]->priority_list,&header,&moffset,&mlength);
		if (match_me != NULL ) {
		    found = true;
		    overflow = false;
		}
		else {
		    // Need to look on the overflow list
		    // Need to look on overflow list
		    match_me = match_header(ptl_table[header.pt_index]->overflow,&header,&moffset,&mlength);
		    if (match_me != NULL ) {
			found = true;
			overflow = true;

			// Need to put the header in the hidden header
			// queue which MEAppends to the priority_list
			// will look through before posting
			match_me->header_count++;
			ptl_int_header_t* ov_hdr = new ptl_int_header_t;
			ov_hdr->header = header;
			ov_hdr->me = match_me;
			ov_hdr->offset = moffset;
			ov_hdr->mlength = mlength;
			ov_hdr->src = ev->src;
			ptl_table[header.pt_index]->overflow_headers->push_back(ov_hdr);			
		    }
		}
	    }

	    if ( found ) {

		if ( header.op == PTL_OP_GET ) {
		    // Need to create a return header and a dma_req
		    ptl_header_t ret_header;

		    ret_header.pt_index = header.pt_index;
		    ret_header.op = PTL_OP_GET_RESP;
		    ret_header.length = header.length;
		    ret_header.out_msg_index = header.out_msg_index;

		    ret_header.header_data = 0;
		    ret_header.remote_offset = 0;
		    ret_header.match_bits = 0;
		    ret_header.options = 0;

		    trig_nic_event* event = new trig_nic_event;
		    event->src = m_id;
		    event->dest = ev->src;
		    event->portals = true;
		    event->head_packet = true;
		    event->stream = PTL_HDR_STREAM_GET;

		    memcpy(event->ptl_data,&ret_header,sizeof(ptl_header_t));

		    // Create DMA request
		    ptl_int_dma_t* dma = new ptl_int_dma_t;
		    dma->start = match_me->me.start;
		    dma->length = header.length;
// 		    dma->offset = header.remote_offset;
		    dma->offset = moffset;
		    dma->target_id = ev->src;
		    dma->ct_handle = match_me->me.ct_handle;
		    dma->stream = PTL_HDR_STREAM_GET;

		    // Prepare the event to be delivered when this is done
		    ptl_event_t* ptl_event = NULL;
		    if ( match_me->eq_handle != PTL_EQ_NONE ) {
			ptl_event = new ptl_event_t;
			ptl_event->type = PTL_EVENT_GET;
			ptl_event->initiator = ev->src;
			ptl_event->pt_index = header.pt_index;
// 			ptl_event->uid = ;
// 			ptl_event->jid = ;
			ptl_event->match_bits = header.match_bits;
			ptl_event->rlength = header.length;
			ptl_event->mlength = header.length;
// 			ptl_event->remote_offset = header.offset;
			ptl_event->remote_offset = moffset;
			ptl_event->start = ((uint8_t*)match_me->me.start) + moffset;
			ptl_event->user_ptr = match_me->user_ptr;
			ptl_event->hdr_data = header.header_data;
			ptl_event->ni_fail_type = PTL_OK;
		    }

		    dma->eq_handle = match_me->eq_handle;
		    dma->event = ptl_event;
		    
		    // Now put both of these in the correct queues
		    dma_hdr_q.push(event);

		    bool empty = dma_req_q.empty();
		    dma_req_q.push(dma);
		    if ( empty ) {
			dma_link->send(1,NULL); 
		    }
		  
		    delete ev;

		}
 		else if ( header.op == PTL_OP_ATOMIC ) {
		    // Figure out what the address is so we can look
		    // in the cache
		    unsigned long addr = ((unsigned long)match_me->me.start) + (unsigned long)header.remote_offset;
// 		    switch ( header.atomic_datatype ) {
// 		    case PTL_DOUBLE:
// 			{
// 			    double value;
// 			    memcpy(&value,&ev->ptl_data[8],8);
// 			    double result = computeDoubleAtomic(addr, value, header.atomic_op);
// 			    // Copy result back to payload to send to
// 			    // host memory.  It actually needs to go
// 			    // back to the front of ptl_data.
// 			    memcpy(ev->ptl_data,&result,8);
// 			}
// 			break;
		      
// 		    case PTL_INT:
			{
			    int64_t value;
			    memcpy(&value,&ev->ptl_data[sizeof(ptl_header_t)],8);
			    int64_t result = computeIntAtomic(addr, value, header.atomic_op);
			    // Copy result back to payload to send to
			    // host memory.  It actually needs to go
			    // back to the front of ptl_data.
			    memcpy(ev->ptl_data,&result,8);
			}
// 			break;
// 		    default:
// 			printf("Unsupported datatype for atomic operation, aborting...\n");
// 			exit(1);
// 		    }
		    // Send on to host memory
		    ev->data_length = 8;
		    ev->start = (void *)addr;
		    // Need to add more latency, ask keith how much
		    cpu_link->send(ptl_msg_latency+additional_atomic_latency,ev); 

		    scheduleCTInc( match_me->me.ct_handle, 1, latency_ct_post + additional_atomic_latency );

		    // Need to prepare an internal ack message back to src
		    trig_nic_event* ack_ev = new trig_nic_event;
		    ack_ev->src = m_id;
		    ack_ev->dest = ev->src;
		    ack_ev->ptl_op = PTL_NO_OP;
		    ack_ev->portals = true;
		    ack_ev->head_packet = true;
		    // Send back the out_msg_index
		    ptl_header_t* hdr = (ptl_header_t*)ack_ev->ptl_data;
		    hdr->op = PTL_OP_ACK;
		    hdr->out_msg_index = header.out_msg_index;
		    hdr->length = header.length;
		    
		    // Send ack back
		    self_link->send(ptl_msg_latency,ack_ev); 
		}
		else { // PUTs
	      
		    // Need to send the data to the cpu
		    ptl_size_t copy_length = header.length <= (HEADER_SIZE - sizeof(ptl_header_t)) ?
			header.length : (HEADER_SIZE - sizeof(ptl_header_t));
// 		    ptl_size_t remote_offset = header.remote_offset;
//		    ptl_size_t remote_offset = moffset;

		    if ( mlength < copy_length ) copy_length = mlength;
		    
		    ev->data_length = copy_length;
// 		    ev->start = (void*)((unsigned long)match_me->me.start+(unsigned long)remote_offset);
		    ev->start = (void*)((unsigned long)match_me->me.start+(unsigned long)moffset);
		    // Fix up the data, i.e. move actual data to top
		    if ( !send_recv ) memcpy(ev->ptl_data,&ev->ptl_data[sizeof(ptl_header_t)],copy_length);
		    cpu_link->send(ptl_msg_latency,ev); 

		    // Need to prepare an internal ack message back to src
		    trig_nic_event* ack_ev = new trig_nic_event;
		    ack_ev->src = m_id;
		    ack_ev->dest = ev->src;
		    ack_ev->ptl_op = PTL_NO_OP;
		    ack_ev->portals = true;
		    ack_ev->head_packet = true;

		    // Send back the out_msg_index
		    ptl_header_t* hdr = (ptl_header_t*)ack_ev->ptl_data;
		    hdr->op = PTL_OP_ACK;
// 		    hdr->out_msg_index = ((ptl_header_t*)ev->ptl_data)->out_msg_index;
 		    hdr->out_msg_index = header.out_msg_index;
		    hdr->length = mlength;
		    hdr->remote_offset = moffset;
		    hdr->header_data = match_me->me.options;

		    // Need to figure out how much to increment a counting event if there is one
		    ptl_size_t increment = (match_me->me.options & PTL_ME_EVENT_CT_BYTES) ? mlength : 1;
		    
		    // Prepare the event to be delivered when this is done
		    ptl_event_t* ptl_event = NULL;
		    if ( match_me->eq_handle != PTL_EQ_NONE && !overflow ) {
			ptl_event = new ptl_event_t;
			ptl_event->type = PTL_EVENT_PUT;
			ptl_event->initiator = ev->src;
			ptl_event->pt_index = header.pt_index;
// 			ptl_event->uid = ;
// 			ptl_event->jid = ;
			ptl_event->match_bits = header.match_bits;
			ptl_event->rlength = header.length;
			ptl_event->mlength = mlength;
// 			ptl_event->remote_offset = header.remote_offset;
			ptl_event->remote_offset = moffset;
			ptl_event->start = match_me->me.start;
			ptl_event->user_ptr = match_me->user_ptr;
			ptl_event->hdr_data = header.header_data;
			ptl_event->ni_fail_type = PTL_OK;
		    }
		    
		    // Handle mulit-packet messages
// 		    if ( (ev->stream == PTL_HDR_STREAM_TRIG || ev->stream == PTL_HDR_STREAM_GET) && copy_length != 0) {
		    if ( ev->stream == PTL_HDR_STREAM_TRIG && header.length != 0) {
			// No data in head packet for triggered
			MessageStream* ms = new MessageStream();
// 			ms->start = (void*)((unsigned long)match_me->me.start+(unsigned long)remote_offset);
			ms->start = (void*)((unsigned long)match_me->me.start+(unsigned long)moffset);
			ms->current_offset = 0;
			ms->remaining_length = header.length;
			ms->ct_handle = match_me->me.ct_handle;
			ms->ct_increment = increment;
			ms->ack_msg = ack_ev;
			ms->event = ptl_event;
			ms->eq_handle = overflow ? PTL_EQ_NONE : match_me->eq_handle;
			ms->remaining_mlength = mlength;
			
			int map_key = ev->src | ev->stream;
		      
			streams[map_key]= ms;
		      
		    }
		    else if ( header.length > (HEADER_SIZE - sizeof(ptl_header_t)) ) {
			MessageStream* ms = new MessageStream();
// 			ms->start = (void*)((unsigned long)match_me->me.start+(unsigned long)remote_offset);
			ms->start = (void*)((unsigned long)match_me->me.start+(unsigned long)moffset);
			ms->current_offset = copy_length;
			ms->remaining_length = header.length - copy_length;
 			ms->ct_handle = match_me->me.ct_handle;
			ms->ct_increment = increment;
			ms->ack_msg = ack_ev;
			ms->event = ptl_event;
			ms->eq_handle = overflow ? PTL_EQ_NONE : match_me->eq_handle;
			ms->remaining_mlength = mlength - copy_length;
			
			int map_key = ev->src | ev->stream;
		      
			streams[map_key]= ms;
		    }
		    else {  // Single packet message
                        // Send ack back
			self_link->send(ptl_msg_latency,ack_ev); 

			// If this is not multi-packet, then we need to
			// update a CT if one is attached.			
			scheduleCTInc( match_me->me.ct_handle, increment, latency_ct_post );
			if ( !overflow ) scheduleEQ( match_me->eq_handle, ptl_event );
		    }
	      
		}
	    }
	    else {
		printf("%d: Message arrived with no match in PT Entry %d @ %lu from %d\n",
		       m_id,header.pt_index,(unsigned long) getCurrentSimTimeNano(),ev->src);
		abort();
	    }
	  
	}
	// Not a head packet, so we need to figure out where it goes.
	else {
	    // Can distinguish message streams by looking at src.
	    MessageStream* ms;
	    int map_key = ev->src | ev->stream;
	    if ( streams.find(map_key) == streams.end() ) {
		printf("%5d:  Received a packet for multi-packet message but didn't get a corresponding head packet first: %x\n",
		       m_id,map_key);
		abort();
	    }
	    else {
		ms = streams[map_key];
	    }

	    // Reuse the same event, just forward in with the extra
	    // information
	    int copy_length = ms->remaining_length < 64 ? ms->remaining_length : 64;
	    ev->data_length = ms->remaining_mlength < copy_length ? ms->remaining_mlength : copy_length;
	    ev->start = (void*)((unsigned long)ms->start+(unsigned long)ms->current_offset);

	    if ( ms->remaining_mlength == 0 ) {
		// Drop packet
		delete ev;
	    }
	    else {
		cpu_link->send(ptl_msg_latency,ev); 

		// Compute remaining mlength
		if ( ms->remaining_mlength < copy_length ) {
		    ms->remaining_mlength = 0;
		}
		else {
		    ms->remaining_mlength -= copy_length;
		}
	    }


	    ms->current_offset += copy_length;
	    ms->remaining_length -= copy_length;


	    if ( ms->remaining_length == 0 ) {
		// Message is done, get rid of it.
		streams.erase(map_key);
		// Send ack back to src if this isn't a get
		if ( ev->stream != PTL_HDR_STREAM_GET) self_link->send(ptl_msg_latency, ms->ack_msg); 
		scheduleCTInc( ms->ct_handle, ms->ct_increment, latency_ct_post );
		scheduleEQ( ms->eq_handle, ms->event );
		delete ms;
	    }
	  
	}
	break;
    }
      
    
    // Attach a triggered operation
    case PTL_NIC_TRIG:
        // Need to see if event should fire immediately
	if ( PtlCTCheckThresh(ev->data.trig->trig_ct_handle,ev->data.trig->threshold) ) {
	    // Put the operation in the already_triggered_q.  If it
	    // was empty, we also need to send an event to wake it up.
	    bool empty = already_triggered_q.empty();
// 	    already_triggered_q.push(operation->data.trig);
 	    already_triggered_q.push(ev->data.trig);
	    if (empty) {
		// Repurpose the event we got.  Just need to wake up
		// again to process event
		ev->ptl_op = PTL_NIC_PROCESS_TRIG;
// 		operation->op_type = PTL_NIC_PROCESS_TRIG;
		ptl_link->send(1,ev); 
	    }
	    else {
		// Done with the op passed into the function, so delete it
		delete ev;
	    }
	}
	// Doesn't fire immediately, just add to counter
	else {
	    ptl_ct_events[ev->data.trig->trig_ct_handle].trig_op_list.push_back(ev->data.trig);
// 	    delete operation;
	    delete ev;
	}
	break;

    // Attach a vector of triggered operations
    case PTL_NIC_TRIG_PUTV:
	{
	bool wakeup_for_trig = false;
	for ( int i = 0; i < ev->data_length; i++ ) {
	    // Need to see if event should fire immediately
	    if ( PtlCTCheckThresh(ev->data.trigV[i]->trig_ct_handle,ev->data.trigV[i]->threshold) ) {
		// Put the operation in the already_triggered_q.  If it
		// was empty, we also need to send an event to wake it up.
		bool empty = already_triggered_q.empty();
		wakeup_for_trig = wakeup_for_trig || empty;
		already_triggered_q.push(ev->data.trigV[i]);
	    }
	    // Doesn't fire immediately, just add to counter
	    else {
		ptl_ct_events[ev->data.trigV[i]->trig_ct_handle].trig_op_list.push_back(ev->data.trigV[i]);
	    }
	}
	delete[] ev->data.trigV;
	if (wakeup_for_trig) {
	    // Repurpose the event we got.  Just need to wake up
	    // again to process event
	    ev->ptl_op = PTL_NIC_PROCESS_TRIG;
	    ptl_link->send(1,ev); 
	}
	else {
	    // Done with the op passed into the function, so delete it
	    delete ev;
	}
	}
	break;

    case PTL_NIC_PROCESS_TRIG:
	{ // Need because we declare a variable here
            // Get the head of the queue and do whatever it says to
            ptl_int_trig_op_t* op = already_triggered_q.front();
            already_triggered_q.pop();

	    // For all by CTInc, we need to put an entry in the
	    // outstanding messages queue.
	    if ( op->op->op_type != PTL_OP_CT_INC ) {
		uint16_t handle = get_next_out_msg_handle();
		op->op->ptl_header->out_msg_index = handle;
		out_msg_q[handle] = op->msg_info;
// 		op->msg_info = NULL;
// 		printf("%5d: trig handle = %d\n",m_id,handle);		
	    }
	    
            if ( op->op->op_type == PTL_OP_PUT || op->op->op_type == PTL_OP_ATOMIC ) {
                trig_nic_event* event = new trig_nic_event;
                event->src = ev->src;
                event->dest = op->op->target_id;
		if ( op->op->op_type == PTL_OP_PUT ) event->stream = PTL_HDR_STREAM_TRIG;
		else event->stream = PTL_HDR_STREAM_TRIG_ATOMIC;
	    
                event->portals = true;
		event->head_packet = true;
	    
		memcpy(event->ptl_data,op->op->ptl_header,sizeof(ptl_header_t));

		bool from_host = true;

		// Check to see if we want to send direct from NIC.
		// We send from NIC if size is 0 bytes, or if size is
		// 8-bytes and it is in the atomics cache.
		if ( op->op->ptl_header->length == 0 ) from_host = false;
		else if ( op->op->ptl_header->length <= 8 && send_atomic_from_cache && op->op->op_type == PTL_OP_ATOMIC ) {
		    // Get the address for the word
		    unsigned long addr = (unsigned long)op->op->dma->start + op->op->dma->offset;
		    if ( atomic_cache.find(addr) != atomic_cache.end() ) {
			from_host = false;
			// Copy data to event
			int64_t value = atomic_cache[addr]->int_val;
 			memcpy(&event->ptl_data[sizeof(ptl_header_t)],&value,8);
// 			printf("%d:  in cache: %lld\n",m_id,value);
		    }		    
		}
		
		if ( from_host ) {
		    dma_hdr_q.push(event);
		
		    // Also need to send the dma data to the dma engine
		    bool empty = dma_req_q.empty();
		    dma_req_q.push(op->op->dma);
		    if ( empty ) {
			dma_link->send(1,NULL); 
		    }
		}
		else {
		    rtr_q_size++;
		    self_link->send(ptl_msg_latency,event); 
		    
		    // If there's a CT attached to the MD, then we need
		    // to update the CT.
		    scheduleCTInc(op->op->dma->ct_handle, 1, latency_ct_post);
		    delete op->op->dma;
		}
                delete op->op;
                delete op;
            }
	    else if ( op->op->op_type == PTL_OP_GET ) {
	        // Need to just create an event, copy the header, and
	        // send it off.
	        trig_nic_event* event = new trig_nic_event;
                event->src = ev->src;
                event->dest = op->op->target_id;
		event->stream = PTL_HDR_STREAM_TRIG;
	    
                event->portals = true;
		event->head_packet = true;
	    
		memcpy(event->ptl_data,op->op->ptl_header,sizeof(ptl_header_t));
		// FIXME: Just send directly.  This may "overflow" the
		// buffer, but it will all work out.  Eventually, will
		// need to do sowething better (probably whatever I do
		// to make zero put triggered put work.
		rtr_q_size++;
 		self_link->send(ptl_msg_latency,event); 
// 		dma_hdr_q.push(event);
		
                delete op->op;
                delete op;
	      
	    }
            else if ( op->op->op_type == PTL_OP_CT_INC ) {
		scheduleCTInc(op->op->ct_handle, op->op->increment, latency_ct_post);
                delete op->op;
                delete op;
            }

            // See if we need to process anything else
            if ( already_triggered_q.empty() ) {
                // Don't need the op passed into the function, so delete it
// XXX                delete operation;
// XXX		delete ev;
            }
            else {
                // Repurpose the operation passed into the function
//                 operation->op_type = PTL_NIC_PROCESS_TRIG;
                ev->ptl_op = PTL_NIC_PROCESS_TRIG;
                // Can process the next event in 8ns
//                 cpu->ptl_link->send(8,new ptl_nic_event(operation)); 
		ptl_link->send(1,ev); 
            }
	}
	break;

    case PTL_NIC_CT_INC:
	{
	    // ptl_ct_events[ev->data.ct_handle].ct_event.success++;
	    ptl_ct_events[ev->data.ct->ct_handle].ct_event.success+= ev->data.ct->ct_event.success;
	    scheduleUpdateHostCT(ev->data.ct->ct_handle);

            // need to see if any triggered operations have hit
            trig_op_list_t* op_list = &ptl_ct_events[ev->data.ct->ct_handle].trig_op_list;
            trig_op_list_t::iterator op_iter, curr;
            for ( op_iter = op_list->begin(); op_iter != op_list->end();  ) {
                curr = op_iter++;
                ptl_int_trig_op_t* op = *curr;
                // For now just use the wait command, will need to change it later
                if ( PtlCTCheckThresh(ev->data.ct->ct_handle,op->threshold) ) {
                    // 	  if ( cpu->my_id == 0 ) {
                    // 	  }
                    // Simply move the operation to the already_triggered_q	      
                    bool empty = already_triggered_q.empty();
                    already_triggered_q.push(op);
                    if (empty) {
			ev->ptl_op = PTL_NIC_PROCESS_TRIG;
			ptl_link->send(1,ev); 
                    }
		    else {
// 			delete operation;
// 			delete ev;
		    }
                    op_list->erase(curr);
                }
            }
 	}
	break;
	
    case PTL_NIC_CT_SET:
	{
	    ptl_int_ct_alloc_t ct_alloc;
	    memcpy(&ct_alloc,ev->ptl_data,sizeof(ptl_int_ct_alloc_t));
// 	    int ct_handle = ev->ptl_data[0];
// 	    ptl_ct_events[ct_handle].ct_event.success = ev->ptl_data[1];
// 	    ptl_ct_events[ct_handle].ct_event.failure = ev->ptl_data[2];
// 	    if ( ev->ptl_data[3] ) { // Clear op_list
// 		ptl_ct_events[ev->ptl_data[0]].trig_op_list.clear();
// 	    }
	    
	    int ct_handle = ct_alloc.ct_handle;
	    ptl_ct_events[ct_handle].ct_event.success = ct_alloc.success;
	    ptl_ct_events[ct_handle].ct_event.failure = ct_alloc.failure;
	    if ( ct_alloc.clear_op_list ) { // Clear op_list
		ptl_ct_events[ev->ptl_data[0]].trig_op_list.clear();
	    }	    
	    
	    scheduleUpdateHostCT(ev->ptl_data[0]);
	}
	break;

    case PTL_NIC_INIT_FOR_SEND_RECV:
    {
	// Need to put in a catch all ME.  Start doesn't matter because
	// it just goes into a queue object for the host to process.
	ptl_int_me_t* int_me = new ptl_int_me_t;
	int_me->active = true;
	int_me->pt_index = 0;
	int_me->ptl_list = PTL_PRIORITY_LIST;
	
	int_me->me.start = NULL;
	int_me->me.length = 0;
	int_me->me.ct_handle = PTL_CT_NONE; 
	int_me->me.ignore_bits = ~0x0;
	int_me->me.options = 0;
	int_me->me.min_free = 0;
	
	ptl_table[0]->priority_list->push_back(int_me);
	send_recv = true;
	delete ev;
    }
      break;
    default:
      break;
	
    }
}

// Self-timed event handler that processes the DMA request Q.

// FIXME: Still need to add a credit based mechanism to limit
// outstanding DMAs when the dma_q is "full".
void
trig_nic::processDMAEvent( Event* e)
{
//     printf("processDMAEvent()\n");
    if ( !currently_clocking ) {
	registerClock( frequency, clock_handler_ptr, false  );
	currently_clocking = true;
    }
    // Look at the incoming queues from the host to see what to do.
    if ( !dma_in_progress ) {
        // Need to start a new DMA
      dma_req = dma_req_q.front();
      if ( dma_req_q.size() == 0 ) return;
      if ( dma_req == NULL ) {
	  if ( dma_req_q.size() != 0 ) {
	    printf("NULL got passed into dma_req_q, aborting...\n");
	    abort();
	  }
	  // Don't actually have a new dma to start
	  return;
	}
        dma_in_progress = true;

	dma_req_q.pop();
    }

    bool end = dma_req->length <= 64;
    int copy_length = end ? dma_req->length : 64;
	    
    trig_nic_event* request = new trig_nic_event;
    request->ptl_op = PTL_DMA_RESPONSE;
    request->data.dma = new ptl_int_dma_t;
    request->data.dma->start = dma_req->start;
    request->data.dma->length = copy_length;
    request->data.dma->offset = dma_req->offset;
    request->data.dma->target_id = dma_req->target_id;
    request->data.dma->end = end;
    request->data.dma->ct_handle = dma_req->ct_handle;
    request->data.dma->eq_handle = dma_req->eq_handle;
    request->data.dma->event = dma_req->event;
    
    request->stream = dma_req->stream;


    cpu_link->send(1,request); 
    
    if ( end ) {
      // Done with this DMA
      delete dma_req;
      dma_in_progress = false;
    }
    else {
      dma_req->offset += copy_length;
      dma_req->length -= copy_length;
    }

    if ( dma_in_progress || !dma_req_q.empty() ) {
      // Need to wake up again to process more DMAs
      dma_link->send(8,NULL); 
    }
}


void
trig_nic::scheduleCTInc(ptl_handle_ct_t ct_handle, ptl_size_t increment, SimTime_t delay) {
    if ( ct_handle != (ptl_handle_ct_t) PTL_CT_NONE ) {
//       printf("scheduleCTInc(%d)\n",ct_handle);
	ptl_update_ct_event_t* ct_update = new ptl_update_ct_event_t;
	ct_update->ct_event.success = increment;
	ct_update->ct_event.failure = 0;
	ct_update->ct_handle = ct_handle;
	
	
	trig_nic_event* event = new trig_nic_event;
	event->ptl_op = PTL_NIC_CT_INC;
	event->data.ct = ct_update;
	ptl_link->send(delay,event); 
    }
}

void
trig_nic::scheduleUpdateHostCT(ptl_handle_ct_t ct_handle) {
    ptl_update_ct_event_t* ct_update = new ptl_update_ct_event_t;
    ct_update->ct_event.success = ptl_ct_events[ct_handle].ct_event.success;
    ct_update->ct_event.failure = ptl_ct_events[ct_handle].ct_event.failure;
    ct_update->ct_handle = ct_handle;
    
//     ptl_int_nic_op_t* update_event = new ptl_int_nic_op_t;
//     update_event->op_type = PTL_NIC_UPDATE_CPU_CT;
//     update_event->data.ct = ct_update;
    
    trig_nic_event* event = new trig_nic_event;
    event->ptl_op = PTL_NIC_UPDATE_CPU_CT;
//     event->operation = update_event;
    event->data.ct = ct_update;

    cpu_link->send(latency_ct_host_update,event);     
}

void
trig_nic::scheduleEQ(ptl_handle_eq_t eq_handle, ptl_event_t* ptl_event) {
    if ( eq_handle == PTL_EQ_NONE ) return;
    trig_nic_event* event = new trig_nic_event;
    event->ptl_op = PTL_NIC_EQ;
    event->data.event = ptl_event;
    event->eq_handle = eq_handle;
    cpu_link->send(latency_ct_post + latency_ct_host_update + 2, event); 
}



double
trig_nic::computeDoubleAtomic(unsigned long addr, double value, ptl_op_t op)
{
    atomic_cache_entry* entry;
    if ( atomic_cache.find(addr) != atomic_cache.end() ) {
	entry = atomic_cache[addr];
    }
    else {
	entry = atomic_cache[addr] = new atomic_cache_entry;
	entry->dbl_val = 0.0;
    }

    switch (op) {
    case PTL_MIN:
	if ( entry->dbl_val > value ) entry->dbl_val = value;
	break;
    case PTL_MAX:
	if ( entry->dbl_val < value ) entry->dbl_val = value;
	break;
    case PTL_SUM:
	entry->dbl_val += value;
	break;
    case PTL_PROD:
	entry->dbl_val *= value;
	// For now, product not supported, let if fall through to
	// abort
	//	break;
    default:
	printf("Unsupported atomic, aborting...");
	exit(1);
	break;
    }
    return entry->dbl_val;
}

int64_t
trig_nic::computeIntAtomic(unsigned long addr, int64_t value, ptl_op_t op)
{
    atomic_cache_entry* entry;
    if ( atomic_cache.find(addr) != atomic_cache.end() ) {
	entry = atomic_cache[addr];
    }
    else {
	entry = atomic_cache[addr] = new atomic_cache_entry;
	// Need to actually request the data from the host, but for
	// now, just initialize to correct value.
	entry->int_val = *((int64_t*)addr);
    }

    switch (op) {
    case PTL_MIN:
	if ( entry->int_val > value ) entry->int_val = value;
	break;
    case PTL_MAX:
	if ( entry->int_val < value ) entry->int_val = value;
	break;
    case PTL_SUM:
	entry->int_val += value;
	break;
    case PTL_LAND:
	entry->int_val = entry->int_val && value;
	break;
    case PTL_PROD:
	entry->int_val *= value;
	// For now, product not supported, let if fall through to
	// abort
	//	break;
    default:
	printf("Unsupported atomic, aborting...");
	exit(1);
	break;
    }
    return entry->int_val;
}

ptl_int_me_t*
trig_nic::match_header(me_list_t* list, ptl_header_t* header, ptl_size_t* offset, ptl_size_t* length)
{
    ptl_int_me_t* match_me;
    bool found = false;
    ptl_size_t moffset, mlength;
    
    me_list_t::iterator iter, curr;
    for ( iter = list->begin(); iter != list->end(); ) {
	curr = iter++;
	ptl_int_me_t* me = *curr;
	// Check to see if me is still active, if not remove it
	if ( !me->active ) {
	    // delete the ME
	    delete me;
	    list->erase(curr);
	    continue;
	}
	if ( (( header->match_bits ^ me->me.match_bits ) & ~me->me.ignore_bits ) == 0 ) {
	    found = true;
	    match_me = me;
	    // Need to figure out the mlength
	    ptl_size_t len_rem = match_me->me.length - match_me->managed_offset;
	    if ( len_rem < header->length ) {
		if ( match_me->me.options & PTL_ME_NO_TRUNCATE ) {
		    printf("Received a message too long for buffer with truncate off\n");
		    abort();
		}
		else {
		    mlength = len_rem;
		}
	    }
	    else {
		mlength = header->length;
	    }
	    
	    if ( match_me->me.options & PTL_ME_MANAGE_LOCAL ) {
		moffset = match_me->managed_offset;
		match_me->managed_offset += mlength;
	    }
	    else {
		moffset = header->remote_offset;
	    }
	    
	    // If USE_ONCE is set, delete
	    if ( me->me.options & PTL_ME_USE_ONCE ) {
// 		delete me;
// 		list->erase(curr);
		// Need to delete, but can't do it now because the
		// data is still needed.  Just set inactive and it will
		// delete the next time around.
		me->active=false;
	    }
	    else if ( match_me->me.min_free != 0 ) {
		// Make sure there's enough room left
		if ( len_rem - mlength < match_me->me.min_free ) {
		    // Set inactive and remove from list.  ME will not
		    // get deleted, and the header object that matched
		    // will point to it for later use.
		    list->erase(curr);
		    me->active = false;
		    printf("Auto-unlinking\n");
		    // Need to send AUTO_UNLINK event
		    if ( match_me->eq_handle != PTL_EQ_NONE ) {
			ptl_event_t* ptl_event = NULL;
			ptl_event = new ptl_event_t;
			ptl_event->type = PTL_EVENT_AUTO_UNLINK;
			ptl_event->pt_index = header->pt_index;
			ptl_event->user_ptr = match_me->user_ptr;
			ptl_event->ni_fail_type = PTL_OK;
			scheduleEQ(match_me->eq_handle,ptl_event);
		    }
		}
	    }
	    break;
	}
    }
    
    if (found) {
	*length = mlength;
	*offset = moffset;
	return match_me;
    }
    return NULL;
}
