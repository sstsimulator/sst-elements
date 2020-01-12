// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2019, NTESS
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

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>
#include <sst/core/unitAlgebra.h>

#include "sst/elements/merlin/merlin.h"
#include "sst/elements/merlin/test/pt2pt/pt2pt_test.h"

using namespace std;
using namespace SST::Merlin;
using namespace SST::Interfaces;

pt2pt_test::pt2pt_test(ComponentId_t cid, Params& params) :
    Component(cid),
    id(-1),
    packets_sent(0)
{
    out.init(getName() + ": ", 0, 0, Output::STDOUT);

    UnitAlgebra link_bw = params.find<UnitAlgebra>("link_bw",UnitAlgebra("2GB/s"));
    
    UnitAlgebra packet_size_ua = params.find<UnitAlgebra>("packet_size",UnitAlgebra("512b"));
    
    if ( !packet_size_ua.hasUnits("b") && !packet_size_ua.hasUnits("B") ) {
        merlin_abort.fatal(CALL_INFO,-1,"packet_size must be specified in either "
                           "bits or bytes: %s\n",packet_size_ua.toStringBestSI().c_str());
    }
    if ( packet_size_ua.hasUnits("B") ) packet_size_ua *= UnitAlgebra("8b/B");
    packet_size = packet_size_ua.getRoundedValue();
    
    packets_to_send = params.find<int>("packets_to_send",100);
    packets_recd = 0;
    
    buffer_size = params.find<UnitAlgebra>("buffer_size","128B");

    if ( buffer_size.hasUnits("B") ) buffer_size *= UnitAlgebra("8b/B");

    if ( packet_size > buffer_size.getRoundedValue() ) {
        merlin_abort.fatal(CALL_INFO,-1,"buffer_size must be greater than or equal to packet_size\n");
    }
    
    // Create a LinkControl object
    // First see if it is defined in the python
    link_control = loadUserSubComponent<SST::Interfaces::SimpleNetwork>
        ("networkIF", ComponentInfo::SHARE_NONE, 1 /* vns */);

    if ( !link_control ) {
#ifndef SST_ENABLE_PREVIEW_BUILD
        // Not defined in python code.  See if this uses the legacy
        // API.  If so, load it with loadSubComponent.  Otherwise, use
        // the default linkcontrol (merlin.linkcontrol) loaded with
        // the new API.
        bool found;

        // Get the link control to be used
        std::string link_control_name = params.find<std::string>("linkcontrol",found);
        if ( found ) {
            // Legacy
DISABLE_WARN_DEPRECATED_DECLARATION
            link_control = (SimpleNetwork*)loadSubComponent(link_control_name, this, params);
REENABLE_WARNING
            link_control->initialize("rtr", link_bw, 1, buffer_size, buffer_size);
        }
        else {
            // Just load the default
            Params if_params;

            if_params.insert("link_bw",params.find<std::string>("link_bw","2GB/s"));
            if_params.insert("input_buf_size",params.find<std::string>("bufffer_size","128B"));
            if_params.insert("output_buf_size",params.find<std::string>("buffer_size","128B"));            
            if_params.insert("port_name","rtr");
        
            link_control = loadAnonymousSubComponent<SST::Interfaces::SimpleNetwork>
                ("merlin.linkcontrol", "networkIF", 0,
                 ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, if_params, 1 /* vns */);
        }
#else
        // Not defined in python, just load the default
        Params if_params;
        
        if_params.insert("link_bw",params.find<std::string>("link_bw","2GB/s"));
        if_params.insert("input_buf_size",params.find<std::string>("bufffer_size","128B"));
        if_params.insert("output_buf_size",params.find<std::string>("buffer_size","128B"));            
        if_params.insert("port_name","rtr");
        
        link_control = loadAnonymousSubComponent<SST::Interfaces::SimpleNetwork>
            ("merlin.linkcontrol", "networkIF", 0,
             ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, if_params, 1 /* vns */);
#endif        
    }
    



    // // Register a clock
    // registerClock( "1GHz", new Clock::Handler<pt2pt_test>(this,&pt2pt_test::clock_handler), false);
    
    params.find_array<int>("src",src);

    params.find_array<int>("dest",dest);

    if ( src.size() == 0 || dest.size() == 0 || src.size() != dest.size() ) {
        merlin_abort.fatal(CALL_INFO,-1,"pt2pt_test: must specify params \"src\" and \"dest\" and they must be the same length arrays\n");
    }

    params.find_array<UnitAlgebra>("stream_delays",delays);
    
    if ( delays.size() == 0 ) {
        for ( int i = 0; i < src.size(); ++i ) {
            delays.emplace_back("0ns");
        }
    }

    if ( delays.size() != src.size() ) {
        merlin_abort.fatal(CALL_INFO,-1,"pt2pt_test: stream_delays array must be same length as \"src\" and \"dest\" arrays\n");
    }

    self_link = configureSelfLink("start_timing", "1ns",
                                  new Event::Handler<pt2pt_test>(this,&pt2pt_test::start));
    
    report_timing = configureSelfLink("report_timing", "1ns",
                                  new Event::Handler<pt2pt_test>(this,&pt2pt_test::report_bw));
    
    // if ( id == 1 ) {
    //     // self_link = configureSelfLink("complete_link", link_bw_s,
	// 	// 		      new Event::Handler<pt2pt_test>(this,&pt2pt_test::handle_complete));

    //     // Configure a new self link to be used to time the
    //     // serialization latency of the last packet.  We won't know
    //     // the final BW until the network is intialized, so we'll put
    //     // in a dummy value, then change it later.
    //     self_link = configureSelfLink("complete_link", "2GHz",
	// 			      new Event::Handler<pt2pt_test>(this,&pt2pt_test::handle_complete));
    // }

    my_dest = -1;
    
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    report_interval = params.find<UnitAlgebra>("report_interval","0ns");
    out.output("report_interval = %s\n",report_interval.toStringBestSI().c_str());
    pkts_in_interval = 0;
    last_pkt_recd = 0;
    interval_start_bw = "0b/s";
}

