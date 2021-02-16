// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include "hr_router/hr_router.h"

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>
#include <sst/core/unitAlgebra.h>
#include <sst/core/sharedRegion.h>

#include <sstream>
#include <string>

#include <signal.h>

#include "merlin.h"

using namespace SST::Merlin;
using namespace SST::Interfaces;
using namespace std;

int hr_router::num_routers = 0;
int hr_router::print_debug = 0;

// Helper functions used only in this file
static string trim(string str)
{
    // Find whitespace in front
    int front_index = 0;
    while ( isspace(str[front_index]) ) front_index++;

    // Find whitespace in back
    int back_index = str.length() - 1;
    while ( isspace(str[back_index]) ) back_index--;

    return str.substr(front_index,back_index-front_index+1);
}

static void split(string input, string delims, vector<string>& tokens) {
    if ( input.length() == 0 ) return;
    size_t start = 0;
    size_t stop = 0;;
    vector<string> ret;

    do {
        stop = input.find_first_of(delims,start);
        tokens.push_back(input.substr(start,stop-start));
        start = stop + 1;
    } while (stop != string::npos);

    for ( unsigned int i = 0; i < tokens.size(); i++ ) {
        tokens[i] = trim(tokens[i]);
    }
}

static std::string getLogicalGroupParam(const Params& params, Topology* topo, int port,
                                        std::string param, std::string default_val = "") {
    // Use topology object to get the group for the port
    std::string group = topo->getPortLogicalGroup(port);

    // Create fully qualified key name
    std::string key = param;
    key.append(std::string(":")).append(group);

    std::string value = params.find<std::string>(key);

    if ( value == "" ) {
        // Look for default value
        value = params.find<std::string>(param, default_val);
        if ( value == "" ) {
            // Abort
            merlin_abort.fatal(CALL_INFO, -1, "hr_router requires %s to be specified\n", param.c_str());
        }
    }
    return value;
}

static UnitAlgebra getLogicalGroupParamUA(const Params& params, Topology* topo, int port,
                                          std::string param, std::string default_val = "") {

    std::string value = getLogicalGroupParam(params,topo,port,param,default_val);

    UnitAlgebra ua(value);
    // If units were in Bytes, convert to bits
    if ( ua.hasUnits("B") || ua.hasUnits("B/s") ) {
        ua *= UnitAlgebra("8b/B");
    }

    return ua;
}


