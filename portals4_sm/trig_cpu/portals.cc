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

#include "sst_config.h"
#include "sst/core/serialization.h"

#include "portals.h"
#include "trig_cpu.h"

#include "sst/elements/portals4_sm/trig_nic/trig_nic_event.h"

#include <string.h>		       // for memcpy()

using namespace std;
using namespace SST;
using namespace SST::Portals4_sm;

portals::portals(trig_cpu* my_cpu) {
    // For now initialize only one portal table entry
    ptl_entry_t* entry = new ptl_entry_t();
    ptl_table[0] = entry;
    entry->priority_list = new me_list_t;

    entry = new ptl_entry_t();
    ptl_table[1] = entry;
    entry->priority_list = new me_list_t;

    entry = new ptl_entry_t();
    ptl_table[2] = entry;
    entry->priority_list = new me_list_t;

    entry = new ptl_entry_t();
    ptl_table[3] = entry;
  
    entry->priority_list = new me_list_t;

    for (int i = 0; i < MAX_CT_EVENTS; i++ ) {
	ptl_ct_cpu_events[i].allocated = false;
    }  

    next_handle_me = 0;

    cpu = my_cpu;
    putv_active = false;
    currently_coalescing = false;
}


void
portals::PtlPTAlloc(unsigned int options, ptl_handle_eq_t eq_handle,
		    ptl_pt_index_t pt_index_req, ptl_pt_index_t *pt_index )
{
    if ( pt_entries.count(pt_index_req) == 0 ) {
	pt_entries[pt_index_req] = eq_handle;
	*pt_index = pt_index_req;
    }
    else {
	printf("PTAlloc failed:  PTE %d already allocated\n",pt_index_req);
	exit(1);
    }
}

void
portals::PtlEQAlloc(ptl_size_t count, ptl_handle_eq_t* eq_handle)
{
    event_queues.push_back(new event_queue_t());
    *eq_handle = event_queues.size() - 1;
}

void
portals::PtlEQFree(ptl_handle_eq_t eq_handle)
{
}

// Seems to work
void
portals::PtlMEAppend(ptl_pt_index_t pt_index, ptl_me_t me, ptl_list_t ptl_list, 
			  void *user_ptr, ptl_handle_me_t& me_handle)
{

    //   if ( cpu->my_id == 0 || cpu->my_id == 992 ) {
    //     printf("%5d: PtlMEAppend(%d) @ %lu\n",cpu->my_id,pt_index,cpu->getCurrentSimTimeNano());
    //   }
    // The ME actually gets appended on the NIC, so all we do is
    // create the ptl_int_me_t datatype and send it the NIC
    
    ptl_int_me_t* int_me = new ptl_int_me_t();
    int_me->me = me;
    int_me->active = true;
    int_me->user_ptr = user_ptr;
    int_me->pt_index = pt_index;
    int_me->ptl_list = ptl_list;
    int_me->managed_offset = 0;
    int_me->header_count = 0;

    
    if ( pt_entries.count(pt_index) == 0 ) {
	printf("Tried to MEAppend to non-allocated pt_index %d\n",pt_index);
	abort();
    }
    int_me->eq_handle = pt_entries[pt_index];
    
    // Send this to the NIC
    trig_nic_event *event = new trig_nic_event;
    event->src = cpu->my_id;
    if ( ptl_list == PTL_PRIORITY_LIST ) event->ptl_op = PTL_NIC_ME_APPEND_PRIORITY;
    else event->ptl_op = PTL_NIC_ME_APPEND_OVERFLOW;
	
    event->data.me = int_me;

    cpu->writeToNIC(event);
    // The processor is tied up for 1/msgrate
    cpu->busy += (cpu->delay_host_pio_write + (currently_coalescing ? 0 : cpu->delay_sfence));
    
    me_handle = int_me;
    return;
}

// FIXME: Need to fix this, can't do it this way anymore since the ME
// is on the NIC (at least not if the NIC and CPU are on different
// ranks).  This will work, but does not incur any timing overhead.
void
portals::PtlMEUnlink(ptl_handle_me_t me_handle) {
    //   ptl_int_me_t* int_me = me_map[me_handle];
    //   me_map.erase(me_handle);
    //   // For now ignore ptl_list
    //   // Need to delete from the list as well.  NOT YET DONE
    // //   me_list_t list = ptl_table[int_me->pt_index]->priority_list;
    me_handle->active = false;
}


