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


// This is a simple component that we use to simulate a disk or NVRAM
// used in a streaming mode. It basically pretends to have a pipe of
// a given bandwidth to which it streams write data. After a block of
// "data" has been written, and ACK is sent back. For reads, it enques
// the read request and sends the data when it has been able to pull the
// data out of the pipe. This is delayed by previous reads and limited
// by the read bandwidth.
//
// The read and write pipes are completly independent. We assume writers
// wait for ACK to come back before data is read, but there is no
// mechanism to veryfy this for independent readers. In fact, since no
// actual data is written or read, a read that in a real application is
// dependent on a write, can happen here without problems.


#include <sst_config.h>
#include <cpunicEvent.h>
#include "bit_bucket.h"



void
Bit_bucket::handle_events(Event *event)
{

bit_bucket_op_t op;
SimTime_t current_time;
SimTime_t delay;
SimTime_t write_time;
SimTime_t read_time;


    CPUNicEvent *e= static_cast<CPUNicEvent *>(event);
    op= (bit_bucket_op_t)e->GetRoutine();
    current_time= getCurrentSimTime();

    switch (op)   {
	case BIT_BUCKET_WRITE_START:
	    // How long until this write can start?
	    if (write_pipe > current_time)   {
		delay= write_pipe - current_time;
	    } else   {
		delay= 0;
	    }

	    // How long until it will finish?
	    // len & bw are in bytes; we want time in ns
	    write_time= ((uint64_t)e->msg_len * 1000000000) / write_bw;
	    delay += write_time;
	    total_write_delay += delay;
	    bytes_written += e->msg_len;
	    total_writes++;

	    write_pipe= delay + current_time;
	    _BIT_BUCKET_DBG(2, "%15.9fs Bit bucket: Starting write of %d bytes, delay %.9fs\n",
		(double)current_time / 1000000000.0, e->msg_len, (float)delay / 1000000000.0);

	    e->SetRoutine(BIT_BUCKET_WRITE_DONE);
	    self_link->send(delay, e);
	    break;

	case BIT_BUCKET_WRITE_DONE:
	    // OK, send the ACK back
	    _BIT_BUCKET_DBG(3, "%15.9fs Bit bucket: Write of %d bytes done.\n",
		(double)current_time / 1000000000.0, e->msg_len);
	    e->route= e->reverse_route;
	    e->hops= 0;
	    e->msg_len= 0;
	    // e->dest= -1;
	    e->SetRoutine(e->return_event);
	    net->send(e);
	    break;

	case BIT_BUCKET_READ_START:
	    // How long until this read can start?
	    if (read_pipe > current_time)   {
		delay= read_pipe - current_time;
	    } else   {
		delay= 0;
	    }

	    // How long until it will finish?
	    // len & bw are in bytes; we want time in ns
	    read_time= ((uint64_t)e->msg_len * 1000000000) / read_bw;
	    delay += read_time;
	    total_read_delay += delay;

	    read_pipe += delay;
	    _BIT_BUCKET_DBG(2, "%15.9fs Bit bucket: Starting read of %d bytes, delay %.9fs\n",
		(double)current_time / 1000000000.0, e->msg_len, (float)delay / 1000000000.0);
	    e->SetRoutine(BIT_BUCKET_READ_DONE);
	    self_link->send(delay, e);
	    break;

	case BIT_BUCKET_READ_DONE:
	    // OK, send the "data" back
	    e->route= e->reverse_route;
	    bytes_read += e->msg_len;
	    total_reads++;
	    _BIT_BUCKET_DBG(3, "%15.9fs Bit bucket: Read of %d bytes done.\n",
		(double)current_time / 1000000000.0, e->msg_len);
	    e->hops= 0;
	    // e->dest= -1;
	    e->SetRoutine(e->return_event);
	    net->send(e);
	    break;
    }

}  // end of handle_events()



/*
// THIS SECTION MOVED TO patterns.cc FOR RELEASE 3.x OF SST - ALEVINE

static SST::Component *
create_bit_bucket(SST::ComponentId_t id, SST::Params& params)
{
    return new Bit_bucket(id, params);
}

static const SST::ElementInfoComponent components[]= {
    {"bit_bucket", "Bit bucket", NULL, create_bit_bucket},
    {NULL, NULL, NULL, NULL}
};

extern "C" {
    SST::ElementLibraryInfo bit_bucket_eli= {
        "bit_bucket", "Trying to figure this out", components
    };
}
*/

