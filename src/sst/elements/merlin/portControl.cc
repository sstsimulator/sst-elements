// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
#include <sst_config.h>

#include "portControl.h"
#include "merlin.h"

#define TRACK 0
#define TRACK_ID 131
#define TRACK_PORT 4

using namespace SST;
using namespace Merlin;
using namespace Interfaces;

void
PortControl::sendTopologyEvent(TopologyEvent* ev)
{
	// If the topology event is zero length (meaning they take no
	// bandwidth), just send immediately
	if ( ev->getSizeInFlits() == 0 ) {
	    port_link->send(1,ev);
	    return;
	}

	// Put event into buffer that will take priority over the VCs
	// for sending.  The event will be sent next time the port is
	// free and it will consume the port for the appropriate time
	// based on number of flits.
	topo_queue.push(ev);
}

void
PortControl::send(internal_router_event* ev, int vc)
{
#if TRACK
    if ( rtr_id == TRACK_ID && port_number == TRACK_PORT ) {
        // std::cout << "send start:" << std::endl;
        printStatus(Simulation::getSimulation()->getSimulationOutput(),0,0);
    }
#endif
	// std::cout << "sending event on port " << port_number << " and VC " << vc << std::endl;
	// if ( xbar_in_credits[vc] < ev->getFlitCount() ) return false;
    
	xbar_in_credits[vc] -= ev->getFlitCount();
    if ( oql_track_port ) {
        int flits = ev->getFlitCount();
        for ( int i = 0; i < num_vcs; ++i ) {
            output_queue_lengths[i] += flits;
        }
    }
    else {
        output_queue_lengths[vc] += ev->getFlitCount();
    }
	ev->setVC(vc);

	output_buf[vc].push(ev);
	if ( waiting ) {
	// if ( waiting && !have_packets ) {
	    // std::cout << "waking up the output" << std::endl;
	    output_timing->send(1,NULL); 
	    waiting = false;
	}
#if TRACK
    if ( rtr_id == TRACK_ID && port_number == TRACK_PORT ) {
        // std::cout << "send end:" << std::endl;
        printStatus(Simulation::getSimulation()->getSimulationOutput(),0,0);
    }
#endif
	// return true;
}

bool
PortControl::spaceToSend(int vc, int flits)
{
	if (xbar_in_credits[vc] < flits) return false;
	return true;
}

internal_router_event*
PortControl::recv(int vc)
{
#if TRACK
    if ( rtr_id == TRACK_ID && port_number == TRACK_PORT ) {
        // std::cout << "recv start:" << std::endl;
        printStatus(Simulation::getSimulation()->getSimulationOutput(),0,0);
    }
#endif
	// if ( input_buf[vc].size() == 0 ) return NULL;
	if ( input_buf[vc].empty() ) return NULL;

	internal_router_event* event = input_buf[vc].front();
	input_buf[vc].pop();

	// Need to update vc_heads
	if ( input_buf[vc].empty() ) {
	    vc_heads[vc] = NULL;
	    parent->dec_vcs_with_data();
	}
	else {
	    vc_heads[vc] = input_buf[vc].front();
	}
	
    int vc_return = topo->isHostPort(port_number) ? event->getCreditReturnVC() : vc;
	// Figure out how many credits to return
	port_ret_credits[vc_return] += event->getFlitCount();

	// For now, we're just going to send the credits back to the
	// other side.  The required BW to do this will not be taken
	// into account.
	port_link->send(1,new credit_event(vc_return,port_ret_credits[vc_return])); 
	port_ret_credits[vc_return] = 0;
    
#if TRACK
    if ( rtr_id == TRACK_ID && port_number == TRACK_PORT ) {
        // std::cout << "recv start:" << std::endl;
        printStatus(Simulation::getSimulation()->getSimulationOutput(),0,0);
    }
#endif
    return event;
}
// time_base is a frequency which represents the bandwidth of the link in flits/second.
// PortControl::PortControl(Router* rif, int rtr_id, std::string link_port_name,
//                          int port_number, TimeConverter* time_base, Topology *topo, 
//                          SimTime_t input_latency_cycles, std::string input_latency_timebase,
//                          SimTime_t output_latency_cycles, std::string output_latency_timebase) :
PortControl::PortControl(Router* rif, int rtr_id, std::string link_port_name,
                         int port_number, const UnitAlgebra& link_bw, const UnitAlgebra& flit_size,
                         Topology *topo, 
                         SimTime_t input_latency_cycles, std::string input_latency_timebase,
                         SimTime_t output_latency_cycles, std::string output_latency_timebase,
                         const UnitAlgebra& in_buf_size, const UnitAlgebra& out_buf_size,
                         std::vector<std::string>& inspector_names,
						 const float dlink_thresh, bool oql_track_port, bool oql_track_remote) :
    rtr_id(rtr_id),
    num_vcs(-1),
    link_bw(link_bw),
    flit_size(flit_size),
    topo(topo),
    port_number(port_number),
    remote_rdy_for_credits(false),
    input_buf(NULL),
    output_buf(NULL),
    input_buf_count(NULL),
    output_buf_count(NULL),
    port_ret_credits(NULL),
    port_out_credits(NULL),
    idle_start(0),
	sai_win_start(0),
	dlink_thresh(dlink_thresh),
	sai_port_disabled(false),
	ongoing_transmit(false),
    oql_track_port(oql_track_port),
    oql_track_remote(oql_track_remote),
    is_idle(true),
	is_active(false),
    waiting(true),
    have_packets(false),
    start_block(0),
    parent(rif),
    output(Simulation::getSimulation()->getSimulationOutput())
{
    // Configure the links.  output_timing will have a temporary time bases.  It will be
    // changed once the final link BW is set.
    switch ( topo->getPortState(port_number) ) {
    case Topology::R2N:
        host_port = true;
        port_link = rif->configureLink(link_port_name, output_latency_timebase,
                                       new Event::Handler<PortControl>(this,&PortControl::handle_input_n2r));
        if ( port_link != NULL ) {
            output_timing = rif->configureSelfLink(link_port_name + "_output_timing", "1GHz",
                                                   new Event::Handler<PortControl>(this,&PortControl::handle_output_n2r));
        }
        break;
    case Topology::R2R:
        host_port = false;
        port_link = rif->configureLink(link_port_name, output_latency_timebase,
                                       new Event::Handler<PortControl>(this,&PortControl::handle_input_r2r));
        if ( port_link != NULL ) {
            output_timing = rif->configureSelfLink(link_port_name + "_output_timing", "1GHz",
                                                   new Event::Handler<PortControl>(this,&PortControl::handle_output_r2r));
        }
        break;
    default:
        host_port = false;
        port_link = NULL;
        break;
    }
    
	// This is the self link to enable the logic for adaptive link widths.
	// The initial call to the handler dynlink_timing->send is made in setup.
	dynlink_timing = rif->configureSelfLink(link_port_name + "_dynlink_timing", "10us",
													new Event::Handler<PortControl>(this,&PortControl::handleSAIWindow));

	disable_timing = rif->configureSelfLink(link_port_name + "_disable_timing", "1us",
													new Event::Handler<PortControl>(this,&PortControl::reenablePort));
    connected = true;

    if ( port_link == NULL ) {
        connected = false;
        return;
    }

    input_buf_size = in_buf_size;
    output_buf_size = out_buf_size;

    if ( port_link && input_latency_timebase != "" ) {
        // std::cout << "Adding extra latency" << std::endl;
        port_link->addRecvLatency(input_latency_cycles,input_latency_timebase);
    }
    
    
    // output_timing = rif->configureSelfLink(link_port_name + "_output_timing", time_base,
    //                                        new Event::Handler<PortControl>(this,&PortControl::handle_output));
    
    curr_out_vc = 0;

    // Register statistics
    std::string port_name("port");
    port_name = port_name + std::to_string(port_number);

    send_bit_count = rif->registerStatistic<uint64_t>("send_bit_count", port_name);
    send_packet_count = rif->registerStatistic<uint64_t>("send_packet_count", port_name);
    output_port_stalls = rif->registerStatistic<uint64_t>("output_port_stalls", port_name);
    idle_time = rif->registerStatistic<uint64_t>("idle_time", port_name);
    width_adj_count = rif->registerStatistic<uint64_t>("width_adj_count", port_name);

	// set the SAI metrics to 0
	stalled = 0;
	active = 0;
	idle = 0;

	// time in seconds
	sai_win_length = 0.00001;
	sai_win_length_nano = sai_win_length * 1000 * 1000 * 1000;
	sai_win_length_pico = sai_win_length_nano * 1000;

	// reasonable values for IB are 4 or 12
	max_link_width = 4;
	cur_link_width = max_link_width;

    // Create any NetworkInspectors
    for ( unsigned int i = 0; i < inspector_names.size(); i++ ) {
        Params empty;
        SimpleNetwork::NetworkInspector* ni = dynamic_cast<SimpleNetwork::NetworkInspector*>(rif->loadSubComponent(inspector_names[i], rif, empty));
        if ( ni == NULL ) {
            merlin_abort.fatal(CALL_INFO,1,"NetworkInspector: %s, not found.\n",inspector_names[i].c_str());
        }
        ni->initialize(port_name);
        network_inspectors.push_back(ni);
    }
}