// Works for all message sizes
void
portals::PtlPut ( ptl_handle_md_t md_handle, ptl_size_t local_offset, 
		  ptl_size_t length, ptl_ack_req_t ack_req, 
		  ptl_process_t target_id, ptl_pt_index_t pt_index,
		  ptl_match_bits_t match_bits, ptl_size_t remote_offset, 
		  void *user_ptr, ptl_hdr_data_t hdr_data)
{
    
    trig_nic_event* event = new trig_nic_event;
    event->src = cpu->my_id;
    event->dest = target_id;
    event->ptl_op = PTL_NO_OP;
    
    event->portals = true;
    event->latency = cpu->latency/2;
    event->head_packet = true;
    
    ptl_header_t header;
    header.pt_index = pt_index;
    header.op = PTL_OP_PUT;
    header.length = length;
    header.match_bits = match_bits;
    header.remote_offset = remote_offset;
    header.header_data = hdr_data;
    
    // Copy the header into the first half of the packet payload
    memcpy(event->ptl_data,&header,sizeof(ptl_header_t));

    // Put in the outstanding message info
    event->msg_info = new ptl_int_msg_info_t;
    event->msg_info->user_ptr  = user_ptr;
    event->msg_info->get_start = NULL;
    event->msg_info->ct_handle = md_handle->ct_handle;
    event->msg_info->eq_handle = md_handle->eq_handle;
    event->msg_info->ack_req = ack_req;

    
    // If this is a single packet message, just send it.  Otherwise,
    // we need to set up a multi-packet PIO or DMA.
    unsigned int hdr_data_size = PACKET_SIZE - sizeof(ptl_header_t);
//     if ( length <= (PACKET_SIZE - sizeof(ptl_header_t)) ) { // Single packet
    if ( length <= hdr_data_size ) { // Single packet
	// Copy over the payload.
	event->stream = PTL_HDR_STREAM_PIO;
	memcpy(&event->ptl_data[sizeof(ptl_header_t)],(void*)((unsigned long)md_handle->start+(unsigned long)local_offset),length);
      	cpu->writeToNIC(event);
	cpu->busy += (cpu->delay_host_pio_write + (currently_coalescing ? 0 : cpu->delay_sfence));

	// Need to send a CTInc if there is a ct_handle specified
// 	if ( md_handle->ct_handle != PTL_CT_NONE ) {
// 	    // Treat it like a PIO, but set the length the 0 and all
// 	    // that will happen is the CTInc will be sent
//   	    cpu->pio_in_progress = true;
// 	    pio_length_rem = 0;
// 	    pio_ct_handle = md_handle->ct_handle;
// 	}
    }
    else if ( length <= 2048 /*100000000*/ ) {  // PIO
	// 	if ( cpu->my_id == 0 ) printf("Starting PIO\n");
	memcpy(&event->ptl_data[sizeof(ptl_header_t)],(void*)((unsigned long)md_handle->start+(unsigned long)local_offset),hdr_data_size);
	cpu->pio_in_progress = true;

	pio_start = md_handle->start;
	pio_current_offset = hdr_data_size;
	pio_length_rem = length - hdr_data_size;
	pio_dest = target_id;
	pio_ct_handle = md_handle->ct_handle;
	
	event->stream = PTL_HDR_STREAM_PIO;
	cpu->writeToNIC(event);
	// FIXME.  Needs to match rate of interface (serialization delay)
	cpu->busy += (cpu->delay_host_pio_write + (currently_coalescing ? 0 : cpu->delay_sfence));
    }
    else {  // DMA
// 	if ( cpu->my_id == 0 ) printf("Starting DMA\n");
	ptl_int_dma_t* dma_req = new ptl_int_dma_t;

	// FIXME: Cheat for now, push header, data and DMA request all
	// at once.  This should really take two transfers.  Should
	// fix this.
	memcpy(&event->ptl_data[sizeof(ptl_header_t)],(void*)((unsigned long)md_handle->start+(unsigned long)local_offset),hdr_data_size);

	// Change event type to DMA
	event->ptl_op = PTL_DMA;
	event->stream = PTL_HDR_STREAM_DMA;

	// Fill in dma structure
	dma_req->start = md_handle->start;
	dma_req->length = length - hdr_data_size;
	dma_req->offset = local_offset + hdr_data_size;
	dma_req->target_id = target_id;
// 	dma_req->ct_handle = md_handle->ct_handle;
	dma_req->stream = PTL_HDR_STREAM_DMA;
	
	event->data.dma = dma_req;

	cpu->writeToNIC(event);
	cpu->busy += (cpu->delay_host_pio_write + (currently_coalescing ? 0 : cpu->delay_sfence));
	
    }
  
}

