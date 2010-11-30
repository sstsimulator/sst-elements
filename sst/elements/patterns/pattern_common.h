/*
** Copyright 2009-2010 Sandia Corporation. Under the terms
** of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
** Government retains certain rights in this software.
**
** Copyright (c) 2009-2010, Sandia Corporation
** All rights reserved.
**
** This file is part of the SST software package. For license
** information, see the LICENSE file in the top level directory of the
** distribution.
*/

#ifndef _PATTERN_COMMON_H
#define _PATTERN_COMMON_H

#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (1)
#endif

#include <sst_config.h>
#include <sst/core/sst_types.h>
#include <sst/core/link.h>



// Event types sent among pattern generators
typedef enum {START,		// Enter first state of state machine
              COMPUTE_DONE,	// Finished a compute step
	      RECEIVE,		// Received a message
	      CHCKPT_DONE,	// A checkpoint has been written
	      FAIL,		// A failure occured on this rank
	      LOG_MSG_DONE,	// Done logging a message to stable (local) storage
	      ENV_LOG_DONE,	// Envelope write to log is done
	      ENTER_WAIT,	// Wait for four receives from neighbors
	      BCAST_DATA,	// Deal with a broadcast
	      REDUCE_DATA,	// Deal with a reduce

	      // Make sure you set this at 200 so we can talk to the bit_bucket component
	      BIT_BUCKET_WRITE_START= 200,
	      BIT_BUCKET_WRITE_DONE,
	      BIT_BUCKET_READ_START,
	      BIT_BUCKET_READ_DONE
} pattern_event_t;


// The checkpoint methods we support
typedef enum {CHCKPT_NONE, CHCKPT_COORD, CHCKPT_UNCOORD, CHCKPT_RAID} chckpt_t;

class Patterns   {
    public:
	Patterns()   {
	    // Nothing to do for now
	    my_net_link= NULL;
	    my_self_link= NULL;
	    mesh_width= -1;
	    mesh_height= -1;
	    NoC_width= -1;
	    NoC_height= -1;
	    total_cores= 1;
	    my_rank= -1;
	    net_latency= 0;
	    net_bandwidth= 0;
	    msg_seq= 1;
	}

	int init(int x, int y, int NoC_x_dim, int NoC_y_dim, int rank, int cores,
		SST::Link *net_link, SST::Link *self_link,
		SST::Link *NoC_link, SST::Link *nvram_link, SST::Link *storage_link,
		SST::SimTime_t net_lat, SST::SimTime_t net_bw, SST::SimTime_t node_lat,
		SST::SimTime_t node_bw, chckpt_t method, int chckpt_size,
		SST::SimTime_t chckpt_interval, int envelope_size);

	void send(int dest, int len);
	void send(SST::SimTime_t start_delay, int dest, int len);
	void event_send(int dest, pattern_event_t event, SST::SimTime_t delay= 0,
		uint32_t msg_len= 0, const char *payload= NULL, int payload_len= 0);
	void sendOB(int dest, pattern_event_t event, int value);
	void storage_write(int data_size, pattern_event_t return_event);
	void nvram_write(int data_size, pattern_event_t return_event);


    private:
	SST::Link *my_net_link;
	SST::Link *my_self_link;
	SST::Link *my_NoC_link;
	SST::Link *my_nvram_link;
	SST::Link *my_storage_link;
	int mesh_width;
	int mesh_height;
	int NoC_width;
	int NoC_height;
	int my_rank;
	int total_cores;
	int cores_per_router;
	int cores_per_node;
	SST::SimTime_t net_bandwidth;	// In bytes per second
	SST::SimTime_t node_bandwidth;	// In bytes per second
	SST::SimTime_t net_latency;	// in nano seconds FIXME: Variable not needed
	SST::SimTime_t node_latency;	// in nano seconds FIXME: Variable not needed
	uint64_t msg_seq;		// Each message event gets a unique number for debugging

} ;  // end of class Patterns


#endif  /* _PATTERN_COMMON_H */
