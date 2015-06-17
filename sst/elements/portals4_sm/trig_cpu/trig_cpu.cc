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


#include <sst_config.h>
#include "sst/core/serialization.h"

#include <sst/core/element.h>

#include "trig_cpu.h"
#include "sst/elements/portals4_sm/trig_nic/trig_nic_event.h"
#include "sst/elements/portals4_sm/trig_cpu/trig_cpu_event.h"

#include "apps/allreduce_recdbl.h"
#include "apps/allreduce_recdbl_trig.h"
#include "apps/allreduce_tree.h"
#include "apps/allreduce_tree_trig.h"
#include "apps/bcast.h"
#include "apps/bcast_trig.h"
#include "apps/barrier_tree.h"
#include "apps/barrier_tree_trig.h"
#include "apps/barrier_recdbl.h"
#include "apps/barrier_recdbl_trig.h"
#include "apps/barrier_dissem.h"
#include "apps/barrier_dissem_trig.h"
#include "apps/pingpong-bw-eager.h"
#include "apps/test_portals.h"
#include "apps/test_mpi.h"
#include "apps/ping_pong.h"
#include "apps/bandwidth.h"
#include "apps/test_atomics.h"
#include "apps/eq_test.h"
#include "barrier_action.h"

// SimTime_t trig_cpu::min = 10000000;
// SimTime_t trig_cpu::max = 0;
// SimTime_t trig_cpu::total_time = 0;
// int trig_cpu::total_num = 0;

// SimTime_t trig_cpu::overall_min = 1000000;
// SimTime_t trig_cpu::overall_max = 0;
// SimTime_t trig_cpu::overall_total_time = 0;
// int trig_cpu::overall_total_num = 0;

using namespace SST::Portals4_sm;

bool trig_cpu::rand_init = false;

barrier_action* trig_cpu::barrier_act = NULL;

// Infrastructure for doing more than one allreduce in a single
// simulation
// Link** trig_cpu::wake_up = NULL;
// int trig_cpu::current_link = 0;
// int trig_cpu::total_nodes;
// int trig_cpu::num_remaining;