bool
portals::progressPIO(void)
{
//   if ( pio_length_rem == 0 && pio_ct_handle != PTL_CT_NONE) {
// 	// Need to send an increment to the CT attached to the MD
// 	trig_nic_event* event = new trig_nic_event;
// 	event->src = cpu->my_id;
// 	event->ptl_op = PTL_NIC_CT_INC;
// 	event->data.ct_handle = pio_ct_handle;
// 	event->ptl_op = PTL_NIC_CT_INC;
// 	cpu->writeToNIC(event);
// 	cpu->busy += cpu->delay_sfence;
// 	cpu->pio_in_progress = false;
// 	return true;
//     }
    
    trig_nic_event* event = new trig_nic_event;
    event->src = cpu->my_id;
    event->dest = pio_dest;
    event->ptl_op = PTL_NO_OP;

    event->portals = true;
    event->head_packet = false;
    event->stream = PTL_HDR_STREAM_PIO;

    int copy_length = pio_length_rem < 64 ? pio_length_rem : 64; 
    memcpy(event->ptl_data,(void*)((unsigned long)pio_start+(unsigned long)pio_current_offset),copy_length);
    cpu->writeToNIC(event);

    // Delay is just serialization delay at this point
    cpu->busy += cpu->delay_bus_xfer;
    
    pio_length_rem -= copy_length;
    pio_current_offset += copy_length;

//     if ( pio_length_rem == 0 && pio_ct_handle == PTL_CT_NONE ) {
//       cpu->pio_in_progress = false;
//       return true;
//     }
    if ( pio_length_rem == 0 ) {
      cpu->pio_in_progress = false;
      return true;
    }
    return false;
}

void
portals::PtlAtomic(ptl_handle_md_t md_handle, ptl_size_t local_offset,
			ptl_size_t length, ptl_ack_req_t ack_req,
			ptl_process_t target_id, ptl_pt_index_t pt_index,
			ptl_match_bits_t match_bits, ptl_size_t remote_offset,
			void *user_ptr, ptl_hdr_data_t hdr_data,
			ptl_op_t operation, ptl_datatype_t datatype) {
    
    trig_nic_event* event = new trig_nic_event;
    event->src = cpu->my_id;
    event->dest = target_id;
    event->ptl_op = PTL_NO_OP;

    event->portals = true;
    event->latency = cpu->latency/2;
    event->head_packet = true;

    ptl_header_t header;
    header.pt_index = pt_index;
    header.op = PTL_OP_ATOMIC;
    header.length = length;
    header.match_bits = match_bits;
    header.remote_offset = remote_offset;
    header.atomic_op = operation;
//     header.atomic_datatype = datatype;
    header.header_data = 0;
    
    // Copy the header into the first half of the packet payload
    memcpy(event->ptl_data,&header,sizeof(ptl_header_t));

    // Put in the outstanding message info
    event->msg_info = new ptl_int_msg_info_t;
    event->msg_info->user_ptr  = user_ptr;
    event->msg_info->get_start = NULL;
    event->msg_info->ct_handle = md_handle->ct_handle;
    event->msg_info->eq_handle = md_handle->eq_handle;
    event->msg_info->ack_req = ack_req;

    // Currently only support up to 8-bytes
    if ( length <= 8 ) { // Single packet
	event->stream = PTL_HDR_STREAM_PIO;
	memcpy(&event->ptl_data[sizeof(ptl_header_t)],(void*)((unsigned long)md_handle->start+(unsigned long)local_offset),length);
      	cpu->writeToNIC(event);
	cpu->busy += (cpu->delay_host_pio_write + (currently_coalescing ? 0 : cpu->delay_sfence));

// 	// Need to send a CTInc if there is a ct_handle specified
// 	if ( md_handle->ct_handle != PTL_CT_NONE ) {
// 	    // Treat it like a PIO, but set the length the 0 and all
// 	    // that will happen is the CTInc will be sent
//   	    cpu->pio_in_progress = true;
// 	    pio_length_rem = 0;
// 	    pio_ct_handle = md_handle->ct_handle;
// 	}
    }
    else {
	printf("Attempting to initiate an atomic greater than 8-bytes\n");
	abort();
    }
}


// Counting event methods

