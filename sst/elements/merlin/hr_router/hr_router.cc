// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
#include <sst_config.h>
#include "sst/core/serialization.h"
#include "hr_router.h"

#include <sst/core/debug.h>
#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>

#include <sstream>

#include <signal.h>

#include "portControl.h"
#include "xbar_arb_rr.h"

using namespace SST::Merlin;

int hr_router::num_routers = 0;
int hr_router::print_debug = 0;

hr_router::~hr_router()
{
    delete [] in_port_busy;
    delete [] out_port_busy;
    delete [] progress_vcs;

    for ( int i = 0 ; i < num_ports ; i++ ) {
        delete ports[i];
    }
    delete [] ports;

    delete topo;
    delete arb;
}

hr_router::hr_router(ComponentId_t cid, Params& params) :
    Router(cid)
{
    // Get the options for the router
    id = params.find_integer("id");
    if ( id == -1 ) {
        _abort(hr_router, "ERROR: hr_router requires id to be specified");
    }
    // std::cout << "id: " << id << std::endl;

    num_ports = params.find_integer("num_ports");
    if ( num_ports == -1 ) {
        _abort(hr_router, "ERROR: hr_router requires num_poorts to be specified");
    }
    // std::cout << "num_ports: " << num_ports << std::endl;

    num_vcs = params.find_integer("num_vcs");
    if ( num_vcs == -1 ) {
        _abort(hr_router, "ERROR: hr_router requires num_vcs to be specified");
    }
    // std::cout << "num_vcs: " << num_vcs << std::endl;

    // Get the topology
    std::string topology = params.find_string("topology");
    // std::cout << "Topology: " << topology << std::endl;

    if ( topology == "" ) {
        _abort(hr_router, "ERROR: hr_router requires topology to be specified");
    }

    topo = dynamic_cast<Topology*>(loadModuleWithComponent(topology,this,params));
    if ( !topo )
        _abort(hr_router, "ERROR:  Unable to find topology '%s'\n", topology.c_str());


    // Parse all the timing parameters
    std::string link_bw = params.find_string("link_bw");
    if ( link_bw == "" ) {
        _abort(hr_router, "ERROR: hr_router requires link_bw to be specified");
        abort();
    }
    // std::cout << "link_bw: " << link_bw << std::endl;

    TimeConverter* link_tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter(link_bw);

    std::string input_latency = params.find_string("input_latency", "0ns");
    // std::cout << "input_latency: " << input_latency << std::endl;

    std::string output_latency = params.find_string("output_latency", "0ns");
    // std::cout << "output_latency: " << output_latency << std::endl;

    xbar_bw = params.find_string("xbar_bw");
    if ( xbar_bw == "" ) {
        _abort(hr_router, "ERROR: hr_router requires xbar_bw to be specified");
    }
    // std::cout << "xbar_bw: " << xbar_bw << std::endl;

    // Create all the PortControl blocks
    ports = new PortControl*[num_ports];


    int input_buf_size = params.find_integer("input_buf_size", 100);
    int output_buf_size = params.find_integer("output_buf_size", 100);


    int in_buf_sizes[num_vcs];
    int out_buf_sizes[num_vcs];

    for ( int i = 0; i < num_vcs; i++ ) {
        in_buf_sizes[i] = input_buf_size;
        out_buf_sizes[i] = output_buf_size;
    }

    // Naming convention is from point of view of the xbar.  So,
    // in_port_busy is >0 if someone is writing to that xbar port and
    // out_port_busy is >0 if that xbar port being read.
    in_port_busy = new int[num_ports];
    out_port_busy = new int[num_ports];

    progress_vcs = new int[num_ports];

    vc_heads = new internal_router_event*[num_ports*num_vcs];
    xbar_in_credits = new int[num_ports*num_vcs];

    topo->setOutputBufferCreditArray(xbar_in_credits);
    
    for ( int i = 0; i < num_ports; i++ ) {
	in_port_busy[i] = 0;
	out_port_busy[i] = 0;
	progress_vcs[i] = -1;
	
	std::stringstream port_name;
	port_name << "port";
	port_name << i;
	// std::cout << port_name.str() << std::endl;

	ports[i] = new PortControl(this, id, port_name.str(), i, link_tc, topo, num_vcs,
				   &vc_heads[i*num_vcs], &xbar_in_credits[i*num_vcs],
				   in_buf_sizes, out_buf_sizes, 1, input_latency, 1, output_latency);

    }

    // Get the Xbar arbitration
    // arb = new xbar_arb_rr(num_ports,num_vcs);
    arb = new xbar_arb_rr();
    arb->setPorts(num_ports,num_vcs);
    
    if ( params.find_integer("debug", 0) ) {
        if ( num_routers == 0 ) {
            signal(SIGUSR2, &hr_router::sigHandler);
        }
	my_clock_handler = new Clock::Handler<hr_router>(this,&hr_router::debug_clock_handler);
    } else {
        my_clock_handler = new Clock::Handler<hr_router>(this,&hr_router::clock_handler);
    }
    xbar_tc = registerClock( xbar_bw, my_clock_handler);
    num_routers++;

#if VERIFY_DECLOCKING
    clocking = true;
#endif
    
    // Check to make sure that the xbar BW is equal to or greater than
    // the link BW, otherwise the model runs into problems
    if ( xbar_tc->getFactor() > link_tc->getFactor() ) {
        std::cout << "ERROR: hr_router requires xbar_bw to be greater than or equal to link_bw" << std::endl;
        std::cout << "  xbar_bw = " << xbar_bw << ", link_bw = " << link_bw << std::endl;
        abort();
    }
}