trig_cpu::trig_cpu(ComponentId_t id, Params& params) :
    Component( id ),
    out(Simulation::getSimulation()->getSimulationOutput()),
    delay_host_pio_write(8),
    delay_bus_xfer(16),
    latency_dma_mem_access(1), // latency built in already
    params( params ),
    nic_timing_wakeup_scheduled(false),
    dma_return_count(0),
    frequency( "1GHz" ),
    wc_buffers_max(8)
{
    if ( params.find("nodes") == params.end() ) {
        out.fatal(CALL_INFO, -1,"couldn't find number of nodes\n");
    }
    num_nodes = strtol( params[ "nodes" ].c_str(), NULL, 0 );
    
    if ( params.find("id") == params.end() ) {
        out.fatal(CALL_INFO, -1,"couldn't find node id\n");
    }
    my_id = strtol( params[ "id" ].c_str(), NULL, 0 );

    if ( params.find("timing_set") == params.end() ) {
	out.fatal(CALL_INFO, -1,"couldn't find timing set\n");
    }
    timing_set = strtol( params[ "timing_set" ].c_str(), NULL, 0 );

    setTimingParams(timing_set);
    if ( params.find("msgrate") == params.end() ) {
        out.fatal(CALL_INFO, -1,"couldn't find msgrate\n");
    }
    std::string msg_rate = params[ "msgrate" ];

    if ( params.find("xDimSize") == params.end() ) {
        out.fatal(CALL_INFO, -1,"couldn't find xDimSize\n");
    }
    x_size = strtol( params[ "xDimSize" ].c_str(), NULL, 0 );

    if ( params.find("yDimSize") == params.end() ) {
        out.fatal(CALL_INFO, -1,"couldn't find yDimSize\n");
    }
    y_size = strtol( params[ "yDimSize" ].c_str(), NULL, 0 );

    if ( params.find("zDimSize") == params.end() ) {
        out.fatal(CALL_INFO, -1,"couldn't find zDimSize\n");
    }
    z_size = strtol( params[ "zDimSize" ].c_str(), NULL, 0 );

    if ( params.find("latency") == params.end() ) {
        out.fatal(CALL_INFO, -1,"couldn't find latency\n");
    }
    latency = strtol( params[ "latency" ].c_str(), NULL, 0 );
    //     latency = latency / 2;

    if ( params.find("radix") == params.end() ) {
	out.fatal(CALL_INFO, -1,"couldn't find radix\n");
    }
    radix = strtol( params[ "radix" ].c_str(), NULL, 0 );

    if ( params.find("msg_size") == params.end() ) {
	out.fatal(CALL_INFO, -1,"couldn't find msg_size\n");
    }
    msg_size = strtol( params[ "msg_size" ].c_str(), NULL, 0 );

    if ( params.find("chunk_size") == params.end() ) {
	out.fatal(CALL_INFO, -1,"couldn't find chunk_size\n");
    }
    chunk_size = strtol( params[ "chunk_size" ].c_str(), NULL, 0 );

    if ( params.find("coalesce") == params.end() ) {
	out.fatal(CALL_INFO, -1,"couldn't find coalesce\n");
    }
    enable_coalescing = (0 != (strtol( params[ "coalesce" ].c_str(), NULL, 0 )));

    if ( params.find("enable_putv") == params.end() ) {
	out.fatal(CALL_INFO, -1,"couldn't find coalesce\n");
    }
    enable_putv = 0 != (strtol( params[ "enable_putv" ].c_str(), NULL, 0 ));


//     TimeConverter* tc = registerTimeBase( frequency );
    TimeConverter* tc = registerTimeBase( "1ns" );
    
    if ( params.find("noiseRuns") == params.end() ) {
	out.fatal(CALL_INFO, -1,"couldn't find noiseRuns\n");
    }
    noise_runs = strtol( params[ "noiseRuns" ].c_str(), NULL, 0 );

    if ( noise_runs == 0 ) {
        noise_interval = 0;
        noise_duration = 0;
    }
    else {
        if ( params.find("noiseInterval") == params.end() ) {
	    out.fatal(CALL_INFO, -1,"couldn't find noiseInterval\n");
	}
	noise_interval =
	    defaultTimeBase->convertFromCoreTime(registerTimeBase(params["noiseInterval"],
                                                                  false)->getFactor());
	
	if ( params.find("noiseDuration") == params.end() ) {
	    out.fatal(CALL_INFO, -1,"couldn't find noiseDuration\n");
	}
	noise_duration =
	    defaultTimeBase->convertFromCoreTime(registerTimeBase(params["noiseDuration"],
                                                                  false)->getFactor());
    }
    do_noise = false;
    
    if (params.find("application") == params.end()) {
        out.fatal(CALL_INFO, -1, "couldn't find application\n");
    }
    std::string application = params["application"];

    initPortals();
    use_portals = true;

    if (application == "allreduce.tree") {
        use_portals = false;
        app = new allreduce_tree(this, false);
    } else if (application == "allreduce.narytree") {
        use_portals = false;
        app = new allreduce_tree(this, true);
    } else if (application == "allreduce.recursive_doubling") {
        use_portals = false;
        app = new allreduce_recdbl(this);
    } else if (application == "allreduce.tree_triggered") {
        use_portals = true;
        app = new allreduce_tree_triggered(this, false);
    } else if (application == "allreduce.narytree_triggered") {
        use_portals = true;
        app = new allreduce_tree_triggered(this, true);
    } else if (application == "allreduce.recursive_doubling_triggered") {
        use_portals = true;
        app = new allreduce_recdbl_triggered(this);
    } else if (application == "bcast.tree") {
        use_portals = false;
        app = new bcast_tree(this);
    } else if (application == "bcast.tree_triggered") {
        app = new bcast_tree_triggered(this);
    } else if (application == "barrier.tree") {
        use_portals = false;
        app = new barrier_tree(this);
    } else if (application == "barrier.recursive_doubling") {
        use_portals = false;
        app = new barrier_recdbl(this);
    } else if (application == "barrier.dissemination") {
        use_portals = false;
        app = new barrier_dissemination(this);
    } else if (application == "barrier.tree_triggered") {
        app = new barrier_tree_triggered(this);
    } else if (application == "barrier.recursive_doubling_triggered") {
        app = new barrier_recdbl_triggered(this);
    } else if (application == "barrier.dissemination_triggered") {
        app = new barrier_dissemination_triggered(this);
    } else if (application == "test_portals" ) {
        app = new test_portals(this);
    } else if (application == "test_mpi" ) {
	use_portals = false;
        app = new test_mpi(this);
    } else if (application == "ping_pong" ) {
        app = new ping_pong(this);
    } else if (application == "bw.eager") {
        app = new pingpong_bw(this, "eager", false);
    } else if (application == "bw.probe") {
        app = new pingpong_bw(this, "probe", false);
    } else if (application == "bw.rndv") {
        app = new pingpong_bw(this, "rndv", false);
    } else if (application == "bw.triggered") {
        app = new pingpong_bw(this, "triggered", false);
    } else if (application == "bw.two") {
        app = new pingpong_bw(this, "two", false);
    } else if (application == "bw.two-probe") {
        app = new pingpong_bw(this, "two-probe", false);
    } else if (application == "bw.eager.delay") {
        app = new pingpong_bw(this, "eager", true);
    } else if (application == "bw.probe.delay") {
        app = new pingpong_bw(this, "probe", true);
    } else if (application == "bw.rndv.delay") {
        app = new pingpong_bw(this, "rndv", true);
    } else if (application == "bw.triggered.delay") {
        app = new pingpong_bw(this, "triggered", true);
    } else if (application == "bw.two.delay") {
        app = new pingpong_bw(this, "two", true);
    } else if (application == "bw.two-probe.delay") {
        app = new pingpong_bw(this, "two-probe", true);
    } else if (application == "bandwidth" ) {
        app = new bandwidth(this);
    } else if (application == "test_atomics" ) {
        app = new test_atomics(this);
    } else if (application == "eq_test" ) {
        app = new eq_test(this);
    } else {
        out.fatal(CALL_INFO, -1, "Invalid application: %s\n", application.c_str());
    }
    
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    nic = configureLink("nic",new Event::Handler<trig_cpu>(this,&trig_cpu::processEventPortals));

    self = configureSelfLink("self", new Event::Handler<trig_cpu>(this,&trig_cpu::event_handler));
    nic_timing_link = configureSelfLink("nic_timing_link", new Event::Handler<trig_cpu>(this,&trig_cpu::event_nic_timing) );
    dma_return_link = configureSelfLink("dma_return_link", new Event::Handler<trig_cpu>(this,&trig_cpu::event_dma_return ) );
    pio_delay_link = configureSelfLink("pio_delay_link", new Event::Handler<trig_cpu>(this,&trig_cpu::event_pio_delay) );


    outstanding_msg = 0;
    top_state = ((noise_runs == 0) ? 0 : 2);
    current_run = 0;

    // Now, convert the msgrate to a delay.  First, get a
    // TimeConverter from the
    // string (should be in hertz).  Then, the factor is the number of
    // core time steps.  Then convert that number to the number of
    // clock cycles using this component's TimeConverter.
    msg_rate_delay =
        tc->convertFromCoreTime(registerTimeBase(msg_rate,false)->getFactor());

    timing_ev = new trig_cpu_event;
    poll_ev = NULL;
    timed_out = false;
    
//     if ( use_portals ) {
//         ptl = new portals(this);
      
//         EventHandler_t* ptlLinkHandler = new EventHandler<
//             trig_cpu, bool, Event* >
//             (this,&trig_cpu::ptlNICHandler );
//         ptl_link = selfLink("ptl", ptlLinkHandler);
//         ptl_link->setDefaultTimeBase(registerTimeBase("1ns",false));
//     }
} 