// Start class functions
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
    Router(cid),
    num_vcs(-1),
    output(Simulation::getSimulation()->getSimulationOutput())
{

    // Get the options for the router
    id = params.find<int>("id",-1);
    if ( id == -1 ) {
        merlin_abort.fatal(CALL_INFO, -1, "hr_router requires id to be specified\n");
    }

    num_ports = params.find<int>("num_ports",-1);
    if ( num_ports == -1 ) {
        merlin_abort.fatal(CALL_INFO, -1, "hr_router requires num_ports to be specified\n");
    }


    // Get the topology
    topo = loadUserSubComponent<SST::Merlin::Topology>
        ("topology", ComponentInfo::SHARE_NONE, num_ports, id);

    if ( !topo ) {
        merlin_abort.fatal(CALL_INFO_LONG, 1, "hr_router requires topology to be specified in input file\n");
    }

    // Get the number of VNs
    num_vns = params.find<int>("num_vns",2);
    num_vcs = topo->computeNumVCs(num_vns);

    // Check to see if remap is on
    vn_remap_shm = params.find<std::string>("vn_remap_shm","");
    if ( vn_remap_shm != "" ) {
        // If I'm id 0, create the shared region
        std::vector<int> vec;
        params.find_array<int>("vn_remap",vec);
        if ( vec.size() == 0 ) {
            merlin_abort.fatal(CALL_INFO, 1, "if vn_remap_shm is specified, a map must be supplied using vn_remap\n");
        }
        vn_remap_shm_size = vec.size() * sizeof(int);
        if ( id == 0 ) {
            SharedRegion* sr = Simulation::getSharedRegionManager()->
                getGlobalSharedRegion(vn_remap_shm, vn_remap_shm_size, new SharedRegionMerger());
            for ( int i = 0; i < vec.size(); ++i ) {
                sr->modifyArray(i,vec[i]);
            }
            sr->publish();
        }
    }

    // Parse all the timing parameters

    // Flit size
    std::string flit_size_s = params.find<std::string>("flit_size");
    if ( flit_size_s == "" ) {
        merlin_abort.fatal(CALL_INFO, -1, "hr_router requires flit_size to be specified\n");
        abort();
    }
    UnitAlgebra flit_size(flit_size_s);

    if ( flit_size.hasUnits("B") ) {
        // Need to convert to bits per second
        flit_size *= UnitAlgebra("8b/B");
    }

    // Link BW default.  Can be overwritten using logical groups
    std::string link_bw_s = params.find<std::string>("link_bw");
    UnitAlgebra link_bw(link_bw_s);

    if ( link_bw.hasUnits("B/s") ) {
        // Need to convert to bits per second
        link_bw *= UnitAlgebra("8b/B");
    }

    // Cross bar bandwidth
    std::string xbar_bw_s = params.find<std::string>("xbar_bw");
    if ( xbar_bw_s == "" ) {
        merlin_abort.fatal(CALL_INFO, -1, "hr_router requires xbar_bw to be specified\n");
    }

    UnitAlgebra xbar_bw_ua(xbar_bw_s);
    if ( xbar_bw_ua.hasUnits("B/s") ) {
        // Need to convert to bits per second
        xbar_bw_ua *= UnitAlgebra("8b/B");
    }

    UnitAlgebra xbar_clock;
    xbar_clock = xbar_bw_ua / flit_size;


    std::string input_latency = params.find<std::string>("input_latency", "0ns");
    std::string output_latency = params.find<std::string>("output_latency", "0ns");


    // Create all the PortControl blocks
    ports = new PortInterface*[num_ports];

    std::string input_buf_size = params.find<std::string>("input_buf_size", "0");
    std::string output_buf_size = params.find<std::string>("output_buf_size", "0");


    // Naming convention is from point of view of the xbar.  So,
    // in_port_busy is >0 if someone is writing to that xbar port and
    // out_port_busy is >0 if that xbar port being read.
    in_port_busy = new int[num_ports];
    out_port_busy = new int[num_ports];

    progress_vcs = new int[num_ports];

    std::string inspector_config = params.find<std::string>("network_inspectors", "");
    split(inspector_config,",",inspector_names);

    bool oql_track_port = params.find<bool>("oql_track_port","false");
    bool oql_track_remote = params.find<bool>("oql_track_remote","false");

    params.enableVerify(false);

    Params pc_params = params.find_prefix_params("portcontrol:");

    pc_params.insert("flit_size", flit_size.toStringBestSI());
    if (pc_params.contains("network_inspectors")) pc_params.insert("network_inspectors", params.find<std::string>("network_inspectors", ""));
    pc_params.insert("oql_track_port", params.find<std::string>("oql_track_port","false"));
    pc_params.insert("oql_track_remote", params.find<std::string>("oql_track_remote","false"));

    for ( int i = 0; i < num_ports; i++ ) {
        in_port_busy[i] = 0;
        out_port_busy[i] = 0;
        progress_vcs[i] = -1;

        std::stringstream port_name;
        port_name << "port";
        port_name << i;

        // For each port, some default parameters can be overwritten
         // by logical group parameters (link_bw, input_buf_size,
        // output_buf_size, input_latency, output_latency).

        pc_params.insert("port_name", port_name.str());
        pc_params.insert("link_bw", getLogicalGroupParam(params,topo,i,"link_bw") );
        pc_params.insert("input_latency", getLogicalGroupParam(params,topo,i,"input_latency","0ns"));
        pc_params.insert("output_latency", getLogicalGroupParam(params,topo,i,"output_latency","0ns"));
        pc_params.insert("input_buf_size", getLogicalGroupParam(params,topo,i,"input_buf_size"));
        pc_params.insert("output_buf_size", getLogicalGroupParam(params,topo,i,"output_buf_size"));
        pc_params.insert("dlink_thresh", getLogicalGroupParam(params,topo,i,"dlink_thresh", "-1"));
        pc_params.insert("vn_remap_shm", vn_remap_shm);
        pc_params.insert("vn_remap_shm_size", std::to_string(vn_remap_shm_size));
        pc_params.insert("num_vns", std::to_string(num_vns));

        // ports[i] = new PortControl(this, id, port_name.str(), i,
        //                            getLogicalGroupParamUA(params,topo,i,"link_bw"),
        //                            flit_size, topo,
        //                            1, getLogicalGroupParam(params,topo,i,"input_latency","0ns"),
        //                            1, getLogicalGroupParam(params,topo,i,"output_latency","0ns"),
        //                            getLogicalGroupParam(params,topo,i,"input_buf_size"),
        //                            getLogicalGroupParam(params,topo,i,"output_buf_size"),
        //                            inspector_names,
		// 						   std::stof(getLogicalGroupParam(params,topo,i,"dlink_thresh", "-1")),
        //                            oql_track_port,oql_track_remote);

        ports[i] = loadAnonymousSubComponent<PortInterface>
            ("merlin.portcontrol","portcontrol", i, ComponentInfo::SHARE_PORTS | ComponentInfo::SHARE_STATS | ComponentInfo::INSERT_STATS,
             pc_params,this,id,i,topo);

    }
    params.enableVerify(true);

    // Get the Xbar arbitration
    std::string xbar_arb = params.find<std::string>("xbar_arb","merlin.xbar_arb_lru");

    Params empty_params; // Empty params sent to subcomponents
    arb =
        loadAnonymousSubComponent<XbarArbitration>(xbar_arb, "XbarArb", 0, ComponentInfo::INSERT_STATS, empty_params);

    my_clock_handler = new Clock::Handler<hr_router>(this,&hr_router::clock_handler);
    xbar_tc = registerClock( xbar_clock, my_clock_handler);
    num_routers++;

#if VERIFY_DECLOCKING
    clocking = true;
#endif

    // Check to make sure that the xbar BW is equal to or greater than
    // the link BW, otherwise the model runs into problems
    // if ( xbar_tc->getFactor() > link_tc->getFactor() ) {
    if ( xbar_bw_ua < link_bw  ) {
        std::cout << "ERROR: hr_router requires xbar_bw to be greater than or equal to link_bw" << std::endl;
        std::cout << "  xbar_bw = " << xbar_bw_ua.toStringBestSI() << ", link_bw = "
                  << link_bw.toStringBestSI() << std::endl;
        abort();
    }
    // Register statistics
    xbar_stalls = new Statistic<uint64_t>*[num_ports];
    for ( int i = 0; i < num_ports; i++ ) {
        std::string port_name("port");
        port_name = port_name + std::to_string(i);
        xbar_stalls[i] = registerStatistic<uint64_t>("xbar_stalls",port_name);
    }

    init_vcs();
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

    int64_t elapsed_cycles = next_cycle - unclocked_cycle;


#if !VERIFY_DECLOCKING
    // Fix up the busy variables
    for ( int i = 0; i < num_ports; i++ ) {
    	// Should stop at zero, need to find a clean way to do this
    	// with no branch.  For now it should work.
        int64_t tmp = in_port_busy[i] - elapsed_cycles;
    	if ( tmp < 0 ) in_port_busy[i] = 0;
        else in_port_busy[i] = tmp;
        tmp = out_port_busy[i] - elapsed_cycles;
    	if ( tmp < 0 ) out_port_busy[i] = 0;
        else out_port_busy[i] = tmp;
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

void
hr_router::printStatus(Output& out)
{
    out.output("Start Router:  id = %d\n", id);
    for ( int i = 0; i < num_ports; i++ ) {
        ports[i]->printStatus(out, out_port_busy[i], in_port_busy[i]);
    }
    out.output("End Router: id = %d\n", id);
}


bool
hr_router::clock_handler(Cycle_t cycle)
{
    // TraceFunction trace(CALL_INFO_LONG);
    // If there are no events in the input queues, then we can remove
    // ourselves from the clock queue, as long as the arbitration unit
    // says it's okay.
    if ( get_vcs_with_data() == 0 ) {
#if VERIFY_DECLOCKING
        if ( clocking ) {
            if ( arb->isOkayToPauseClock() ) {
                setRequestNotifyOnEvent(true);
                unclocked_cycle = cycle;
                clocking = false;
            }
        }
#else
        if ( arb->isOkayToPauseClock() ) {
            setRequestNotifyOnEvent(true);
            unclocked_cycle = cycle;
            return true;
        }
        else {
            return false;
        }

#endif
    }
    // Loop through all the events at the heads of the queues and call
    // route
    int index = 0;
    for ( int i = 0; i < num_ports; i++ ) {
        for ( int j = 0; j < num_vcs; j++ ) {
            if ( vc_heads[index] != NULL ) {
                topo->reroute(i,j,vc_heads[index]);
            }
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
        // if ( progress_vcs[i] != -1 ) {
        if ( progress_vcs[i] > -1 ) {
            internal_router_event* ev = ports[i]->recv(progress_vcs[i]);
            ports[ev->getNextPort()]->send(ev,ev->getVC());
            // std::cout << "" << id << ": " << "Moving VC " << progress_vcs[i] <<
            // 	" for port " << i << " to port " << ev->getNextPort() << std::endl;

            if ( ev->getTraceType() == SimpleNetwork::Request::FULL ) {
                output.output("TRACE(%d): %" PRIu64 " ns: Copying event (src = %d, dest = %d) "
                              "over crossbar in router %d (%s) from port %d, VC %d to port"
                              " %d, VC %d.\n",
                              ev->getTraceID(),
                              getCurrentSimTimeNano(),
                              ev->getSrc(),
                              ev->getDest(),
                              id,
                              getName().c_str(),
                              i,
                              progress_vcs[i] ,
                              ev->getNextPort(),
                              ev->getVC());
            }

        }
        else if ( progress_vcs[i] == -2 ) {
                xbar_stalls[i]->addData(1);
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
    for ( int i = 0; i < num_ports; i++ ) {
    	ports[i]->setup();
    }
}

void hr_router::finish()
{
    for ( int i = 0; i < num_ports; i++ ) {
    	ports[i]->finish();
    }

}

void
hr_router::init(unsigned int phase)
{
    for ( int i = 0; i < num_ports; i++ ) {
        // std::cout << "Calling init on port: " << i << ", in phase " << phase << std::endl;
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
                    ports[*j]->sendUntimedData(ire->getEncapsulatedEvent()->clone());
                    break;
                case Topology::R2R: {
                    internal_router_event *new_ire = ire->clone();
                    new_ire->setEncapsulatedEvent(ire->getEncapsulatedEvent()->clone());
                    ports[*j]->sendUntimedData(new_ire);
                    break;
                }
                default:
                    break;
                }
            }
            delete ire;
        }
    }


    // Always do the above.  A few specific things to do during init

    // After phase 1, all the PortControl blocks will have reported
    // the requested VNs.  Now we need to translate this to the number
    // of VCs needed.
    // if ( phase == 1 ) {
    //     num_vcs = topo->computeNumVCs(requested_vns);
    //     init_vcs();
    // }

}

void
hr_router::complete(unsigned int phase)
{
    for ( int i = 0; i < num_ports; i++ ) {
        // std::cout << "Calling init on port: " << i << ", in phase " << phase << std::endl;
        ports[i]->complete(phase);
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
                    ports[*j]->sendUntimedData(ire->getEncapsulatedEvent()->clone());
                    break;
                case Topology::R2R: {
                    internal_router_event *new_ire = ire->clone();
                    new_ire->setEncapsulatedEvent(ire->getEncapsulatedEvent()->clone());
                    ports[*j]->sendUntimedData(new_ire);
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


void
hr_router::init_vcs()
{
    vc_heads = new internal_router_event*[num_ports*num_vcs];
    xbar_in_credits = new int[num_ports*num_vcs];
    output_queue_lengths = new int[num_ports*num_vcs];
    for ( int i = 0; i < num_ports*num_vcs; i++ ) {
        vc_heads[i] = NULL;
        xbar_in_credits[i] = 0;
        output_queue_lengths[i] = 0;
    }

    int* vcs_per_vn = new int[num_vns];
    // For now, all VNs have the same number of VCs
    int vpv = topo->computeNumVCs(1);
    for ( int i = 0; i < num_vns; ++i ) {
        vcs_per_vn[i] = vpv;
    }
    for ( int i = 0; i < num_ports; i++ ) {
        ports[i]->initVCs(num_vns,vcs_per_vn,&vc_heads[i*num_vcs],&xbar_in_credits[i*num_vcs],&output_queue_lengths[i*num_vcs]);
    }
    delete[] vcs_per_vn;

    topo->setOutputBufferCreditArray(xbar_in_credits, num_vcs);
    topo->setOutputQueueLengthsArray(output_queue_lengths, num_vcs);

    // Now that we have the number of VCs we can finish initializing
    // arbitration logic
    arb->setPorts(num_ports,num_vcs);


}