void
hr_router::notifyEvent()
{
    setRequestNotifyOnEvent(false);

#if VERIFY_DECLOCKING
    clocking = true;
    Cycle_t next_cycle = getNextClockCycle( xbar_tc );
#else
    Cycle_t next_cycle = reregisterClock( xbar_tc, my_clock_handler); 
#endif

    int elapsed_cycles = next_cycle - unclocked_cycle;

#if !VERIFY_DECLOCKING
    // Fix up the busy variables
    for ( int i = 0; i < num_ports; i++ ) {
    	// Should stop at zero, need to find a clean way to do this
    	// with no branch.  For now it should work.
    	in_port_busy[i] -= elapsed_cycles;
    	out_port_busy[i] -= elapsed_cycles;
    	if ( in_port_busy[i] < 0 ) in_port_busy[i] = 0;
    	if ( out_port_busy[i] < 0 ) out_port_busy[i] = 0;
    }
#endif
    // Report skipped cycles to arbitration unit.
    arb->reportSkippedCycles(elapsed_cycles);
}

void
hr_router::sigHandler(int signal)
{
    print_debug = num_routers * 5;
}

void
hr_router::dumpState(std::ostream& stream)
{
    stream << "Router id: " << id << std::endl;
    for ( int i = 0; i < num_ports; i++ ) {
	ports[i]->dumpState(stream);
	stream << "  Output_busy: " << out_port_busy[i] << std::endl;
	stream << "  Input_Busy: " <<  in_port_busy[i] << std::endl;
    }
    
}


bool
hr_router::debug_clock_handler(Cycle_t cycle)
{
    if ( print_debug > 0 ) {
        /* TODO:  PRINT DEBUGGING */
        // Change cycle to a long long unsigned int from a uint64_t (which is a unsigned long long int) to avoid a compile warning
        printf("Debug output for %s at cycle %llu\n", getName().c_str(), (long long unsigned int)cycle);
        dumpState(std::cout);
        print_debug--;
    }

    return clock_handler(cycle);
}