void
trig_cpu::initPortals() {
    ptl = new portals(this);
    
//     EventHandler_t* ptlLinkHandler = new EventHandler<
//     trig_cpu, bool, Event* >
// 	(this,&trig_cpu::ptlNICHandler );
//     ptl_link = selfLink("ptl", ptlLinkHandler);
//     ptl_link->setDefaultTimeBase(registerTimeBase("1ns",false));
}

void trig_cpu::setup() 
{
    busy = 0;
    recv_handle = 0;

//     if (my_id == 0 ) {
// 	setTotalNodes(num_nodes);
// 	resetBarrier();
//     }

    if ( barrier_act == NULL ) {
	barrier_act = new barrier_action();
	barrier_act->resetBarrier();
    }
    
    noise_count = barrier_act->getRand(my_id,noise_interval);
//     printf("%5d: %lu\n",my_id,noise_count);
    waiting = false;
    self->send(1,timing_ev); 
    count = 0;
    barrier_act->addWakeUp(self);

    nic_credits = 128;
    blocking = false;

    pio_in_progress = false;

    
    if ( (sizeof(ptl_header_t) & 8) != 0 ) {
	fprintf(stderr,
		"Portals header (ptl_header_t) must be a multiple of 8-bytes (actual = %d), aborting...\n",
		(int) sizeof(ptl_header_t));
	exit(1);
    }

    if ( !use_portals ) {
      trig_nic_event* event = new trig_nic_event;
      event->src = my_id;
      event->ptl_op = PTL_NIC_INIT_FOR_SEND_RECV;

      nic->send(1,event); 

      
    }
}