void
PortControl::initVCs(int vcs, internal_router_event** vc_heads_in, int* xbar_in_credits_in, int* output_queue_lengths_in)
{
    vc_heads = vc_heads_in;
    // If the port is not connected, we still need to initialize
    // vc_heads entries to NULL
    if ( !connected ) {
        for ( int i = 0; i < vcs; i++ ) {
            vc_heads[i] = NULL;
        }
        return;
    }
    num_vcs = vcs;
    xbar_in_credits = xbar_in_credits_in;
    output_queue_lengths = output_queue_lengths_in;
    
    // Input and output buffers
    input_buf = new port_queue_t[vcs];
    output_buf = new port_queue_t[vcs];
    
    input_buf_count = new int[vcs];
    output_buf_count = new int[vcs];
	
    for ( int i = 0; i < num_vcs; i++ ) {
        input_buf_count[i] = 0;
        output_buf_count[i] = 0;
        vc_heads[i] = NULL;
    }
	
    // Initialize credit arrays
    // xbar_in_credits = new int[vcs];
    port_ret_credits = new int[vcs];
    port_out_credits = new int[vcs];
    
    // Figure out how large the buffers are in flits

    // Need to see if we need to convert to bits
    UnitAlgebra ibs = input_buf_size;
    UnitAlgebra obs = output_buf_size;

    if ( !ibs.hasUnits("b") && !ibs.hasUnits("B") ) {
        merlin_abort.fatal(CALL_INFO,-1,"input_buf_size must be specified in either "
                           "bits or bytes: %s\n",ibs.toStringBestSI().c_str());
    }
    
    if ( !obs.hasUnits("b") && !obs.hasUnits("B") ) {
        merlin_abort.fatal(CALL_INFO,-1,"output_buf_size must be specified in either "
                           "bits or bytes: %s\n",obs.toStringBestSI().c_str());
    }

    if ( ibs.hasUnits("B") ) ibs *= UnitAlgebra("8b/B");
    if ( obs.hasUnits("B") ) obs *= UnitAlgebra("8b/B");


    ibs /= flit_size;
    obs /= flit_size;
    
    for ( int i = 0; i < vcs; i++ ) {
        port_ret_credits[i] = ibs.getRoundedValue();
        xbar_in_credits[i] = obs.getRoundedValue();
        port_out_credits[i] = 0;
    }
    
    // // Copy the starting return tokens for the input buffers (this
    // // essentially sets the size of the buffer)
    // memcpy(port_ret_credits,in_buf_size,vcs*sizeof(int));
    // memcpy(xbar_in_credits,out_buf_size,vcs*sizeof(int));
	
    // The output credits are set to zero and the other side of the
    // link will send the number of tokens.
    // for ( int i = 0; i < vcs; i++ ) port_out_credits[i] = 0;


    // If I'm not a host port, I'm going to send out the VC count to
    // my neighbors in case they don't have any host ports to get the
    // request from.
    if ( !host_port && port_link ) {
        RtrInitEvent* init_ev = new RtrInitEvent();
        init_ev->command = RtrInitEvent::SET_VCS;
        init_ev->int_value = vcs;
        port_link->sendInitData(init_ev);
    }
    // Need to start the timer for links that never send data
    idle_start = Simulation::getSimulation()->getCurrentSimCycle();
    is_idle = true;
}