void pt2pt_test::finish()
{
    link_control->finish();

    // Compute bandwidths and write out report
    for ( auto& x : my_recvs ) {
        if ( x.second.end_arrival == 0 ) {
            // Ended early, compute using current sim time
            x.second.end_arrival = Simulation::getSimulation()->getEndSimCycle();
        }
        // Compute bandwidth in bits/core time quantum
        // UnitAlgebra bits_sent = UnitAlgebra("1b") * packets_to_send * packet_size;
        UnitAlgebra bits_sent = UnitAlgebra("1b") * x.second.packets_recd * packet_size;
        UnitAlgebra start_time = Simulation::getSimulation()->getTimeLord()->getTimeBase() * x.second.first_arrival;
        UnitAlgebra end_time = Simulation::getSimulation()->getTimeLord()->getTimeBase() * x.second.end_arrival;
        // TODO: Still need to tweak to account for serialization latency of the last packet
        UnitAlgebra total_time = Simulation::getSimulation()->getTimeLord()->getTimeBase() * (x.second.end_arrival - x.second.first_arrival);
        
        UnitAlgebra bw = bits_sent / total_time;

        Simulation::getSimulationOutput().output(
            "For src = %d and dest = %d:\n"
            "  First packet received at: %s\n"
            "  Last packet received at: %s\n"
            "  Bandwidth: %s (%s)\n\n",
            x.first, id,
            start_time.toStringBestSI().c_str(),
            end_time.toStringBestSI().c_str(),
            bw.toStringBestSI().c_str(), (bw / UnitAlgebra("8 b/B")).toStringBestSI().c_str());
        
    }
}

void pt2pt_test::start(Event* ev)
{
    while ( link_control->spaceToSend(0,packet_size) && packets_sent < packets_to_send ) {
        SimpleNetwork::Request* req = new SimpleNetwork::Request();
        
        req->dest = my_dest;
        req->src = id;
        req->vn = 0;
        req->size_in_bits = packet_size;
        link_control->send(req,0);
        ++packets_sent;
    }
}