void trig_cpu::finish()  
{
    if (my_id == 0 ) barrier_act->printOverallStats();
}

int
trig_cpu::calcXPosition( int node )
{
    return node % x_size; 
}

int
trig_cpu::calcYPosition( int node )
{
    return ( node / x_size ) % y_size; 
}

int
trig_cpu::calcZPosition( int node )
{
    return node / ( x_size * y_size ); 
}

int
trig_cpu::calcNodeID(int x, int y, int z)
{
    return ( z * ( x_size*y_size ) ) + ( y * x_size ) + x;
}

void
trig_cpu::setTimingParams(int set) {
    if ( set == 1 ) {
//  	delay_host_pio_write = 75;
	delay_host_pio_write = 25;
	delay_sfence = 50;
	added_pio_latency = 0;
	recv_overhead = 100;
    }
    if ( set == 2 ) {
// 	delay_host_pio_write = 100;
	delay_host_pio_write = 50;
	delay_sfence = 50;
	added_pio_latency = 0;
	recv_overhead = 175;
    }
    if ( set == 3 ) {
// 	delay_host_pio_write = 200;
	delay_host_pio_write = 100;
	delay_sfence = 100;
	added_pio_latency = 100;
	recv_overhead = 300;
    }

//     printf("delay_host_pio_write = %d\n",delay_host_pio_write);
//     printf("added_pio_latency = %d\n",added_pio_latency);
}

void
trig_cpu::event_pio_delay(Event* e)
{
  trig_nic_event* ev = static_cast<trig_nic_event*>(e);
//   printf("%5d: putting event type %d into wc_buffers @ %llu\n",my_id,ev->ptl_op,getCurrentSimTimeNano());
//   if (ev->ptl_op == PTL_NIC_ME_APPEND) printf("   ME Append\n");
  wc_buffers.push(ev);

  // If this is the only entry, then we need to wakeup the interface
  // to transfer the data.  Otherwise, it will wakeup itself
  if ( !nic_timing_wakeup_scheduled ) {
    nic_timing_link->send(1,NULL); 
    nic_timing_wakeup_scheduled = true;
  }
}