PortControl::~PortControl() {
    if ( input_buf != NULL ) delete [] input_buf;
    if ( output_buf != NULL ) delete [] output_buf;
    if ( input_buf_count != NULL ) delete [] input_buf_count;
    if ( output_buf_count != NULL ) delete [] output_buf_count;
    //if ( xbar_in_credits != NULL ) delete [] xbar_in_credits;
    if ( port_ret_credits != NULL ) delete [] port_ret_credits;
    if ( port_out_credits != NULL ) delete [] port_out_credits;
    for ( unsigned int i = 0; i < network_inspectors.size(); i++ ) {
        delete network_inspectors[i];
    }
}

void
PortControl::setup() {
	
	if (dlink_thresh >= 0) dynlink_timing->send(1,NULL);
    while ( init_events.size() ) {
        delete init_events.front();
        init_events.pop_front();
    }
}

void
PortControl::finish() {

	//std::cerr << "Link was adjusted " << width_adj_count->getCollectionCount() << " times.\n";
    // Any links that ended in an idle state need to add stats
    if (is_idle && connected) {
        idle_time->addData(Simulation::getSimulation()->getCurrentSimCycle() - idle_start);
        is_idle = false;
    }

    // Clean up all the events left in the queues.  This will help
    // track down real memory leaks as all this events won't be in the
    // way.
    for ( int i = 0; i < num_vcs; i++ ) {
        while ( !input_buf[i].empty() ) {
            delete input_buf[i].front();
            input_buf[i].pop();
        }
        while ( !output_buf[i].empty() ) {
            delete output_buf[i].front();
            output_buf[i].pop();
        }
    }

    // finish any inspectors
    for ( unsigned int i = 0; i < network_inspectors.size(); i++ ) {
        network_inspectors[i]->finish();
    }
}


void
PortControl::init(unsigned int phase) {
    // if ( topo->getPortState(port_number) == Topology::UNCONNECTED ) return;
    if ( !connected ) return;
    Event *ev;
    RtrInitEvent* init_ev;

	//if (link_bw.hasUnits("GB"))
	//	printf("Has units GBytes\n");
	//if (link_bw.hasUnits("B"))
	//	printf("Has units Bytes\n");
	//if (link_bw.hasUnits("B/s"))
	//	printf("Has units Bytes per second\n");
	//if (link_bw.hasUnits("b"))
	//	printf("Has units Bits\n");
	//if (link_bw.hasUnits("b/s"))
	//	printf("Has units Bits per second\n");
	//else
	//	printf("Bandwidth is: %s\n", link_bw.toString().c_str());
    switch ( phase ) {
    case 0:
        // Negotiate link speed.  We will take the min of the two link speeds
        init_ev = new RtrInitEvent();
        init_ev->command = RtrInitEvent::REPORT_BW;
        init_ev->ua_value = link_bw;
        port_link->sendInitData(init_ev);
        
        // std::cout << "FLIT - isHostPort(" << port_number << "): " << topo->isHostPort(port_number) << std::endl;
        // If this is a host port, send the endpoint ID to the LinkControl
        if ( topo->isHostPort(port_number) ) {
            init_ev = new RtrInitEvent();
            init_ev->command = RtrInitEvent::REPORT_FLIT_SIZE;
            init_ev->ua_value = flit_size;
            port_link->sendInitData(init_ev);
            // std::cout << "FLIT_SIZE: " << flit_size.toStringBestSI() << std::endl;
        
            RtrInitEvent* ev = new RtrInitEvent();
            ev->command = RtrInitEvent::REPORT_ID;
            ev->int_value = topo->getEndpointID(port_number);
            // ev->print("PC: ", Simulation::getSimulation()->getSimulationOutput());
            port_link->sendInitData(ev);
        }
        else {
            // Report router ID and port number to other side of link
            init_ev = new RtrInitEvent();
            init_ev->command = RtrInitEvent::REPORT_ID;
            init_ev->int_value = rtr_id;
            port_link->sendInitData(init_ev);
            
            init_ev = new RtrInitEvent();
            init_ev->command = RtrInitEvent::REPORT_PORT;
            init_ev->int_value = port_number;
            port_link->sendInitData(init_ev);
        }
        break;
    case 1:
        {
        // Get the link speed from the other side.  Actual link speed
        // will be the minumum the two sides
        ev = port_link->recvInitData();
        init_ev = dynamic_cast<RtrInitEvent*>(ev);
        if ( link_bw > init_ev->ua_value ) link_bw = init_ev->ua_value;

        // Initialize links (or rather, reset the TimeBase to get the
        // right BW).
        UnitAlgebra link_clock = link_bw / flit_size;
        // std::cout << link_clock.toStringBestSI() << std::endl;
        TimeConverter* tc = parent->getTimeConverter(link_clock);
        output_timing->setDefaultTimeBase(tc);
        delete ev;
        
        // Get initialization event from endpoint, but only if I am a host port
        if ( topo->isHostPort(port_number) ) {
            ev = port_link->recvInitData();
            init_ev = dynamic_cast<RtrInitEvent*>(ev);
            // Need to notify the router about the number of VNs requested
            parent->reportRequestedVNs(port_number,init_ev->int_value);
            remote_rdy_for_credits = true;
            delete ev;

            // Set remote_rtr_id and remote_port_number to -1
            // indicating this is a host port
            remote_rtr_id = -1;
            remote_port_number = -1;
        } else {
            // If not a host port, the other side sent us their rtr_id
            // and port_number
            ev = port_link->recvInitData();
            init_ev = dynamic_cast<RtrInitEvent*>(ev);
            remote_rtr_id = init_ev->int_value;
            delete init_ev;

            ev = port_link->recvInitData();
            init_ev = dynamic_cast<RtrInitEvent*>(ev);
            remote_port_number = init_ev->int_value;
            delete init_ev;
        }
        }
        break;
    // case 2:
    //     // If I'm a host port, I can send my credit events now.
    //     // VCs should now be initialized, send over the credit events
    //     if ( host_port ) {
    //         for ( int i = 0; i < num_vcs; i++ ) {
    //             port_link->sendInitData(new credit_event(i,port_ret_credits[i]));
    //             port_ret_credits[i] = 0;
    //         }
    //     }
        // break;

    default:
        // If my VCs have been initialized and the remote side is
        // ready to receive credits, send the credit events.
        if ( num_vcs != -1 && remote_rdy_for_credits ) {
            // std::cout << "Sending credit events (port = " << port_number << "), num_vcs = " << num_vcs << std::endl;
            for ( int i = 0; i < num_vcs; i++ ) {
                port_link->sendInitData(new credit_event(i,port_ret_credits[i]));
                port_ret_credits[i] = 0;
            }
            remote_rdy_for_credits = false;
        }
        
        // Need to recv the credits send from the other side
        while ( ( ev = port_link->recvInitData() ) != NULL ) {
            credit_event* ce = dynamic_cast<credit_event*>(ev);
            if ( ce != NULL ) {
                if ( ce->vc >= num_vcs ) {
                    // _abort(PortControl, "Received Credit Event for VC %d.  I only know of VCS[0-%d]\n", ce->vc, num_vcs-1);
                }
                // std::cout << "Received credit_event (port = " << port_number << ")" << std::endl;
                port_out_credits[ce->vc] += ce->credits;
                delete ev;
            }
            else {
                RtrInitEvent* init_ev = dynamic_cast<RtrInitEvent*>(ev);
                if ( init_ev != NULL ) {
                    // std::cout << "Received RtrInitEvent (port = " << port_number << ")" << std::endl;
                    remote_rdy_for_credits = true;
                    if ( num_vcs == -1 ) {
                        // I have not yet been intiialized, so report VCs to router
                        parent->reportSetVCs(port_number,init_ev->int_value);
                    }
                    delete init_ev;
                }
                else {
                    init_events.push_back(ev);
                }
            }
        }
        break;
    }   
    // std::cout << "End PortControl::init" << std::endl;
}