void pt2pt_test::setup()
{
    TraceFunction trace(CALL_INFO_LONG);
    trace.output("id = %d\n",id);
    link_control->setup();

    if ( my_dest != -1 ) {
        trace.output("I'm a sender\n");
        link_control->setNotifyOnSend(new SimpleNetwork::Handler<pt2pt_test>(this,&pt2pt_test::send_handler));
        // Compute delay in nanoseconds
        UnitAlgebra delay_in_ns = my_delay / UnitAlgebra("1ns");
        self_link->send(delay_in_ns.getRoundedValue(),NULL);
    }

    if ( !my_recvs.empty() ) {
        trace.output("I'm a receiver\n");
        link_control->setNotifyOnReceive(new SimpleNetwork::Handler<pt2pt_test>(this,&pt2pt_test::recv_handler));
    }

    // If I am a receiver and report_interval is not zero, set up
    // bandwidth reporting
    if ( !my_recvs.empty() && (report_interval > UnitAlgebra("0ns")) ) {
        trace.output("Setting up report interval of %s\n",report_interval.toStringBestSI().c_str());
        // First compute the serialization time of a packet
        const UnitAlgebra& bw = link_control->getLinkBW();
        UnitAlgebra pkt_size("1b");
        pkt_size *= packet_size;

        UnitAlgebra ser_time = pkt_size / bw;
        pkt_ser_cycles = (ser_time / Simulation::getSimulation()->getTimeLord()->getTimeBase()).getRoundedValue();

        // Create a number that when multiplied by the number of
        // packets received in the interval, will give the bandwidth
        bw_multiplier = pkt_size / report_interval;

        // For packets that don't finish serializing in the reporting
        // window, we'll have to compute the contribution to the
        // bandwidth as a percentage of packet that arrived in the
        // window.
        bw_multiplier_partial = (pkt_size / report_interval) / pkt_ser_cycles;

        // Now, send a wake-up to do the reporting.  First, we need to
        // set the timebase to be the interval time
        report_timing->setDefaultTimeBase(getTimeConverter(report_interval));
        report_timing->send(1,NULL);
    }
    
    // if ( id == 1 ) {
    //     UnitAlgebra link_bw = link_control->getLinkBW();
    //     // Invert this to get a time per bit
    //     std::cout << link_bw.toStringBestSI() << std::endl;
    //     link_bw.invert();
    //     std::cout << link_bw.toStringBestSI() << std::endl;

    //     link_bw *= (UnitAlgebra("1b") *= packet_size);
    //     TimeConverter* tc = getTimeConverter(link_bw);
    //     std::cout << link_bw.toStringBestSI() << std::endl;
    //     self_link->setDefaultTimeBase(tc);
    //     std::cout << tc->getFactor() << std::endl;
    // }
}

void
pt2pt_test::init(unsigned int phase)
{
    link_control->init(phase);
    if ( link_control->isNetworkInitialized() && id == -1 ) {
        id = link_control->getEndpointID();
        // See if I am a src or dest and set up the right data
        // structures
        for ( int i = 0; i < src.size(); ++i ) {
            if ( src[i] == id ) {
                if ( my_dest != -1 ) {
                    merlin_abort.fatal(CALL_INFO,-1,"Multiple destinations specified for id %d, can only specify one\n",id);
                }
                my_dest = dest[i];
                my_delay = delays[i];
            }
        }

        for ( int i = 0; i < dest.size(); ++i ) {
            if ( dest[i] == id ) {
                my_recvs[src[i]].packets_recd = 0;
            }
        }

        if ( my_dest == -1 && my_recvs.empty() ) {
            // Not participatiing
            primaryComponentOKToEndSim();
        }
    }
}