// This attempts to write to the write combined buffers to the NIC.
// If they are full, then the CPU will stall until there's space
bool
trig_cpu::writeToNIC(trig_nic_event* ev)
{
//     printf("trig_cpu::writeToNIC()\n");
    // Need to see if we have any credits to use
    if ( nic_credits != 0 ) {
        // Put this through the delay link.  Only do host delay time,
        // not sfence time in delay.
        pio_delay_link->send(delay_host_pio_write+delay_bus_xfer,ev); 
// 	// Put the event into the write combining buffers
// 	wc_buffers.push(ev);
	nic_credits--;

	return true;
    }
    else {
//       printf("No credits available\n");
	// No credits, need to wait until we get some.
        blocking = true;
	waiting = false;

//  	printf("%5d: Ran out of credits @ %llu\n",my_id,getCurrentSimTimeNano());
	
	// Need to keep track of what is blocking so we can send it
	// once we get credits.  It's done this way to make the "user"
	// code easier to write.
	blocked_event = ev;
 	blocked_busy = busy;
	return false;
    }
}

void
trig_cpu::returnCredits(int num)
{
    nic_credits += num;
    if ( blocking ) {
	blocking = false;
	writeToNIC(blocked_event);
	busy += blocked_busy;
	wakeUp();
//  	printf("Blocking, but got credit back @ %llu\n",getCurrentSimTimeNano());
    }
}


void
trig_cpu::event_nic_timing(Event* e)
{
  // Need to arbitrate between PIO and DMA traffic.  For now just give
  // DMA traffic priority, we shouldn't see them both at the same time
  // for the current sims
  
  if ( dma_buffers.size() != 0 ) {
    nic->send(0,dma_buffers.front()); 
    dma_buffers.pop();
  }
  else if ( wc_buffers.size() != 0 ) {
//     if ( my_id == 0 && wc_buffers.front()->ptl_op == PTL_NO_OP )
//     if ( my_id == 0 )
//       printf("%5d:  Writing event type %d from WCs to bus @ %llu\n",my_id,wc_buffers.front()->ptl_op,getCurrentSimTimeNano());

    nic->send(added_pio_latency,wc_buffers.front()); 
    wc_buffers.pop();
  }

    // If there's still events left, make myself up again
  if ( wc_buffers.size() != 0 || dma_buffers.size() != 0 ) {
    nic_timing_link->send(delay_bus_xfer,NULL); 
    nic_timing_wakeup_scheduled = true;
  }
  else {
    nic_timing_wakeup_scheduled = false;
  }
}

