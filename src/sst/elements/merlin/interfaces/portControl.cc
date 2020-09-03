// Copyright 2013-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2020, NTESS
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

#include "output_arb_basic.h"
#include "output_arb_qos_multi.h"

#include <sst/core/sharedRegion.h>

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
	    output_timing->send(1,NULL); 
	    waiting = false;
	}
#if TRACK
    if ( rtr_id == TRACK_ID && port_number == TRACK_PORT ) {
        printStatus(Simulation::getSimulation()->getSimulationOutput(),0,0);
    }
#endif
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
        printStatus(Simulation::getSimulation()->getSimulationOutput(),0,0);
    }
#endif
	if ( input_buf[vc].empty() ) return NULL;

	internal_router_event* event = input_buf[vc].front();
	input_buf[vc].pop();

	// Need to update vc_heads
	if ( input_buf[vc].empty() ) {
	    vc_heads[vc] = NULL;
	    parent->dec_vcs_with_data();
	}
	else {
        auto event = input_buf[vc].front();
        topo->route_packet(port_number, event->getVC(), event);
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
        printStatus(Simulation::getSimulation()->getSimulationOutput(),0,0);
    }
#endif
    return event;
}


PortControl::PortControl(ComponentId_t cid, Params& params,  Router* rif, int rtr_id, int port_number, Topology *topo) :
    PortInterface(cid),
    rtr_id(rtr_id),
    num_vcs(-1),
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
	sai_port_disabled(false),
	ongoing_transmit(false),
    is_idle(true),
	is_active(false),
    waiting(true),
    have_packets(false),
    start_block(0),
    parent(rif),
    output(Simulation::getSimulation()->getSimulationOutput())
{
    // Process the parameters

    // Get the port name.  For now, we only load anonymously, but when
    // user loading is enabled, this will have to check to see which
    // way it is being loaded
    std::string link_port_name = params.find<std::string>("port_name");

    // Bool to see if parameters are found
    bool found;

    // Link bandwidth
    link_bw = params.find<UnitAlgebra>("link_bw", found);
    if ( !found ) {
        merlin_abort.fatal(CALL_INFO_LONG, 1, "PortControl: link_bw must be specified\n");
    }
    if ( !link_bw.hasUnits("b/s") && !link_bw.hasUnits("B/s") ) {
        merlin_abort.fatal(CALL_INFO,-1,"PortControl: link_bw must be specified in either "
                           "b/s or B/s: %s\n",link_bw.toStringBestSI().c_str());
    }
    if ( link_bw.hasUnits("B/s") ) {
        link_bw *= UnitAlgebra("8b/B");
    }
    
    // Flit size
    flit_size = params.find<UnitAlgebra>("flit_size", found);
    if ( !found ) {
        merlin_abort.fatal(CALL_INFO_LONG, 1, "PortControl: flit_size must be specified\n");
    }
    if ( !flit_size.hasUnits("b") && !flit_size.hasUnits("B") ) {
        merlin_abort.fatal(CALL_INFO,-1,"PortControl: flit_size must be specified in either "
                           "bits (b) or bytes (B): %s\n",flit_size.toStringBestSI().c_str());
    }
    if ( flit_size.hasUnits("B") ) {
        flit_size *= UnitAlgebra("8b");
    }
    
    std::string output_latency_timebase = params.find<std::string>("output_latency","0ns");
    

    // Configure the links.  output_timing will have a temporary time bases.  It will be
    // changed once the final link BW is set.
    switch ( topo->getPortState(port_number) ) {
    case Topology::R2N:
        host_port = true;
        port_link = configureLink(link_port_name, output_latency_timebase,
                                   new Event::Handler<PortControl>(this,&PortControl::handle_input_n2r));
        if ( port_link != NULL ) {
            output_timing = configureSelfLink(link_port_name + "_output_timing", "1GHz",
                                              new Event::Handler<PortControl>(this,&PortControl::handle_output));
        }
        break;
    case Topology::R2R:
        host_port = false;
        port_link = configureLink(link_port_name, output_latency_timebase,
                                  new Event::Handler<PortControl>(this,&PortControl::handle_input_r2r));
        if ( port_link != NULL ) {
            output_timing = configureSelfLink(link_port_name + "_output_timing", "1GHz",
                                              new Event::Handler<PortControl>(this,&PortControl::handle_output));
        }
        break;
    default:
        host_port = false;
        port_link = NULL;
        break;
    }
    
	// This is the self link to enable the logic for adaptive link widths.
	// The initial call to the handler dynlink_timing->send is made in setup.
	dynlink_timing = configureSelfLink(link_port_name + "_dynlink_timing", "10us",
                                       new Event::Handler<PortControl>(this,&PortControl::handleSAIWindow));

	disable_timing = configureSelfLink(link_port_name + "_disable_timing", "1us",
                                       new Event::Handler<PortControl>(this,&PortControl::reenablePort));
    connected = true;

    if ( port_link == NULL ) {
        connected = false;
        return;
    }
    
    input_buf_size = params.find<UnitAlgebra>("input_buf_size",found);
    if ( !found ) {
        merlin_abort.fatal(CALL_INFO_LONG, 1, "PortContol: input_buf_size must be specified\n");
    }
    if ( !input_buf_size.hasUnits("b") && !input_buf_size.hasUnits("B") ) {
        merlin_abort.fatal(CALL_INFO,-1,"PortControl: input_buf_size must be specified in either "
                           "bits (b) or bytes (B): %s\n",input_buf_size.toStringBestSI().c_str());
    }
    if ( input_buf_size.hasUnits("B") ) {
        input_buf_size *= UnitAlgebra("8b");
    }
    
    output_buf_size = params.find<UnitAlgebra>("output_buf_size",found);
    if ( !found ) {
        merlin_abort.fatal(CALL_INFO_LONG, 1, "PortControl: output_buf_size must be specified\n");
    }
    if ( !output_buf_size.hasUnits("b") && !output_buf_size.hasUnits("B") ) {
        merlin_abort.fatal(CALL_INFO,-1,"PortControl: output_buf_size must be specified in either "
                           "bits (b) or bytes (B): %s\n",output_buf_size.toStringBestSI().c_str());
    }
    if ( output_buf_size.hasUnits("B") ) {
        output_buf_size *= UnitAlgebra("8b");
    }
    
    std::string input_latency_timebase = params.find<std::string>("input_latency",found);
    if ( port_link && found ) {
        port_link->addRecvLatency(1,input_latency_timebase);
    }
    
    
    // Register statistics
    std::string port_name("port");
    port_name = port_name + std::to_string(port_number);

    send_bit_count = registerStatistic<uint64_t>("send_bit_count", port_name);
    send_packet_count = registerStatistic<uint64_t>("send_packet_count", port_name);
    output_port_stalls = registerStatistic<uint64_t>("output_port_stalls", port_name);
    idle_time = registerStatistic<uint64_t>("idle_time", port_name);
    width_adj_count = registerStatistic<uint64_t>("width_adj_count", port_name);

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

    std::vector<std::string> inspector_names;
    params.find_array<std::string>("network_inspectors",inspector_names);

    // Create any NetworkInspectors
    for ( unsigned int i = 0; i < inspector_names.size(); i++ ) {
        Params empty;
        SimpleNetwork::NetworkInspector* ni = loadAnonymousSubComponent<SimpleNetwork::NetworkInspector>
            (inspector_names[i], "inspector_slot", i, ComponentInfo::INSERT_STATS, empty, port_name);
        if ( ni == NULL ) {
            merlin_abort.fatal(CALL_INFO,1,"NetworkInspector: %s, not found.\n",inspector_names[i].c_str());
        }
        network_inspectors.push_back(ni);
    }

    dlink_thresh = params.find<float>("dlink_thresh",-1.0);
    oql_track_port = params.find<bool>("oql_track_port",false);
    oql_track_remote = params.find<bool>("oql_track_remote",false);

    Params arb_params = params.find_prefix_params("arbitration:");

    std::string output_arb_name = params.find<std::string>("output_arb","merlin.arb.output.basic");
    output_arb = loadAnonymousSubComponent<OutputArbitration>
        (output_arb_name,"arbitration",0,ComponentInfo::SHARE_NONE, arb_params);

    // See if number of VNs was set explicitly and if so, see if there is a VN remapping
    num_vns = params.find<int>("num_vns",-1);
    if ( num_vns != -1 ) {
        vn_remap_shm = params.find<std::string>("vn_remap_shm","");
        if ( vn_remap_shm != "" ) {
            int size = params.find<int>("vn_remap_shm_size",-1);
            shared_region = Simulation::getSharedRegionManager()->
                getGlobalSharedRegion(vn_remap_shm, size, new SharedRegionMerger());
            shared_region->publish();
        }
    }
}