bool
hr_router::clock_handler(Cycle_t cycle)
{
    // If there are no events in the input queues, then we can remove
    // ourselves from the clock queue.
    if ( get_vcs_with_data() == 0 ) {
#if VERIFY_DECLOCKING
	if ( clocking ) {
	    setRequestNotifyOnEvent(true);
	    unclocked_cycle = cycle;
	    clocking = false;
	}
#else
	setRequestNotifyOnEvent(true);
	unclocked_cycle = cycle;
	return true;
#endif
    }

    // Loop through all the events at the heads of the queues and call
    // route
    int index = 0;
    for ( int i = 0; i < num_ports; i++ ) {
	for ( int j = 0; j < num_vcs; j++ ) {
	    if ( vc_heads[index] != NULL ) topo->reroute(i,j,vc_heads[index]);
	    index++;
	}
    }
    
    // All we need to do is arbitrate the crossbar
#if VERIFY_DECLOCKING
    arb->arbitrate(ports,in_port_busy,out_port_busy,progress_vcs,clocking);
#else
    arb->arbitrate(ports,in_port_busy,out_port_busy,progress_vcs);
#endif
    
    // Move the events and decrement the busy values
    for ( int i = 0; i < num_ports; i++ ) {
 	if ( progress_vcs[i] != -1 ) {
	    internal_router_event* ev = ports[i]->recv(progress_vcs[i]);
	    ports[ev->getNextPort()]->send(ev,ev->getVC());
	    // std::cout << "" << id << ": " << "Moving VC " << progress_vcs[i] <<
	    // 	" for port " << i << " to port " << ev->getNextPort() << std::endl;

	    if ( ev->getTraceType() == RtrEvent::FULL ) {
		std::cout << "TRACE(" << ev->getTraceID() << "): " << getCurrentSimTimeNano()
			  << " ns: Copying event (src = " << ev->getSrc() << ","
			  << " dest = "<< ev->getDest() << ") over crossbar in router " << id
			  << " (" << getName() << ")"
			  << " from port " << i << ", VC " << progress_vcs[i] 
			  << " to port " << ev->getNextPort() << ", VC " << ev->getVC()
			  << "." << std::endl;
	    }
	}
	
	// Should stop at zero, need to find a clean way to do this
	// with no branch.  For now it should work.
        if ( in_port_busy[i] != 0 ) in_port_busy[i]--;
	if ( out_port_busy[i] != 0 ) out_port_busy[i]--;
    }

    return false;
}

void hr_router::setup()
{
    // for ( int i = 0; i < num_ports; i++ ) {
    // 	ports[i]->Setup();
    // }
//    return 0;
}

void
hr_router::init(unsigned int phase)
{
    for ( int i = 0; i < num_ports; i++ ) {
	ports[i]->init(phase);
        Event *ev = NULL;
        while ( (ev = ports[i]->recvInitData()) != NULL ) {
            internal_router_event *ire = dynamic_cast<internal_router_event*>(ev);
            if ( ire == NULL ) {
                ire = topo->process_InitData_input(static_cast<RtrEvent*>(ev));
            }
            std::vector<int> outPorts;
            topo->routeInitData(i, ire, outPorts);
            for ( std::vector<int>::iterator j = outPorts.begin() ; j != outPorts.end() ; ++j ) {
                /* Little tricky here.  Need to clone both the event, and the
                 * encapsulated event.
                 */
                switch ( topo->getPortState(*j) ) {
                case Topology::R2N:
                    ports[*j]->sendInitData(ire->getEncapsulatedEvent()->clone());
                    break;
                case Topology::R2R: {
                    internal_router_event *new_ire = ire->clone();
                    new_ire->setEncapsulatedEvent(ire->getEncapsulatedEvent()->clone());
                    ports[*j]->sendInitData(new_ire);
                    break;
                }
                default:
                    break;
                }
            }
            delete ire;
        }
    }
}

void
hr_router::sendTopologyEvent(int port, TopologyEvent* ev) {
    // Event just gets forwarded to appropriate PortControl object
    ports[port]->sendTopologyEvent(ev);
}

void
hr_router::recvTopologyEvent(int port, TopologyEvent* ev) {
    // Event just get's sent on to topolgy object
    topo->recvTopologyEvent(port,ev);
}