// This is the main handler, and it will call the appropriate handler
// underneath
void
trig_cpu::event_handler(Event* ev)
{
    // printf("trig_cpu::event_handler()\n");

    if ( static_cast<trig_cpu_event*>(ev)->isTimeout() ) {
	if ( !(static_cast<trig_cpu_event*>(ev)->isCanceled()) ) {
	    // A call to a polling function timed out.  Set timeout
	    // flag and call wake-up
	    timed_out = true;
	    wakeUp();
	}
	delete ev;
	return;
    }

    bool done = false;

    // Need to see if there is any left over work to do

    // One thing that can happen is we block on a PIO to the NIC.  If
    // we do, we need to complete it before we proceed
//     if ( blocking && !waiting ) {
// 	blocking = false;
// 	writeToNIC(blocked_event);
// 	busy += blocked_busy;
//     }
//     else
    if ( pio_in_progress ) {
	// Need to progress the PIO.  A return value of true means
	// it's finally done.
	if ( ptl->progressPIO() ) {
	    pio_in_progress = false;
	    busy+= recv_overhead;
	}
    }
    else {
// 	printf("top_state = %d\n",top_state);
	switch (top_state) {
	case 0:
	    // No noise
	    done = (*app)(ev);
// 	    printf("done = %d\n",done);
	    if (done) {
		top_state = 1;
		barrier_act->barrier();
		return;
	    }
	    break;
	case 1:
 	  // printf("Unregister exit 1\n");
      primaryComponentOKToEndSim();
	    return;
	case 2:
	    // Noise runs, have to do the specified number
	    if ( current_run < noise_runs ) {
		top_state = 3;
	    }
	    else {
		// Done with runs
// 	  printf("Unregister exit 2\n");
		primaryComponentOKToEndSim();
		return;
	    }
	case 3:
	    done = (*app)(ev);
	    if (done) {
		current_run++;
		top_state = 2;
		barrier_act->barrier();
		return;
	    }
	    break;
	    
	}
    }
  
    if (done) {
// 	printf("NOT DONE\n");
	return;
    }
    // Figure out how long to wait until the next time we do
    // something.  We do this by using a combination of busy and the
    // noise parameters.

    // Normally, we will just return after the busy period, but if
    // noise is supposed to start during that time, we will also need
    // to add the noise duration.
    int noise_rem = noise_count;
//     if ( do_noise ) noise_rem -= busy;
    noise_rem -= busy;
    
    if ( noise_rem <= 0 && noise_interval != 0 ) {
        // Noise should start before we return, so add noise to busy and
        // reset the noise_count
        busy += noise_duration;
        noise_count = noise_interval - noise_duration;
        noise_rem = noise_count;
    }
    else if ( waiting || blocking ) {
	// This means nothing interesting happened and won't until we
	// get a new message.  We will simply wait until we have a new
	// message, but we have to keep some extra state around to
	// make sure the noise gets added in correctly.
// 	if (my_id == 1024 && waiting ) printf("%5d: waiting, outstanding recvs = %d\n",my_id,posted_recv_q.size());
	wait_start_time = getCurrentSimTime();
	return;  // Won't wake up until a new message arrives
    }
    else if ( busy == 0 ) {
        // We need to start again next cycle, so set busy to 1
        busy = 1;
    }
//     printf("Busy for %d\n",busy);
    self->send(busy,timing_ev); 
    busy = 0;
    if ( noise_interval != 0 ) noise_count = noise_rem;
    else noise_count = 0;
    return;
}

void
trig_cpu::processEventPortals(Event* event)
{
//     printf("trig_cpu::processEventPortals()\n");
    trig_nic_event* ev = static_cast<trig_nic_event*>(event);
    // For now just add the portals stuff here
    ptl->processMessage(ev);
}

void
trig_cpu::ptlNICHandler(Event* event)
{
// //     printf("trig_cpu::ptlNICHandler()\n");
//     ptl_nic_event* ev = static_cast<ptl_nic_event*>(event);
//     ptl->processNICOp(ev->operation);
    return;
}

void
trig_cpu::wakeUp()
{
//     if ( waiting ) {
        //     if (my_id == 0) printf("Received message while waiting\n");

	waiting = false;
	busy = 0;

	if ( noise_interval == 0 || !do_noise ) {
            self->send(1,timing_ev); 
            return;
        }
        // See if we need to add any noise before we wake up the main
        // "thread"
        SimTime_t elapsed_time = getCurrentSimTime() - wait_start_time;
        if ( elapsed_time < noise_count ) {
            noise_count -= elapsed_time;
            self->send(1,timing_ev); 
        }
        else if ( elapsed_time < (noise_count + noise_duration) ) {
            // This means we are in the middle of noise, figure out how much
            // is left
            SimTime_t noise_left = noise_count + noise_duration - elapsed_time;
            noise_count = noise_interval - noise_duration;
            self->send(noise_left,timing_ev); 
        }
        else if ( elapsed_time < (noise_count + noise_interval) ) {
            // Noise happened, but is done
            noise_count = noise_count + noise_interval - elapsed_time;
            self->send(1,timing_ev); 
        }
        else {
            // Need to determin if we are in noise or not.  Figure out how
            // far we are from the start of a noise interval;
            SimTime_t from_interval_start =
                (elapsed_time - (noise_count + noise_interval)) % noise_interval;
            if ( from_interval_start < noise_duration ) {
                // In noise
                self->send(noise_duration - from_interval_start, NULL); 
                noise_count = noise_interval - noise_duration;
            }
            else {
                // Not in noise
                self->send(1,timing_ev); 
                noise_count = noise_interval - from_interval_start;
            }
        }
//     }
}

