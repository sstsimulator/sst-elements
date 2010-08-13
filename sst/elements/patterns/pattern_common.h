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
typedef enum {START, COMPUTE_DONE, RECEIVE, FAIL, RESEND_MSG} pattern_event_t;

class Patterns   {
    public:
	Patterns()   {
	    // Nothing to do for now
	    my_net_link= NULL;
	    mesh_width= -1;
	    mesh_height= -1;
	    my_rank= -1;
	    net_latency= 0;
	    net_bandwidth= 0;
	}

	int init(int x, int y, int my_rank, SST::Link *net_link, SST::Link *self_link,
		SST::SimTime_t lat, SST::SimTime_t bw);
	void send(int dest, int len);
	void event_send(int dest, pattern_event_t event, SST::SimTime_t delay= 0,
		uint32_t msg_len= 0);


    private:
	SST::Link *my_net_link;
	SST::Link *my_self_link;
	int mesh_width;
	int mesh_height;
	int my_rank;
	SST::SimTime_t net_latency;	// in nano seconds
	SST::SimTime_t net_bandwidth;	// In bytes per second

} ;  // end of class Patterns


#endif  /* _PATTERN_COMMON_H */