// Seems to work
void
portals::PtlCTAlloc(ptl_ct_type_t ct_type, ptl_handle_ct_t& ct_handle) {
    // Find the first counter that is free
    for (int i = 0; i < MAX_CT_EVENTS; i++ ) {
        if ( !ptl_ct_cpu_events[i].allocated ) {
	    ct_handle = i;
            ptl_ct_cpu_events[i].allocated = true;
            ptl_ct_cpu_events[i].ct_type = ct_type;
            ptl_ct_cpu_events[i].ct_event.success = 0;
            ptl_ct_cpu_events[i].ct_event.failure = 0;

	    // Need to send a message to the NIC to clear the counter
	    trig_nic_event* event = new trig_nic_event;
	    event->ptl_op = PTL_NIC_CT_SET;
	    ptl_int_ct_alloc_t ct_alloc;
	    ct_alloc.ct_handle = i;
	    ct_alloc.success = 0;
	    ct_alloc.failure = 0;
	    ct_alloc.clear_op_list = true;
	    memcpy(event->ptl_data,&ct_alloc,sizeof(ptl_int_ct_alloc_t));
// 	    event->ptl_data[0] = i;  // ct_handle
// 	    event->ptl_data[1] = 0;  // success value
// 	    event->ptl_data[2] = 0;  // failure value
// 	    event->ptl_data[3] = true;  // Clear op_list
	    cpu->writeToNIC(event);
	    cpu->busy += (cpu->delay_host_pio_write + (currently_coalescing ? 0 : cpu->delay_sfence));
	    
            break;
        }
    }
}

// FIXME: Leaves dangling resources (though they will be cleaned up if
// the counter is reallocated.
void
portals::PtlCTFree(ptl_handle_ct_t ct_handle) {
    // Should check to make sure there are not operations posted to the
    // event, but not going to for now.  OK, the spec says that is user
    // responsibility, so I'll just delete everything out of the list.
    ptl_ct_cpu_events[ct_handle].allocated = false;
//     ptl_ct_events[ct_handle].trig_op_list.clear(); 
}

void
portals::PtlCTGet(ptl_handle_ct_t ct_handle, ptl_ct_event_t *ev)
{
    ev->success = ptl_ct_cpu_events[ct_handle].ct_event.success;
    ev->failure = ptl_ct_cpu_events[ct_handle].ct_event.failure;
}

bool
portals::PtlCTWait(ptl_handle_ct_t ct_handle, ptl_size_t test) {
//   printf("%5d: PTLCTWait: %d, %d\n",cpu->my_id,test,ptl_ct_cpu_events[ct_handle].ct_event.success);
    if ( (ptl_ct_cpu_events[ct_handle].ct_event.success +
          ptl_ct_cpu_events[ct_handle].ct_event.failure ) >= test ) {
        cpu->waiting = false;
	return true;
    }
//     printf("waiting...\n");
    cpu->waiting = true;
    return false;
}

bool
portals::PtlCTCheckThresh(ptl_handle_ct_t ct_handle, ptl_size_t test) {
//     if ( (ptl_ct_events[ct_handle].ct_event.success +
//           ptl_ct_events[ct_handle].ct_event.failure ) >= test ) {
//         return true;
//     }
    return false;
}

bool
portals::PtlEQWait(ptl_handle_eq_t eq_handle, ptl_event_t* event) {
    if ( event_queues[eq_handle]->size() >= 1 ) {
	ptl_event_t* ev = event_queues[eq_handle]->front();
	event_queues[eq_handle]->pop();
	*event = *ev;
	delete ev;
        cpu->waiting = false;
	return true;
    }
    cpu->waiting = true;
    return false;
}

bool
portals::PtlEQPoll(int* return_value, ptl_handle_eq_t eq_handle, ptl_time_t timeout,
		   ptl_event_t* event) {
    // See if we ended because of a timeout
    if ( cpu->timed_out ) {
	cpu->poll_ev = NULL;
	cpu->timed_out = false;
	*return_value = PTL_EQ_EMPTY;
	return true;
    }

    if ( event_queues[eq_handle]->size() >= 1 ) {
	ptl_event_t* ev = event_queues[eq_handle]->front();
	event_queues[eq_handle]->pop();
	*event = *ev;
	delete ev;
        cpu->waiting = false;
	if ( cpu->poll_ev != NULL ) {
	    cpu->poll_ev->cancel();
	    cpu->poll_ev = NULL;
	}
	cpu->timed_out = false;
	*return_value = PTL_OK;
	return true;
    }

    // First, see if this is the first entry into this particular call
    if ( !cpu->waiting ) {
	cpu->waiting = true;
	cpu->timed_out = false;
	if ( (int64_t)timeout != PTL_TIME_FOREVER ) {
	    // First time in and didn't find a match.  Need to set up
	    // the timeout event
	    cpu->poll_ev = new trig_cpu_event(true);
	    cpu->self->send(timeout, cpu->poll_ev); 
	    return false;
	}
    }
    
    // This means we re-entered because a new event arrived, but it
    // wasn't to the right eq
    cpu->waiting = true;
    return false;
}