void
PortControl::complete(unsigned int phase) {
    if ( !connected ) return;
    Event *ev;

    // Need to get all the init events
    while ( ( ev = port_link->recvInitData() ) != NULL ) {
        init_events.push_back(ev);
    }
}

void
PortControl::sendInitData(Event *ev)
{
    sendUntimedData(ev);
}

Event*
PortControl::recvInitData()
{
    return recvUntimedData();
}

void
PortControl::sendUntimedData(Event *ev)
{
    if ( connected ) {
        port_link->sendInitData(ev);
    }
}

Event*
PortControl::recvUntimedData()
{
    if ( connected && init_events.size() ) {
        Event *ev = init_events.front();
        init_events.pop_front();
        return ev;
    }
    else {
        return NULL;
    }
}

void
PortControl::dumpState(std::ostream& stream)
{
	stream << "Router id: " << rtr_id << ", port " << port_number << ":" << std::endl;
	stream << "  Waiting = " << waiting << std::endl;
    if (is_idle)
        stream << "Time since last active = " << (Simulation::getSimulation()->getCurrentSimCycle() - idle_start) << std::endl;
    stream << "  have_packets = " << have_packets << std::endl;
    stream << "  start_block = " << start_block << std::endl;
	stream << "  curr_out_vc = " << curr_out_vc << std::endl;
	for ( int i = 0; i < num_vcs; i++ ) {
	    stream << "  VC " << i << " Information:" << std::endl;
	    stream << "    xbar_in_credits = " << xbar_in_credits[i] << std::endl;
	    stream << "    port_out_credits = " << port_out_credits[i] << std::endl;
	    stream << "    port_ret_credits = " << port_ret_credits[i] << std::endl;
	    stream << "    Input Buffer:" << std::endl;
	    dumpQueueState(input_buf[i],stream);
	    stream << "    Output Buffer:" << std::endl;
	    dumpQueueState(output_buf[i],stream);
	}
}

void
PortControl::printStatus(Output& out, int out_port_busy, int in_port_busy)
{
	out.output("Start Port %d\n",port_number);
	out.output("  Remote id = %d\n", remote_rtr_id);
	out.output("  Remote port = %d\n", remote_port_number);
    out.output("  Output_busy = %d\n", out_port_busy);
    out.output("  Input_Busy = %d\n",in_port_busy);
	out.output("  Waiting = %u\n", waiting);
    out.output("  have_packets = %u\n", have_packets);
    out.output("  start_block = %" PRIu64 "\n", start_block);
    out.output("  curr_out_vc = %d\n", curr_out_vc);
	for ( int i = 0; i < num_vcs; i++ ) {
	    out.output("  Start VC %d\n", i);
	    out.output("    xbar_in_credits = %d\n", xbar_in_credits[i]);
	    out.output("    port_out_credits = %d\n", port_out_credits[i]);
	    out.output("    port_ret_credits = %d\n", port_ret_credits[i]);
	    out.output("    Start Input Buffer\n");
	    dumpQueueState(input_buf[i],out);
	    out.output("    End Input Buffer\n");
	    out.output("    Start Output Buffer\n");
	    dumpQueueState(output_buf[i],out);
	    out.output("    End Output Buffer\n");
        out.output("  End VC %d\n", i);
    }
    out.output("End Port %d\n", port_number);
}