bool
pt2pt_test::send_handler(int vn) {
    // TraceFunction trace(CALL_INFO);
    while ( link_control->spaceToSend(0,packet_size) && packets_sent < packets_to_send ) {
        SimpleNetwork::Request* req = new SimpleNetwork::Request();
        
        req->dest = my_dest;
        req->src = id;
        req->vn = 0;
        req->size_in_bits = packet_size;

        // req->setTraceType(SimpleNetwork::Request::FULL);
        req->setTraceID(id * 100 + packets_sent);

        link_control->send(req,0);
        // std::cout << id << ": Sending packet to: " << my_dest << " at " << Simulation::getSimulation()->getCurrentSimCycle() << std::endl;
        ++packets_sent;
    }

    if ( packets_sent == packets_to_send ) {
        // primaryComponentOKToEndSim();
        return false; // remove myself from the handler list
    }
    return true;
}

bool
pt2pt_test::recv_handler(int vn) {
    // TraceFunction trace(CALL_INFO);
    SimpleNetwork::Request* req = link_control->recv(0);
    
    int src = req->src;

    // std::cout << "Recived packet from: " << src << " at " << Simulation::getSimulation()->getCurrentSimCycle() << std::endl;

    auto data_it = my_recvs.find(src);
    if ( data_it == my_recvs.end() ) {
        // fatal
    }
    
    recv_data& data = data_it->second;
    if ( data.packets_recd == 0 ) {
        data.first_arrival = Simulation::getSimulation()->getCurrentSimCycle();
    }
    
    data.packets_recd++;
    packets_recd++;

    pkts_in_interval++;
    last_pkt_recd = Simulation::getSimulation()->getCurrentSimCycle();
    
    if ( data.packets_recd == packets_to_send ) {
        data.end_arrival = Simulation::getSimulation()->getCurrentSimCycle();
    }

    if ( packets_recd == ( my_recvs.size() * packets_to_send ) ) {
        // Done receiving
        primaryComponentOKToEndSim();
    }

    delete req;
    
    return true;
}

void pt2pt_test::report_bw(Event* ev) {

    // Figure out if we have a partial packet at the end of the window
    SimTime_t curr_cycle = Simulation::getSimulation()->getCurrentSimCycle();
    UnitAlgebra bw = interval_start_bw;
    if ( last_pkt_recd + pkt_ser_cycles > curr_cycle ) {
        // Partial packet
        SimTime_t in_window = curr_cycle - last_pkt_recd;
        SimTime_t after_window = pkt_ser_cycles - in_window;

        bw += (bw_multiplier * (pkts_in_interval-1)) + (bw_multiplier_partial * in_window);
        interval_start_bw = bw_multiplier_partial * after_window;
    }
    else {
        // No partial packet
        bw += bw_multiplier * pkts_in_interval;
        interval_start_bw = "0b/s";
    }

    bw += interval_start_bw;
    // out.output("%d, %" PRIu64 ", %s\n",id,curr_cycle,bw.toStringBestSI().c_str());
    out.output("%d, %" PRIu64 ", %s\n",id,curr_cycle,(bw/UnitAlgebra("1Gb/s")).toString().c_str());

    // Reset the interval variables
    pkts_in_interval = 0;

    // Send event for next reporting interval
    report_timing->send(1,NULL);
}

/*
bool
pt2pt_test::clock_handler(Cycle_t cycle)
{
    if ( id == 0 ) {
	// ID 0 is the sender

	// Do a bandwidth test
	if ( packets_sent == packets_to_send ) {
      primaryComponentOKToEndSim();
	    
	    cout << "0: Done" << endl;
	    return true;  // Take myself off clock list
	}
	
	if (  link_control->spaceToSend(1,packet_size) ) {
	    pt2pt_test_event* ev = new pt2pt_test_event();
	    ev->dest = 1;
	    ev->vn = 1;
	    ev->size_in_flits = packet_size;
	    link_control->send(ev,1);
	    ++packets_sent;
	}
    }
    
    else {
	// ID 1 is the receiver
	pt2pt_test_event* rec_ev = static_cast<pt2pt_test_event*>(link_control->recv(1));
	if ( rec_ev != NULL ) {
	    if ( packets_recd == 0 ) start_time = getCurrentSimTimeNano();
	    ++packets_recd;

	    if ( packets_recd == packets_to_send ) {
		// Need to send this event to a self link to account
		// for serialization latency.  This will trigger an
		// event handler that will compute the BW.
		self_link->send(packet_size,rec_ev);
		return true;
	    }
	    else {
		delete rec_ev;
	    }
	}
    }

    return false;
}
*/
// bool
// pt2pt_test::clock_handler(Cycle_t cycle)
// {
//     if ( id == 0 ) {
//         // Do a bandwidth and latency test
//         if ( packets_sent == packets_to_send ) {
//             primaryComponentOKToEndSim();
//             return true;  // Take myself off clock list
//         }
	