// Seems to work
void
portals::PtlTriggeredPut( ptl_handle_md_t md_handle, ptl_size_t local_offset, 
			  ptl_size_t length, ptl_ack_req_t ack_req, 
			  ptl_process_t target_id, ptl_pt_index_t pt_index,
			  ptl_match_bits_t match_bits, ptl_size_t remote_offset, 
			  void *user_ptr, ptl_hdr_data_t hdr_data,
			  ptl_handle_ct_t trig_ct_handle, ptl_size_t threshold) {

    // Create an op structure
    ptl_int_op_t* op = new ptl_int_op_t;
    op->op_type = PTL_OP_PUT;
    op->target_id = target_id;
    op->pt_index = pt_index;
    op->match_bits = match_bits;

    // Add the dma request that is used when triggered
    op->dma = new ptl_int_dma_t;
    op->dma->start = md_handle->start;
    op->dma->length = length;
    op->dma->offset = local_offset;
    op->dma->target_id = target_id;
    op->dma->stream = PTL_HDR_STREAM_TRIG;
//     op->dma->ct_handle = md_handle->ct_handle;
    
    // Add the header which will be sent when triggered
    op->ptl_header = new ptl_header_t;
    op->ptl_header->pt_index = pt_index;
    op->ptl_header->op = PTL_OP_PUT;
    op->ptl_header->length = length;
    op->ptl_header->match_bits = match_bits;
    op->ptl_header->remote_offset = remote_offset;
    op->ptl_header->header_data = hdr_data;

    
    // Create a triggered op structure
    ptl_int_trig_op_t* trig_op = new ptl_int_trig_op_t;
    trig_op->op = op; 
    trig_op->trig_ct_handle = trig_ct_handle;
    trig_op->threshold = threshold;
    
     // Put in the outstanding message info
    trig_op->msg_info = new ptl_int_msg_info_t;
    trig_op->msg_info->user_ptr  = user_ptr;
    trig_op->msg_info->get_start = NULL;
    trig_op->msg_info->ct_handle = md_handle->ct_handle;
    trig_op->msg_info->eq_handle = md_handle->eq_handle;
    trig_op->msg_info->ack_req = ack_req;
    
    
    if ( !putv_active ) {
	// Need to send this object to the NIC with latency = 30% latency
	trig_nic_event* event = new trig_nic_event;
	event->src = cpu->my_id;
	event->ptl_op = PTL_NIC_TRIG;
	event->data.trig = trig_op;

	cpu->writeToNIC(event);
	
	//     cpu->busy += cpu->delay_host_pio_write;
	cpu->busy += (cpu->delay_host_pio_write + (currently_coalescing ? 0 : cpu->delay_sfence));
    }
    else {
	// Just add this to the list to send to nic when
	// PtlEndTriggeredPutV is called.
	if ( putv_event->data_length == putv_curr ) {
	    printf("%d: Too many TriggeredPuts to vectored call.\n",cpu->my_id);
	    exit(1);
	}
	putv_event->data.trigV[putv_curr++] = trig_op;
	cpu->busy += 2;
    }
}

void
portals::PtlTriggeredPutV(ptl_handle_md_t md_handle, ptl_size_t local_offset, 
			  ptl_size_t length, ptl_ack_req_t ack_req, 
			  ptl_process_t* target_ids, ptl_size_t id_length, ptl_pt_index_t pt_index,
			  ptl_match_bits_t match_bits, ptl_size_t remote_offset, 
			  void *user_ptr, ptl_hdr_data_t hdr_data,
			  ptl_handle_ct_t trig_ct_handle, ptl_size_t threshold) {    
}