void
trig_cpu::event_dma_return(Event *e)
{
  trig_nic_event* ev = static_cast<trig_nic_event*>(e);
  dma_buffers.push(ev);

  // Need to wake up the nic_timing handler if it is not already
  // scheduled to be
  if ( !nic_timing_wakeup_scheduled ) {
    nic_timing_link->send(delay_bus_xfer,NULL); 
    nic_timing_wakeup_scheduled = true;
  }
}


void
trig_cpu::processEvent(Event* event)
{
    trig_nic_event* ev = static_cast<trig_nic_event*>(event);
  
    //   if ( my_id == 0 )
    //       printf("%5d: event received by 0 @ %lu\n",ev->src,getCurrentSimTimeNano());
    // For now, just put the event in a queue that will be processed
    // when recv or wait is called.
    pending_msg.push(ev);

    // If we are waiting, then we need to wake up the main processing
    // code
    if ( waiting ) {
        wakeUp();
    }
}

void
trig_cpu::send(int dest, uint64_t data)
{
    //   if ( my_id == 0 ) printf("send(%d) @ %lu\n",dest,getCurrentSimTimeNano());
    //   printf("%4d: send(%d)\n",my_id,dest);
    trig_nic_event* event = new trig_nic_event;
    event->src = my_id;
    event->dest = dest;

    nic->send(busy,event); 
    // We are busy for msg_rate_delay
    busy += msg_rate_delay;
}

void
trig_cpu::isend(int dest, void* data, int length)
{
    //    if ( my_id == 1024 )printf("%5d: isend(%d, %p, %d)\n",my_id,dest,data,length);
  // Need to create an MD
  ptl_md_t md;
  md.start = data;
  md.length = length;
  md.eq_handle = PTL_EQ_NONE; 
  md.ct_handle = PTL_CT_NONE; 

  // Now send it
  ptl->PtlPut(&md,0,length,PTL_NO_ACK_REQ,dest,0,0,0,NULL,0);
}

bool
trig_cpu::process_pending_msg()
{
  while (!pending_msg.empty()) {
        trig_nic_event* event = pending_msg.front();

	// Need to extract the portals header to get the length
	ptl_header_t header;
	memcpy(&header,event->ptl_data,sizeof(ptl_header_t));
	// Figure out how many packets there are
	int packets;
	packets = (((header.length - 32) + 63) / 64) + 1;
//  	printf("%5d: packets = %d length = %d\n",my_id,packets,header.length);
	// Make sure it's all there, otherwise we won't process yet
	bool all_there = false;
	if ( (int) pending_msg.size() >= packets ) all_there = true;

	if ( !all_there ) {
	  // If it's not all there, we'll act as if the queue is empty
	  return true;
	}
// 	printf("Received a message\n");
        pending_msg.pop();

	// Copy it all into one place
	//	memcpy(pr->buffer,&event->ptl_data[9],copy_length);

	int src = event->src;
	
	// Already have the first packet
	uint8_t* msg = new uint8_t[header.length];
	int copy_length = header.length <= 32 ? header.length : 32;
	memcpy(msg,&event->ptl_data[32],copy_length);
	int rem_length = header.length - copy_length;
	int curr_offset = copy_length;

	delete event;
	for ( int i = 1; i < packets; i++ ) {
	  event = pending_msg.front();
	  pending_msg.pop();
	  copy_length = rem_length <= 64 ? rem_length : 64;
	  memcpy(&msg[curr_offset],event->ptl_data,copy_length);
	  rem_length -= copy_length;
	  curr_offset += copy_length;
	  delete event;
	}

	posted_recv* found_pr = NULL;
        // Search the posted receives queue for a match
        std::list<posted_recv*>::iterator iter;
        bool found = false;
        for ( iter = posted_recv_q.begin(); iter != posted_recv_q.end(); ++iter ) {
            posted_recv* pr = *iter;
            if (pr->src == src) {
                posted_recv_q.erase(iter);
                found_pr = pr;
                busy += recv_overhead;
                found = true;
                break;
            }
        }

        if (found) {
	  // Need to do all the copying and pop all the packets in the
	  // message.
	  // Copy the message to the final buffer
// 	  printf("%5d: copying message to buffer\n",my_id);
	  memcpy(found_pr->buffer,msg,header.length);
	  delete[] msg;
	  delete found_pr;
	  return false;
        }
        else {
            // No match, put in enexpected queue
	  unex_msg_q.push_back(new unex_msg(msg,src,header.length));
        }
    }
    return true;
}