void
PortControl::dumpQueueState(port_queue_t& q, std::ostream& stream) {
	int size = q.size();
	for ( int i = 0; i < size; i++ ) {
	    internal_router_event* ev = q.front();
	    stream << "      dest = " << ev->getDest()
               << ", size = " << ev->getFlitCount()
               << ", vc = " << ev->getVC()
               << ", next_port = " << ev->getNextPort()
               << std::endl;
	    q.pop();
	    q.push(ev);
	}
}
    
void
PortControl::dumpQueueState(port_queue_t& q, Output& out) {
	int size = q.size();
	for ( int i = 0; i < size; i++ ) {
	    internal_router_event* ev = q.front();
        out.output("      dest = %d, size = %d, vc = %d, next_port = %d\n",
                   ev->getDest(), ev->getFlitCount(), ev->getVC(), ev->getNextPort());
	    q.pop();
	    q.push(ev);
	}
}
    
void
PortControl::handle_input_n2r(Event* ev)
{
    
	// Check to see if this is a credit or data packet
	// credit_event* ce = dynamic_cast<credit_event*>(ev);
	// if ( ce != NULL ) {
	BaseRtrEvent* base_event = static_cast<BaseRtrEvent*>(ev);
    
	switch (base_event->getType()) {
	case BaseRtrEvent::CREDIT:
    {
	    credit_event* ce = static_cast<credit_event*>(ev);
	    port_out_credits[ce->vc] += ce->credits;

        if ( oql_track_remote ) {
            if ( oql_track_port ) {
                for ( int i = 0; i < num_vcs; ++i ) {
                    output_queue_lengths[i] -= ce->credits;
                }
            }
            else {
                output_queue_lengths[ce->vc] -= ce->credits;
            }
        }

        delete ce;
        
	    // If we're waiting, we need to send a wakeup event to the
	    // output queues
	    if ( waiting ) {
            // std::cout << output_timing << std::endl;
            output_timing->send(1,NULL); 
            waiting = false;
            // If we were stalled waiting for credits and we had
            // packets, we need to add stall time
            if ( have_packets) {
                output_port_stalls->addData(Simulation::getSimulation()->getCurrentSimCycle() - start_block);
            }
	    }
	}
    break;
	case BaseRtrEvent::PACKET:
	{
	    RtrEvent* event = static_cast<RtrEvent*>(ev);
	    // Simply put the event into the right virtual network queue
        
	    // Need to process input and do the routing
        int vn = event->request->vn;
        internal_router_event* rtr_event = topo->process_input(event);
        rtr_event->setCreditReturnVC(vn);
        int curr_vc = rtr_event->getVC();
	    topo->route(port_number, rtr_event->getVC(), rtr_event);
	    input_buf[curr_vc].push(rtr_event);
	    input_buf_count[curr_vc]++;
        
	    // If this becomes vc_head we need to put it into the vc_heads array
	    if ( vc_heads[curr_vc] == NULL ) {
            vc_heads[curr_vc] = rtr_event;
            parent->inc_vcs_with_data();
	    }
	    
	    if ( event->request->getTraceType() != SST::Interfaces::SimpleNetwork::Request::NONE ) {
            output.output("TRACE(%d): %" PRIu64 " ns: Received an event on port %d in router %d"
                          " (%s) on VC %d from src %" PRIu64 " to dest %" PRIu64 ".\n",
                          event->getTraceID(),
                          parent->getCurrentSimTimeNano(),
                          port_number,
                          rtr_id,
                          parent->getName().c_str(),
                          curr_vc,
                          event->request->src,
                          event->request->dest);
                          
            // std::cout << "TRACE(" << event->getTraceID() << "): " << parent->getCurrentSimTimeNano()
            //           << " ns: Received an event on port " << port_number
            //           << " in router " << rtr_id << " ("
            //           << parent->getName() << ") on VC " << curr_vc << " from src " << event->request->src
            //           << " to dest " << event->request->dest << "." << std::endl;
	    }
        
	    if ( parent->getRequestNotifyOnEvent() ) parent->notifyEvent();
	}
    break;
	case BaseRtrEvent::INTERNAL:
	    // Should never get here
	    break;
	case BaseRtrEvent::TOPOLOGY:
	    // This should never happen (for now)
	    break;
	default:
	    break;
	}
}
    