void
portals::PtlStartTriggeredPutV(ptl_size_t id_length) {
    if ( !cpu->enable_putv ) return;
    
    putv_event = new trig_nic_event;
    putv_event->src = cpu->my_id;
    putv_event->data.trigV = new ptl_int_trig_op_t*[id_length];
    putv_event->data_length = id_length;
    putv_event->ptl_op = PTL_NIC_TRIG_PUTV;
    putv_curr = 0;
    putv_active = true;
}

void
portals::PtlEndTriggeredPutV() {
    if ( !cpu->enable_putv ) return;
    if ( putv_event->data_length != putv_curr ) {
	printf("%d:  Called PtlEndTriggeredPutV before all puts were posted.  Got %d, expected %d\n",cpu->my_id,putv_curr,putv_event->data_length);
	exit(1);
    }
    cpu->writeToNIC(putv_event);
    putv_curr = 0;
    putv_active = false;
    cpu->busy += cpu->delay_host_pio_write + cpu->delay_sfence;
}

void
portals::PtlTriggeredAtomic(ptl_handle_md_t md_handle, ptl_size_t local_offset,
			    ptl_size_t length, ptl_ack_req_t ack_req,
			    ptl_process_t target_id, ptl_pt_index_t pt_index,
			    ptl_match_bits_t match_bits, ptl_size_t remote_offset,
			    void *user_ptr, ptl_hdr_data_t hdr_data,
			    ptl_op_t operation, ptl_datatype_t datatype,
			    ptl_handle_ct_t trig_ct_handle, ptl_size_t threshold) {

    // Create an op structure
    ptl_int_op_t* op = new ptl_int_op_t;
    op->op_type = PTL_OP_ATOMIC;
    op->target_id = target_id;
    op->pt_index = pt_index;
    op->match_bits = match_bits;
    op->atomic_op = operation;
//     op->atomic_datatype = datatype;

    // Add the dma request that is used when triggered
    op->dma = new ptl_int_dma_t;
    op->dma->start = md_handle->start;
    op->dma->length = length;
    op->dma->offset = local_offset;
    op->dma->target_id = target_id;
    op->dma->stream = PTL_HDR_STREAM_TRIG;
    op->dma->ct_handle = md_handle->ct_handle;
    
    // Add the header which will be sent when triggered
    op->ptl_header = new ptl_header_t;
    op->ptl_header->pt_index = pt_index;
    op->ptl_header->op = PTL_OP_ATOMIC;
    op->ptl_header->length = length;
    op->ptl_header->match_bits = match_bits;
    op->ptl_header->remote_offset = remote_offset;
    op->ptl_header->atomic_op = operation;
//     op->ptl_header->atomic_datatype = datatype;

    
    // Create a triggered op structure
    ptl_int_trig_op_t* trig_op = new ptl_int_trig_op_t;
    trig_op->op = op;
    trig_op->trig_ct_handle = trig_ct_handle;
    trig_op->threshold = threshold;

     // Put in the outstanding message info
    trig_op->msg_info = new ptl_int_msg_info_t;
    trig_op->msg_info->user_ptr  = user_ptr;
    trig_op->msg_info->get_start = NULL;
    trig_op->msg_info->ct_handle = md_handle->ct_handle;
    trig_op->msg_info->eq_handle = md_handle->eq_handle;
    trig_op->msg_info->ack_req = ack_req;

    
    // Need to send this object to the NIC with latency = 30% latency
    trig_nic_event* event = new trig_nic_event;
    event->src = cpu->my_id;
    event->ptl_op = PTL_NIC_TRIG;
    event->data.trig = trig_op;
    cpu->writeToNIC(event);
    
//     cpu->busy += cpu->delay_host_pio_write;
    cpu->busy += (cpu->delay_host_pio_write + (currently_coalescing ? 0 : cpu->delay_sfence));

}

void
portals::PtlCTInc(ptl_handle_ct_t ct_handle, /*ptl_ct_event_t*/ ptl_size_t increment) {
    // Need to send an increment to the CT attached to the MD
    ptl_update_ct_event_t* ct_update = new ptl_update_ct_event_t;
    ct_update->ct_event.success = increment;
    ct_update->ct_event.failure = 0;
    ct_update->ct_handle = ct_handle;

    trig_nic_event* event = new trig_nic_event;
    event->src = cpu->my_id;
    event->ptl_op = PTL_NIC_CT_INC;
    event->data.ct = ct_update;
    
    cpu->writeToNIC(event);
}