//         if ( link_control->spaceToSend(0,packet_size) ) {
//             pt2pt_test_event* ev = new pt2pt_test_event();
//             SimpleNetwork::Request* req = new SimpleNetwork::Request();
//             req->givePayload(ev);
            
//             // if ( packets_sent == 0 ) ev->setTraceType(RtrEvent::FULL);
//             // else ev->setTraceType(RtrEvent::NONE);
//             if ( packets_sent == 0 ) {
//                 ev->start_time = getCurrentSimTimeNano();
//                 req->setTraceType(SimpleNetwork::Request::FULL);
//             }
//             req->setTraceID(packets_sent);
//             req->dest = 1;
//             req->src = 0;
//             req->vn = 0;
//             req->size_in_bits = packet_size;
//             link_control->send(req,0);
//             ++packets_sent;
//         }
//     }    
//     else {
//         // ID 1 is the receiver
//         if ( link_control->requestToReceive(0) ) {
//             SimpleNetwork::Request* req = link_control->recv(0);
//             pt2pt_test_event* rec_ev = static_cast<pt2pt_test_event*>(req->takePayload());
//             // cout << "received packet at " << getCurrentSimTimeNano() << endl;
//             if ( packets_recd == 0 ) {
//                 start_time = getCurrentSimTimeNano();
//                 latency = start_time - rec_ev->start_time;
//             }
//             ++packets_recd;
	    
//             if ( packets_recd == packets_to_send ) {
//                 // Need to send this event to a self link to account
//                 // for serialization latency.  This will trigger an
//                 // event handler that will compute the BW.
//                 std::cout << getCurrentSimTimeNano() << ", " << packet_size << std::endl;
//                 self_link->send(1,rec_ev); 
//                 delete req;
//                 return true;
//             }
//             else {
//                 delete req;
//                 delete rec_ev;
//             }
//         }

//         // rec_ev = static_cast<pt2pt_test_event*>(link_control->recv(1));
//         // if ( rec_ev != NULL ) {
//         //     if ( packets_recd == 0 ) {
//         //         start_time = getCurrentSimTimeNano();
//         //         latency = start_time - rec_ev->start_time;
//         //     }
//         //     ++packets_recd;
            
//         //     if ( packets_recd == 2*packets_to_send ) {
//         //         // Need to send this event to a self link to account
//         //         // for serialization latency.  This will trigger an
//         //         // event handler that will compute the BW.
//         //         self_link->send(packet_size,rec_ev); 
//         //         return true;
//         //     }
//         //     else {
//         //         delete rec_ev;
//         //     }
//         // }
//     }
    
//     return false;
// }

// void
// pt2pt_test::handle_complete(Event* ev) {
//     delete ev;

//     std::cout << getCurrentSimTimeNano() << std::endl;
//     // Compute BW
//     SimTime_t end_time = getCurrentSimTimeNano();
    
//     double total_sent = (packet_size * packets_to_send);
//     double total_time = (double)end_time - double (start_time);
//     double bw = total_sent / total_time;

//     cout << "Latency = " << latency << " ns" << endl;
//     cout << "Start time = " << start_time << endl;
//     cout << "End time = " << end_time << endl;
    
//     cout << "Total sent = " << total_sent << endl;
    
//     cout << "BW = " << bw << " Gbits/sec" << endl;
//     primaryComponentOKToEndSim();

// }