bool
trig_cpu::recv(int src, uint64_t* buf, int& handle)
{
//     bool ret = process_pending_msg();
//     // If we found something in the pending messages that match, we need
//     // to return false to let the sim go busy.
//     if (!ret) return ret;
  
//     // See if the message is in the unexpected queue
//     // Search the posted receives queue for a match
//     std::list<trig_nic_event*>::iterator iter;
//     bool found = false;
//     for ( iter = unex_msg_q.begin(); iter != unex_msg_q.end(); ++iter ) {
//         trig_nic_event* msg = *iter;
//         if (msg->src == src) {
//             unex_msg_q.erase(iter);
//             delete msg;
//             busy += msg_rate_delay;
//             found = true;
//             break;
//         }
//     }
  
//     if (!found) {
//         // Insert a new entry in posted recieves queue
//         posted_recv_q.push_back(new posted_recv(recv_handle,src,buf));
    
//         // Insert an entry into a set so we can more easily implement wait()
//         outstanding_recv.insert(recv_handle);
//     }
//     handle = recv_handle++;
    return true;
  
}

void foobar(void);

bool
trig_cpu::irecv(int src, void* buf, int& handle)
{
    //    if ( my_id == 1024 ) printf("%5d: irecv(%d, %p)\n",my_id,src,buf);
  bool ret = process_pending_msg();
    // If we found something in the pending messages that match, we need
    // to return false to let the sim go busy.
    if (!ret) return ret;

    // See if the message is in the unexpected queue
    // Search the posted receives queue for a match
    std::list<unex_msg*>::iterator iter;
    bool found = false;
    for ( iter = unex_msg_q.begin(); iter != unex_msg_q.end(); ++iter ) {
        unex_msg* msg = *iter;
        if (msg->src == src) {
            unex_msg_q.erase(iter);
            busy += recv_overhead;
            found = true;
// 	    printf("%5d: Found unexpected from %d\n",my_id,src);

	    // Need to copy over the data
	    memcpy(buf,msg->data,msg->length);
	    
	    delete[] msg->data;
	    delete msg;
            break;
        }
    }
  
    if (!found) {
        // Insert a new entry in posted recieves queue
//       printf("%5d: Posting a receive from %d\n",my_id,src);
      posted_recv_q.push_back(new posted_recv(recv_handle,src,buf));
    
        // Insert an entry into a set so we can more easily implement wait()
        outstanding_recv.insert(recv_handle);
    }
    handle = recv_handle++;
    return true;
  
}

bool
trig_cpu::waitall()
{
  if (!process_pending_msg()) return false;
    //   if ( outstanding_msg == 0 ) return true;
    if ( posted_recv_q.size() == 0 ) {
        waiting = false;
        return true;
    }
    //     if (my_id == 0) printf("waiting = true, busy = %d\n",busy);
//     printf("Waiting...\n");
    waiting = true;
    return false;
}

void
trig_cpu::addTimeToStats(SimTime_t time)
{
    barrier_act->addTimeToStats(time);
}

void
trig_cpu::barrier()
{
    barrier_act->barrier();
}