void
portals::PtlTriggeredCTInc(ptl_handle_ct_t ct_handle, ptl_size_t increment,
			   ptl_handle_ct_t trig_ct_handle, ptl_size_t threshold) {
    
    // Create an op structure
    ptl_int_op_t* op = new ptl_int_op_t;
    op->op_type = PTL_OP_CT_INC;
    op->ct_handle = ct_handle;
    op->increment = increment;

    // Create a triggered op structure
    ptl_int_trig_op_t* trig_op = new ptl_int_trig_op_t;
    trig_op->op = op;
    trig_op->trig_ct_handle = trig_ct_handle;
    trig_op->threshold = threshold;

    // Need to send this object to the NIC with latency = 30% latency
    trig_nic_event* event = new trig_nic_event;
    event->src = cpu->my_id;
    event->ptl_op = PTL_NIC_TRIG;
//     event->operation = new ptl_int_nic_op_t;
//     event->operation->op_type = PTL_NIC_TRIG;
//     event->operation->data.trig = trig_op;
    event->ptl_op = PTL_NIC_TRIG;
    event->data.trig = trig_op;
    cpu->nic->send(event); 
    
    // The processor is tied up for 1/msgrate
//     cpu->busy += cpu->delay_host_pio_write;
    cpu->busy += (cpu->delay_host_pio_write + (currently_coalescing ? 0 : cpu->delay_sfence));
}

void
portals::PtlMDBind(ptl_md_t md, ptl_handle_md_t *md_handle)
{
    // Need to copy the information to a new md that we keep around
    ptl_md_t* new_md = new ptl_md_t(md);
    *md_handle = new_md;

    // Add some busy time
    cpu->busy += cpu->defaultTimeBase->convertFromCoreTime(cpu->registerTimeBase("100ns", false)->getFactor());
}

void
portals::PtlMDRelease(ptl_handle_md_t md_handle)
{
    delete md_handle;
}

void
portals::PtlGet ( ptl_handle_md_t md_handle, ptl_size_t local_offset, 
		  ptl_size_t length, ptl_process_t target_id, 
		  ptl_pt_index_t pt_index, ptl_match_bits_t match_bits, 
		  void *user_ptr, ptl_size_t remote_offset ) {

    trig_nic_event* event = new trig_nic_event;
    event->src = cpu->my_id;
    event->dest = target_id;
    event->ptl_op = PTL_NO_OP;
    
    event->portals = true;
    event->latency = cpu->latency/2;
    event->head_packet = true;
    
    ptl_header_t header;
    header.pt_index = pt_index;
    header.op = PTL_OP_GET;
    header.length = length;
    header.match_bits = match_bits;
    header.remote_offset = remote_offset;
//     header.get_ct_handle = md_handle->ct_handle;
//     header.get_start = (void*)((unsigned long)md_handle->start+(unsigned long)local_offset);
    header.header_data = 0;

    // Copy the header into the first half of the packet payload
    memcpy(event->ptl_data,&header,sizeof(ptl_header_t));

    // Put in the outstanding message info
    event->msg_info = new ptl_int_msg_info_t;
    event->msg_info->user_ptr  = user_ptr;
    event->msg_info->get_start = (void*)((unsigned long)md_handle->start+(unsigned long)local_offset);
    event->msg_info->ct_handle = md_handle->ct_handle;
    event->msg_info->eq_handle = md_handle->eq_handle;

    cpu->writeToNIC(event);
}

