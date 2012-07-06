// -*- mode: c++ -*-

// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_RTRIF_H
#define COMPONENTS_MERLIN_RTRIF_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <queue>
#include <cstring>

using namespace SST;

class RtrEvent : public Event {

    typedef enum { credit, packet } msgType_t;
    
public:
    msgType_t type;
    int dest;
    int src_or_cred;
    int vc;
    int size_in_flits;
};

// Whole class definition needs to be in the header file so that other
// libraries can use the class to talk with the merlin routers.


typedef std::queue<RtrEvent*> network_queue_t;

// Class to manage link between NIC and router.  A single NIC can have
// more than one link_control (and thus link to router).
class link_control {
private:
    Link* rtr_link;
    Link* output_timing;
    
    int num_vns;
    int num_vcs;

    int* vn2vc_map;
    int* vc2vn_map;

    network_queue_t* input_buf;
    network_queue_t* output_buf;

    int* in_tokens;
    int* out_tokens;


public:
    link_control(RtrIF* rif, std::string port_name, TimeConverter* time_base, int vns, int vcs, int* vn2vc, int* vc2vn, int* out_tok) :
	num_vns(vns),
	num_vcs(vcs),
	vn2vc_map(vn2vc),
	vc2vn_map(vc2vn)
    {	// Buffers only for the number of VNs
	input_buf = new network_queue_t[vns];
	output_buf = new network_queue_t[vns];


	// Although there are only num_vns buffers, we will manage
	// things as if there were one buffer per vc
	in_tokens = new int[vcs];
	out_tokens = new int[vcs];

	// Copy the starting tokens for the output buffers (this
	// essentially sets the size of the buffer)
	memcpy(out_tokens,out_tok,vcs*sizeof(int));

	// The input tokens are set to zero and the other side of the
	// link will send the number of tokens.
	for ( int i = 0; i < vcs; i++ ) in_tokens[i] = 0;

	// Configure the links
	rtr_link = rif->configureLink(port_name, time_base, new Event::Handler<link_control>(this,&link_control::handle_input));
	output_timing = rtr->configureLink(port_name + "_output_timing", time_base,
					   new Event::Handler<link_control>(this,&link_control::handle_output));
    }

    void handle_input(Event* ev) {
    }
    void handle_output(Event* ev) {
    }
    
};

class RtrIF : public Component {

private:
    int num_vns;
    network_queue_t* input_bufs;
    network_queue_t* output_bufs;
    
public:
    RtrIF(ComponentId_t cid, Params& params) :
	Component(cid)
    {
	// Get the parameters
	num_vns = params.find_integer("num_vns");
	if ( num_vns == -1 ) {
	}

	
    }

};

#endif // COMPONENTS_MERLIN_RTRIF_H