void
PortControl::handle_input_r2r(Event* ev)
{
#if TRACK
    if ( rtr_id == TRACK_ID && port_number == TRACK_PORT ) {
        // std::cout << "handle_input_r2r start:" << std::endl;
        ev->print("  ", Simulation::getSimulation()->getSimulationOutput());
        printStatus(Simulation::getSimulation()->getSimulationOutput(),0,0);
    }
#endif
	// Check to see if this is a credit or data packet
	// credit_event* ce = dynamic_cast<credit_event*>(ev);
	// if ( ce != NULL ) {
	BaseRtrEvent* base_event = static_cast<BaseRtrEvent*>(ev);
    
	switch (base_event->getType()) {
	case BaseRtrEvent::CREDIT:
	{
	    credit_event* ce = static_cast<credit_event*>(ev);
	    port_out_credits[ce->vc] += ce->credits;
	    delete ce;
        
	    // If we're waiting, we need to send a wakeup event to the
	    // output queues
	    if ( waiting ) {
            output_timing->send(1,NULL); 
            waiting = false;
            // If we were stalled waiting for credits and we had
            // packets, we need to add stall time
            if ( have_packets) {
                output_port_stalls->addData(Simulation::getSimulation()->getCurrentSimCycle() - start_block);
            }
            // I don't think this should be idle, but I need to think it over more carefully - TLG
            if (idle_start) {
                idle_time->addData(Simulation::getSimulation()->getCurrentSimCycle() - idle_start);
                idle_start = 0;
            }
	    }
	}
    break;
	case BaseRtrEvent::PACKET:
	    // This shouldn't happen
	    break;
	case BaseRtrEvent::INTERNAL:
    {
	    internal_router_event* event = static_cast<internal_router_event*>(ev);
	    // Simply put the event into the right virtual network queue
        
	    // Need to do the routing
	    int curr_vc = event->getVC();
	    topo->route(port_number, event->getVC(), event);
	    input_buf[curr_vc].push(event);
	    input_buf_count[curr_vc]++;
        
	    // If this becomes vc_head (there isn't an event already
	    // in the array) we need to put it into the vc_heads array
	    if ( vc_heads[curr_vc] == NULL ) {
            vc_heads[curr_vc] = event;
            parent->inc_vcs_with_data();
	    }
        // std::cout << "Got to here 3" << std::endl; 
	    
	    if ( event->getTraceType() != SimpleNetwork::Request::NONE ) {
            output.output("TRACE(%d): %" PRIu64 " ns: Received an event on port %d in router %d"
                          " (%s) on VC %d from src %d to dest %d.\n",
                          event->getTraceID(),
                          parent->getCurrentSimTimeNano(),
                          port_number,
                          rtr_id,
                          parent->getName().c_str(),
                          curr_vc,
                          event->getSrc(),
                          event->getDest());

                          
            // std::cout << "TRACE(" << event->getTraceID() << "): " << parent->getCurrentSimTimeNano()
            //           << " ns: Received an event on port " << port_number
            //           << " in router " << rtr_id << " ("
            //           << parent->getName() << ") on VC " << curr_vc << " from src " << event->getSrc()
            //           << " to dest " << event->getDest() << "." << std::endl;
	    }
        
	    if ( parent->getRequestNotifyOnEvent() ) parent->notifyEvent();
	}
    break;
	case BaseRtrEvent::TOPOLOGY:
	    parent->recvTopologyEvent(port_number,static_cast<TopologyEvent*>(ev));
	    break;
	default:
	    break;
	}
#if TRACK
    if ( rtr_id == TRACK_ID && port_number == TRACK_PORT ) {
        printStatus(Simulation::getSimulation()->getSimulationOutput(),0,0);
    }
#endif
}

void
PortControl::handle_output_r2r(Event* ev) {
    // TraceFunction trace (CALL_INFO_LONG);
#if TRACK
    if ( rtr_id == TRACK_ID && port_number == TRACK_PORT ) {
        printStatus(Simulation::getSimulation()->getSimulationOutput(),0,0);
    }
#endif
	// The event is an empty event used just for timing.

	// ***** Need to add in logic for when to return credits *****
	// For now just done automatically when events are pulled out
	// of the block
    
	// If there is data in the topo_queue, it takes priority
	if ( !topo_queue.empty() ) {
	    TopologyEvent* event = topo_queue.front();
	    // Send an event to wake up again after packet is done
	    output_timing->send(event->getSizeInFlits(),NULL);
        
	    // Send event
	    port_link->send(1,event);
        // trace.getOutput().output(CALL_INFO, "after\n");
	    return;
	}
    // trace.getOutput().output(CALL_INFO, "Got to here 1\n");
    
	// We do a round robin scheduling.  If the current vc has no
	// data, find one that does.
	int vc_to_send = -1;
	bool found = false;
	internal_router_event* send_event = NULL;
    have_packets = false;
    // trace.getOutput().output(CALL_INFO, "Got to here 2\n");
    
	if (!sai_port_disabled){
		for ( int i = curr_out_vc; i < num_vcs; i++ ) {
			if ( output_buf[i].empty() ) continue;
			have_packets = true;
			send_event = output_buf[i].front();
			// Check to see if the needed VC has enough space
			if ( port_out_credits[i] < send_event->getFlitCount() ) continue;
			vc_to_send = i;
			output_buf[i].pop();
			found = true;
			break;
		}
		// trace.getOutput().output(CALL_INFO, "Got to here 3\n");
			
		if (!found)  {
			for ( int i = 0; i < curr_out_vc; i++ ) {
				if ( output_buf[i].empty() ) continue;
				have_packets = true;
				send_event = output_buf[i].front();
				// Check to see if the needed VC has enough space
				if ( port_out_credits[i] < send_event->getFlitCount() ) continue;
				vc_to_send = i;
				output_buf[i].pop();
				found = true;
				break;
			}
		}
	}
    // trace.getOutput().output(CALL_INFO, "Got to here 4\n");
	
	// If we found an event to send, go ahead and send it
	if ( found ) {
	    // std::cout << "Found an event to send on output port " << port_number << std::endl;
	    // Send the output to the network.
	    // First set the virtual channel.

        // If this is a host port, then we return it to the VN instead of the VC
        send_event->setVC(vc_to_send);
        
	    // Need to return credits to the output buffer
	    int size = send_event->getFlitCount();
	    xbar_in_credits[vc_to_send] += size;
        if ( !oql_track_remote ) {
            if ( oql_track_port ) {
                for ( int i = 0; i < num_vcs; ++i ) {
                    output_queue_lengths[i] -= size;
                }
            }
            else {
                output_queue_lengths[vc_to_send] -= size;
            }
        }
        
	    // Send an event to wake up again after this packet is sent.
	    output_timing->send(size,NULL); 
	    
	    // Take care of the round variable
	    curr_out_vc = vc_to_send + 1;
	    if ( curr_out_vc == num_vcs ) curr_out_vc = 0;
        
	    // Subtract credits
	    port_out_credits[vc_to_send] -= size;
	    output_buf_count[vc_to_send]++;

        if (is_idle) {
            idle_time->addData(Simulation::getSimulation()->getCurrentSimCycle() - idle_start);
            is_idle = false;
        }
        
	    if ( send_event->getTraceType() == SimpleNetwork::Request::FULL ) {
            output.output("TRACE(%d): %" PRIu64 " ns: Sent and event to router from PortControl in router: %d"
                          " (%s) on VC %d from src %d to dest %d.\n",
                          send_event->getTraceID(),
                          parent->getCurrentSimTimeNano(),
                          rtr_id,
                          parent->getName().c_str(),
                          send_event->getVC(),
                          send_event->getSrc(),
                          send_event->getDest());

            // std::cout << "TRACE(" << send_event->getTraceID() << "): " << parent->getCurrentSimTimeNano()
            //           << " ns: Sent an event to router from PortControl in router: " << rtr_id
            //           << " (" << parent->getName() << ") on VC " << send_event->getVC()
            //           << " from src " << send_event->getSrc()
            //           << " to dest " << send_event->getDest()
            //           << "." << std::endl;
	    }
        send_bit_count->addData(send_event->getEncapsulatedEvent()->request->size_in_bits);
        send_packet_count->addData(1);

        // Send the request to all the registered NetworkInspectors
        for ( unsigned int i = 0; i < network_inspectors.size(); i++ ) {
            network_inspectors[i]->inspectNetworkData(send_event->getEncapsulatedEvent()->request);
        }

	    if ( host_port ) {
            // std::cout << "Found an event to send on host port " << port_number << std::endl;
            // trace.getOutput().output(CALL_INFO, "before\n");
            port_link->send(1,send_event->getEncapsulatedEvent()); 
            // trace.getOutput().output(CALL_INFO, "after\n");
            send_event->setEncapsulatedEvent(NULL);
            delete send_event;
	    }
	    else {
            // trace.getOutput().output(CALL_INFO, "before\n");
            port_link->send(1,send_event); 
            // trace.getOutput().output(CALL_INFO, "after\n");
	    }
	}
	else {
	    // What do we do if there's nothing to send??  It could be
	    // because everything is empty or because there's not
	    // enough room in the router buffers.  Either way, we
	    // don't send a wakeup event.  We will send a wake up when
	    // we either get something new in the output buffers or
	    // receive credits back from the router.  However, we need
	    // to know that we got to this state.
        start_block = Simulation::getSimulation()->getCurrentSimCycle();
	    waiting = true;
        // Begin counting the amount of time this port was idle
        if (!have_packets && !is_idle) {
            idle_start = Simulation::getSimulation()->getCurrentSimCycle();
            is_idle = true;
        }
		// Should be in a stalled state rather than idle
		// This should also be triggered when a link is temporarily disabled due to
		// adjusting the link width
		if (have_packets && is_idle){
            idle_time->addData(Simulation::getSimulation()->getCurrentSimCycle() - idle_start);
            is_idle = false;
        }
		if (sai_port_disabled){
			output_timing->send(1,NULL); 
		}
	}
#if TRACK
    if ( rtr_id == TRACK_ID && port_number == TRACK_PORT ) {
        printStatus(Simulation::getSimulation()->getSimulationOutput(),0,0);
    }
#endif
}