void
PortControl::initVCs(int vns, int* vcs_per_vn, internal_router_event** vc_heads_in, int* xbar_in_credits_in, int* output_queue_lengths_in)
{
    vc_heads = vc_heads_in;
    num_vns = vns;
    
    // num_vcs = vcs;
    num_vcs = 0;
    for ( int i = 0; i < vns; ++i ) {
        num_vcs += vcs_per_vn[i];
    }

    // If the port is not connected, we still need to initialize
    // vc_heads entries to NULL
    if ( !connected ) {
        for ( int i = 0; i < num_vcs; i++ ) {
            vc_heads[i] = NULL;
        }
        return;
    }
    xbar_in_credits = xbar_in_credits_in;
    output_queue_lengths = output_queue_lengths_in;
    
    // Input and output buffers
    input_buf = new port_queue_t[num_vcs];
    output_buf = new port_queue_t[num_vcs];
    
    input_buf_count = new int[num_vcs];
    output_buf_count = new int[num_vcs];
	
    for ( int i = 0; i < num_vcs; i++ ) {
        input_buf_count[i] = 0;
        output_buf_count[i] = 0;
        vc_heads[i] = NULL;
    }
	
    // Initialize credit arrays
    // xbar_in_credits = new int[vcs];
    port_ret_credits = new int[num_vcs];
    port_out_credits = new int[num_vcs];
    
    // Figure out how large the buffers are in flits

    // Need to see if we need to convert to bits
    UnitAlgebra ibs = input_buf_size;
    UnitAlgebra obs = output_buf_size;

    // if ( !ibs.hasUnits("b") && !ibs.hasUnits("B") ) {
    //     merlin_abort.fatal(CALL_INFO,-1,"input_buf_size must be specified in either "
    //                        "bits or bytes: %s\n",ibs.toStringBestSI().c_str());
    // }
    
    // if ( !obs.hasUnits("b") && !obs.hasUnits("B") ) {
    //     merlin_abort.fatal(CALL_INFO,-1,"output_buf_size must be specified in either "
    //                        "bits or bytes: %s\n",obs.toStringBestSI().c_str());
    // }

    // if ( ibs.hasUnits("B") ) ibs *= UnitAlgebra("8b/B");
    // if ( obs.hasUnits("B") ) obs *= UnitAlgebra("8b/B");


    ibs /= flit_size;
    obs /= flit_size;
    
    for ( int i = 0; i < num_vcs; i++ ) {
        port_ret_credits[i] = ibs.getRoundedValue();
        xbar_in_credits[i] = obs.getRoundedValue();
        port_out_credits[i] = 0;
    }


    // Need to start the timer for links that never send data
    idle_start = Simulation::getSimulation()->getCurrentSimCycle();
    is_idle = true;

    output_arb->setVCs(num_vns, vcs_per_vn);
}