void
portals::PtlTriggeredGet ( ptl_handle_md_t md_handle, ptl_size_t local_offset, 
			   ptl_size_t length, ptl_process_t target_id, 
			   ptl_pt_index_t pt_index, ptl_match_bits_t match_bits, 
			   void *user_ptr, ptl_size_t remote_offset,
			   ptl_handle_ct_t ct_handle, ptl_size_t threshold) {
    if ( length >0x80000000 ) {
	printf("%5d: Bad length passed into PtlTriggeredGet(): %d\n",cpu->my_id,length);
	abort();
    }
    
    // Create an op structure
    ptl_int_op_t* op = new ptl_int_op_t;
    op->op_type = PTL_OP_GET;
    op->target_id = target_id;
    op->pt_index = pt_index;
    op->match_bits = match_bits;

    // Add the header which will be sent when triggered
    op->ptl_header = new ptl_header_t;
    op->ptl_header->pt_index = pt_index;
    op->ptl_header->op = PTL_OP_GET;
    op->ptl_header->length = length;
//     printf("ptltriggeredget: return header length = %d\n",length);
    op->ptl_header->match_bits = match_bits;
    op->ptl_header->remote_offset = remote_offset;    
//     op->ptl_header->get_ct_handle = md_handle->ct_handle;
//     op->ptl_header->get_start = (void*)((unsigned long)md_handle->start+(unsigned long)local_offset);

    // Create a triggered op structure
    ptl_int_trig_op_t* trig_op = new ptl_int_trig_op_t;
    trig_op->op = op;
    trig_op->trig_ct_handle = ct_handle;
    trig_op->threshold = threshold;
    
     // Put in the outstanding message info
    trig_op->msg_info = new ptl_int_msg_info_t;
    trig_op->msg_info->user_ptr  = user_ptr;
    trig_op->msg_info->get_start = (void*)((unsigned long)md_handle->start+(unsigned long)local_offset);;
    trig_op->msg_info->ct_handle = md_handle->ct_handle;
    trig_op->msg_info->eq_handle = md_handle->eq_handle;

    // Need to send the objectto the NIC
    trig_nic_event* event = new trig_nic_event;
    event->portals = true;
    event->head_packet = true;
    event->src = cpu->my_id;
    event->dest = target_id;
    event->ptl_op = PTL_NIC_TRIG;
//     event->operation = new ptl_int_nic_op_t;
//     event->operation->op_type = PTL_NIC_TRIG;
//     event->operation->data.trig = trig_op;
    event->ptl_op = PTL_NIC_TRIG;
    event->data.trig = trig_op;
        
    cpu->writeToNIC(event);
}



void
portals::scheduleUpdateHostCT(ptl_handle_ct_t ct_handle) {
}

// This is where incoming messages are processed
// bool portals::processMessage(int src, uint32_t* ptl_data) {
bool
portals::processMessage(trig_nic_event* ev) {
//      printf("portals::processMessage()\n");
    // May be returning credits
    if ( ev->ptl_op == PTL_CREDIT_RETURN ) {
      cpu->returnCredits(ev->data_length);
	delete ev;
    }
    else if ( ev->ptl_op == PTL_NIC_UPDATE_CPU_CT ) {
      ptl_ct_cpu_events[ev->data.ct->ct_handle].ct_event.success = 
	    ev->data.ct->ct_event.success;
	ptl_ct_cpu_events[ev->data.ct->ct_handle].ct_event.failure = 
	    ev->data.ct->ct_event.failure;
	delete ev->data.ct;
	delete ev;
	if ( cpu->waiting ) {
// 	    printf("Calling wakeup @ %llu\n",cpu->getCurrentSimTimeNano());
	    // Need to wakeup the CPU;
	    cpu->wakeUp();
	}
    }
    else if ( ev->ptl_op == PTL_NIC_EQ ) {
	event_queues[ev->eq_handle]->push(ev->data.event);
	delete ev;
	if ( cpu->waiting ) {
	    // Need to wakeup the CPU;
	    cpu->wakeUp();
	}
	
    }
    else if ( ev->ptl_op == PTL_DMA_RESPONSE ) {
      // Fill data into the request and send it back
	void* start = ev->data.dma->start;
	ptl_size_t offset = ev->data.dma->offset;
	ptl_size_t length = ev->data.dma->length;
	memcpy(ev->ptl_data,(void*)((unsigned long)start+(unsigned long)offset),length);
	
	// Fill in the rest of the event
	ev->src = cpu->my_id;
	ev->dest = ev->data.dma->target_id;
	ev->portals = true;
	ev->head_packet = false;
	
	// Need to add latency to memory
	cpu->dma_return_link->send(cpu->latency_dma_mem_access,ev); 
    }
    else {
      // In the case of send/recv (cpu->use_portals == false), we need
      // to just put the data into the received messages buffer.
      if ( cpu->use_portals ) {
	// The list is on the NIC, so all we get here is requests to put
	// data in memory.
	memcpy(ev->start,ev->ptl_data,ev->data_length);
	delete ev;
      }
      else {
	cpu->pending_msg.push(ev);
	if ( cpu->waiting )
	  cpu->wakeUp();
      }
    }

    return false;
    
}

void
portals::PtlEnableCoalesce(void) {
    if ( cpu->enable_coalescing ) currently_coalescing = true;
}

void
portals::PtlDisableCoalesce(void) {
    if ( cpu->enable_coalescing ) {
	currently_coalescing = false;
	cpu->busy += cpu->delay_sfence;
    }
}