void
PortControl::handle_output_n2r(Event* ev) {
    // TraceFunction trace(CALL_INFO_LONG);
	// The event is an empty event used just for timing.

	// ***** Need to add in logic for when to return credits *****
	// For now just done automatically when events are pulled out
	// of the block
    
	// If there is data in the topo_queue, it takes priority
	if ( !topo_queue.empty() ) {
	    TopologyEvent* event = topo_queue.front();
	    // Send an event to wake up again after packet is done
	    output_timing->send(event->getSizeInFlits(),NULL);
        
	    // Send event
	    port_link->send(1,event);
	    return;
	}
	
	// We do a round robin scheduling.  If the current vc has no
	// data, find one that does.
	int vc_to_send = -1;
	bool found = false;
	internal_router_event* send_event = NULL;
    have_packets = false;
   	
	if (!sai_port_disabled){
		for ( int i = curr_out_vc; i < num_vcs; i++ ) {
			if ( output_buf[i].empty() ) continue;
			have_packets = true;
			send_event = output_buf[i].front();
			// Check to see if the needed VC has enough space
			if ( port_out_credits[send_event->getVN()] < send_event->getFlitCount() ) continue;
			vc_to_send = i;
			output_buf[i].pop();
			found = true;
			break;
		}
			
		if (!found)  {
			for ( int i = 0; i < curr_out_vc; i++ ) {
				if ( output_buf[i].empty() ) continue;
				have_packets = true;
				send_event = output_buf[i].front();
				// Check to see if the needed VC has enough space
				if ( port_out_credits[send_event->getVN()] < send_event->getFlitCount() ) continue;
				vc_to_send = i;
				output_buf[i].pop();
				found = true;
				break;
			}
		}
	}
	
	// If we found an event to send, go ahead and send it
	if ( found ) {
	    // std::cout << "Found an event to send on output port " << port_number << std::endl;
	    // Send the output to the network.
	    // First set the virtual channel.

        // If this is a host port, then we return it to the VN instead of the VC
        // send_event->setVC(vc_to_send);
        
	    // Need to return credits to the output buffer
	    int size = send_event->getFlitCount();
	    xbar_in_credits[vc_to_send] += size;
        if ( !oql_track_remote ) {
            if ( oql_track_port ) {
                for ( int i = 0; i < num_vcs; ++i ) {
                    output_queue_lengths[i] -= size;
                }
            }
            else {
                output_queue_lengths[vc_to_send] -= size;
            }
        }
        
	    // Send an event to wake up again after this packet is sent.
	    output_timing->send(size,NULL); 
	    
	    // Take care of the round variable
	    curr_out_vc = vc_to_send + 1;
	    if ( curr_out_vc == num_vcs ) curr_out_vc = 0;
        
	    // Subtract credits
	    // port_out_credits[vc_to_send] -= size;
	    port_out_credits[send_event->getVN()] -= size;
	    output_buf_count[vc_to_send]++;

        if (is_idle) {
            idle_time->addData(Simulation::getSimulation()->getCurrentSimCycle() - idle_start);
            is_idle = false;
        }
        
	    if ( send_event->getTraceType() == SimpleNetwork::Request::FULL ) {
            output.output("TRACE(%d): %" PRIu64 " ns: Sent and event to router from PortControl in router: %d"
                          " (%s) on VC %d from src %d to dest %d.\n",
                          send_event->getTraceID(),
                          parent->getCurrentSimTimeNano(),
                          rtr_id,
                          parent->getName().c_str(),
                          send_event->getVC(),
                          send_event->getSrc(),
                          send_event->getDest());

            // std::cout << "TRACE(" << send_event->getTraceID() << "): " << parent->getCurrentSimTimeNano()
            //           << " ns: Sent an event to router from PortControl in router: " << rtr_id
            //           << " (" << parent->getName() << ") on VC " << send_event->getVC()
            //           << " from src " << send_event->getSrc()
            //           << " to dest " << send_event->getDest()
            //           << "." << std::endl;
	    }
        send_bit_count->addData(send_event->getEncapsulatedEvent()->request->size_in_bits);
        send_packet_count->addData(1);
	    if ( host_port ) {
            // std::cout << "Found an event to send on host port " << port_number << std::endl;
            port_link->send(1,send_event->getEncapsulatedEvent()); 
            send_event->setEncapsulatedEvent(NULL);
            delete send_event;
	    }
	    else {
            port_link->send(1,send_event); 
	    }   
	}
	// TLG -- need to think about how to count a disabled link, is it stalled?
	else {
	    // What do we do if there's nothing to send??  It could be
	    // because everything is empty or because there's not
	    // enough room in the router buffers.  Either way, we
	    // don't send a wakeup event.  We will send a wake up when
	    // we either get something new in the output buffers or
	    // receive credits back from the router.  However, we need
	    // to know that we got to this state.
        start_block = Simulation::getSimulation()->getCurrentSimCycle();
	    waiting = true;
        // Begin counting the amount of time this port was idle
        if (!have_packets && !is_idle) {
            idle_start = Simulation::getSimulation()->getCurrentSimCycle();
            is_idle = true;
        }
		// Should be in a stalled state rather than idle
		// This should also be triggered when a link is temporarily disabled due to
		// adjusting the link width
		if (have_packets && is_idle){
            idle_time->addData(Simulation::getSimulation()->getCurrentSimCycle() - idle_start);
            is_idle = false;
        }
		if (sai_port_disabled){
			output_timing->send(1,NULL); 
		}
	}
}