PortControl::~PortControl() {
    if ( input_buf != NULL ) delete [] input_buf;
    if ( output_buf != NULL ) delete [] output_buf;
    if ( input_buf_count != NULL ) delete [] input_buf_count;
    if ( output_buf_count != NULL ) delete [] output_buf_count;
    if ( port_ret_credits != NULL ) delete [] port_ret_credits;
    if ( port_out_credits != NULL ) delete [] port_out_credits;
    for ( unsigned int i = 0; i < network_inspectors.size(); i++ ) {
        delete network_inspectors[i];
    }
}

void
PortControl::setup() {
    if ( !connected ) return;
	if (dlink_thresh >= 0) dynlink_timing->send(1,NULL);
    while ( init_events.size() ) {
        delete init_events.front();
        init_events.pop_front();
    }
}

void
PortControl::finish() {
    if ( !connected ) return;

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
    if ( !connected ) return;
    Event *ev;
    RtrInitEvent* init_ev;

    switch ( phase ) {
    case 0:
        // Negotiate link speed.  We will take the min of the two link speeds
        init_ev = new RtrInitEvent();
        init_ev->command = RtrInitEvent::REPORT_BW;
        init_ev->ua_value = link_bw;
        
        port_link->sendInitData(init_ev);
        
        // If this is a host port, send the endpoint ID to the LinkControl
        if ( topo->isHostPort(port_number) ) {
            init_ev = new RtrInitEvent();
            init_ev->command = RtrInitEvent::REPORT_FLIT_SIZE;
            init_ev->ua_value = flit_size;
            port_link->sendInitData(init_ev);
        
            RtrInitEvent* ev = new RtrInitEvent();
            ev->command = RtrInitEvent::REPORT_ID;
            ev->int_value = topo->getEndpointID(port_number);
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
        flit_cycle = getTimeConverter(link_clock);
        output_timing->setDefaultTimeBase(flit_cycle);
        delete ev;
        
        // Get initialization event from endpoint, but only if I am a host port
        if ( topo->isHostPort(port_number) ) {
            // Number of VNs used by the endpoint
            ev = port_link->recvInitData();
            init_ev = dynamic_cast<RtrInitEvent*>(ev);
            int req_vns = init_ev->int_value;
            if ( num_vns == -1 ) num_vns = req_vns;

            remote_rdy_for_credits = true;
            delete ev;

            // Set remote_rtr_id and remote_port_number to -1
            // indicating this is a host port
            remote_rtr_id = -1;
            remote_port_number = -1;

            // Need to report back the actual number of VNs, then the VN mapping
            init_ev = new RtrInitEvent();
            init_ev->command = RtrInitEvent::REQUEST_VNS;
            init_ev->int_value = num_vns;
            port_link->sendInitData(init_ev);

            for ( int i = 0; i < req_vns; ++i ) {
                init_ev = new RtrInitEvent();
                init_ev->command = RtrInitEvent::REQUEST_VNS;
                if ( vn_remap_shm == "" ) {
                    // No remap, just send the same value back
                    init_ev->int_value = i;
                }
                else {
                    int* map = (int*)shared_region->getRawPtr();
                    int endpoint_id = topo->getEndpointID(port_number);
                    for ( int j = 0; j < num_vns; ++j ) {
                        if ( map[endpoint_id*num_vns + j] == i ) {
                            init_ev->int_value = j;
                            break;
                        }
                    }
                }
                port_link->sendInitData(init_ev);
            }
            
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

            remote_rdy_for_credits = true;

        }
        }
        break;

    default:
        if ( host_port ) {
            // Only send to vc 0 for each VN
            // Get the number of VCs per VN
            if ( num_vcs != -1 && remote_rdy_for_credits ) {
                std::vector<int> vcs_per_vn(num_vns);
                topo->getVCsPerVN(vcs_per_vn);
                int curr_vc = 0;
                // Send credits to host, but only once for each VN
                for ( int i = 0; i < num_vns; ++i ) {
                    port_link->sendInitData(new credit_event(i,port_ret_credits[curr_vc]));
                    curr_vc += vcs_per_vn[i];
                }
                // Set all return credits to zero
                for ( int i = 0; i < num_vcs; ++i ) {
                    port_ret_credits[i] = 0;
                }
                remote_rdy_for_credits = false;
            }
            
        }
        else {
            // If my VCs have been initialized and the remote side is
            // ready to receive credits, send the credit events.
            if ( remote_rdy_for_credits ) {
                for ( int i = 0; i < num_vcs; i++ ) {
                    port_link->sendInitData(new credit_event(i,port_ret_credits[i]));
                    port_ret_credits[i] = 0;
                }
                // Make sure we only send the credits once
                remote_rdy_for_credits = false;
            }
        }
        
        // Need to recv the credits sent from the other side
        while ( ( ev = port_link->recvInitData() ) != NULL ) {
            credit_event* ce = dynamic_cast<credit_event*>(ev);
            if ( ce != NULL ) {
                if ( ce->vc >= num_vcs ) {
                    // _abort(PortControl, "Received Credit Event for VC %d.  I only know of VCS[0-%d]\n", ce->vc, num_vcs-1);
                }
                port_out_credits[ce->vc] += ce->credits;
                delete ev;
            }
            else {
                RtrInitEvent* init_ev = dynamic_cast<RtrInitEvent*>(ev);
                if ( init_ev != NULL ) {
                    remote_rdy_for_credits = true;
                    delete init_ev;
                }
                else {
                    init_events.push_back(ev);
                }
            }
        }
        break;
    }   
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
        int vn = event->getRouteVN();
        internal_router_event* rtr_event = topo->process_input(event);
        rtr_event->setCreditReturnVC(vn);
        int curr_vc = rtr_event->getVC();

	    input_buf[curr_vc].push(rtr_event);
	    input_buf_count[curr_vc]++;

	    // If this becomes vc_head we need to put it into the vc_heads
	    // array and do the routing decision here using route_packet()
	    if ( vc_heads[curr_vc] == NULL ) {
            topo->route_packet(port_number, rtr_event->getVC(), rtr_event);
            vc_heads[curr_vc] = rtr_event;
            parent->inc_vcs_with_data();
	    }
	    
	    if ( event->getTraceType() != SST::Interfaces::SimpleNetwork::Request::NONE ) {
            output.output("TRACE(%d): %" PRIu64 " ns: Received an event on port %d in router %d"
                          " (%s) on VC %d from src %" PRIu64 " to dest %" PRIu64 ".\n",
                          event->getTraceID(),
                          getCurrentSimTimeNano(),
                          port_number,
                          rtr_id,
                          getName().c_str(),
                          curr_vc,
                          event->getTrustedSrc(),
                          event->getDest());
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

	    input_buf[curr_vc].push(event);
	    input_buf_count[curr_vc]++;
        
	    // If this becomes vc_head (there isn't an event already
	    // in the array) we need to put it into the vc_heads array
	    if ( vc_heads[curr_vc] == NULL ) {
            topo->route_packet(port_number, event->getVC(), event);
            vc_heads[curr_vc] = event;
            parent->inc_vcs_with_data();
	    }
	    
	    if ( event->getTraceType() != SimpleNetwork::Request::NONE ) {
            output.output("TRACE(%d): %" PRIu64 " ns: Received an event on port %d in router %d"
                          " (%s) on VC %d from src %d to dest %d.\n",
                          event->getTraceID(),
                          getCurrentSimTimeNano(),
                          port_number,
                          rtr_id,
                          getName().c_str(),
                          curr_vc,
                          event->getSrc(),
                          event->getDest());
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
PortControl::handle_output(Event* ev) {
#if TRACK
    if ( rtr_id == TRACK_ID && port_number == TRACK_PORT ) {
        printStatus(Simulation::getSimulation()->getSimulationOutput(),0,0);
    }
#endif
	// The event is an empty event used just for timing.

	// If there is data in the topo_queue, it takes priority
	if ( !topo_queue.empty() ) {
	    TopologyEvent* event = topo_queue.front();
	    // Send an event to wake up again after packet is done
	    output_timing->send(event->getSizeInFlits(),NULL);
        
	    // Send event
	    port_link->send(1,event);
	    return;
	}
    // Use the output_arb to find VC to send
    int vc_to_send = -1;
    if ( !sai_port_disabled )
        vc_to_send = output_arb->arbitrate(getCurrentSimTime(flit_cycle),output_buf, port_out_credits, host_port, have_packets);

    if ( vc_to_send != -1 ) {
        //  We found something to send
        internal_router_event* send_event = output_buf[vc_to_send].front();
        output_buf[vc_to_send].pop();

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
                          getCurrentSimTimeNano(),
                          rtr_id,
                          getName().c_str(),
                          send_event->getVC(),
                          send_event->getSrc(),
                          send_event->getDest());
	    }
        send_bit_count->addData(send_event->getEncapsulatedEvent()->getSizeInBits());
        send_packet_count->addData(1);

        // Send the request to all the registered NetworkInspectors
        for ( unsigned int i = 0; i < network_inspectors.size(); i++ ) {
            network_inspectors[i]->inspectNetworkData(send_event->inspectRequest());
        }

	    if ( host_port ) {
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
#if TRACK
    if ( rtr_id == TRACK_ID && port_number == TRACK_PORT ) {
        printStatus(Simulation::getSimulation()->getSimulationOutput(),0,0);
    }
#endif
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
    if ( cur_link_width == max_link_width ) {
        cur_link_width = cur_link_width/2;
        link_bw = link_bw/2;
        UnitAlgebra link_clock = link_bw / flit_size;
        TimeConverter* tc = getTimeConverter(link_clock);
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
	TimeConverter* tc = getTimeConverter(link_clock);
	output_timing->setDefaultTimeBase(tc);
	width_adj_count->addData(1);
	// I need to add a delay before messages can transmit on the link
	disable_timing->send(1,NULL);
	sai_port_disabled = true;
	return true;
}

else return false;

}