// This is the handler for an event that disables a port
// from sending or receiving data for sai_adj_delay time 
void
PortControl::reenablePort(Event* ev) {
	sai_port_disabled = false;
}

// Triggered every window duration of time
// This resets SAI metrics and calls increase/decreaseLinkWidth
void 
PortControl::handleSAIWindow(Event* ev) {
	
	SimTime_t cur_time = Simulation::getSimulation()->getCurrentSimCycle();
	// If we are in the middle of an active state.
	
	// If we are in the middle of an idle state.
	if (is_idle){
		if (idle_start < sai_win_start){
			idle = 1;
		}
		else {
			// This is idle time for this interval (picosecs),
			idle = (cur_time - idle_start)/(double)sai_win_length_pico;
		}
	}
	
	// There should be a flag based off an input parameter enabling disabling dynamic link width.
	// Here's the logic for adjusting link width based on idle time.
	if (idle > dlink_thresh){
		decreaseLinkWidth();
	}
	else if (cur_link_width < max_link_width){
		increaseLinkWidth();
	}
	
	// DEBUG
	//if (active > 1 || idle > 1 || stalled > 1){
   	//	std::cerr << "Error SAI greater than 1: " << stalled << ", " 
	//			  << active << ", "
	//			  << idle << "\n";
	//}

	active = 0; 
	stalled = 0; 
	idle = 0;

	dynlink_timing->send(1,NULL);
	sai_win_start = cur_time;
}

// If we are idle or stalled beyond some threshold,
// we want to reduce the bandwidth of the outgoing traffic.
// Each port monitors the amount of outgoing traffic.
// Since links are bidirectional we can assume that we can configure output ports independently.
// This should translate into potential power savings, 
// which a power constrained system can take advantage of.
// 
// TODO add delay on port, before it can transmit data again. 
bool
PortControl::decreaseLinkWidth() {
// We don't want to reduce the link width below 1
if ( cur_link_width == max_link_width )
{
	cur_link_width = cur_link_width/2;
	link_bw = link_bw/2;
	UnitAlgebra link_clock = link_bw / flit_size;
	TimeConverter* tc = parent->getTimeConverter(link_clock);
	output_timing->setDefaultTimeBase(tc);
	width_adj_count->addData(1);
	// I need to add a delay before messages can transmit on the link
	disable_timing->send(1,NULL);
	sai_port_disabled = true;
	return true;
}
else return false;

}

// If we are active beyond some threshold,
// we want to increase the bandwidth of the outgoing traffic.
// Each port monitors the amount of outgoing traffic.
// Since links are bidirectional we can assume that we can configure output ports independently.
// This should translate into potential performance savings.
//
// TODO add delay on port, before it can transmit data again. 
bool
PortControl::increaseLinkWidth() {
// We don't want to increase the link width above the max_link_width
if ( cur_link_width < max_link_width )
{
	cur_link_width = max_link_width;
	link_bw = link_bw*2;
	UnitAlgebra link_clock = link_bw / flit_size;
	TimeConverter* tc = parent->getTimeConverter(link_clock);
	output_timing->setDefaultTimeBase(tc);
	width_adj_count->addData(1);
	// I need to add a delay before messages can transmit on the link
	disable_timing->send(1,NULL);
	sai_port_disabled = true;
	return true;
}

else return false;

}
