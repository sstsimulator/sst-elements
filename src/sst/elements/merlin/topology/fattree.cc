// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2020, NTESS
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
#include "fattree.h"

#include <algorithm>
#include <stdlib.h>


using namespace SST::Merlin;
using namespace std;


void
topo_fattree::parseShape(const std::string &shape, int *downs, int *ups) const
{
    size_t start = 0;
    size_t end = 0;

    int levels = std::count(shape.begin(), shape.end(), ':') + 1;
    for ( int i = 0; i < levels; i++ ) {
        end = shape.find(':',start);
        size_t length = end - start;
        std::string sub = shape.substr(start,length);
        
        // Get the up and down
        size_t comma = sub.find(',');
        string down;
        string up;
        if ( comma == string::npos ) {
            down = sub;
            up = "0";
        }
        else {
            down = sub.substr(0,comma);
            up = sub.substr(comma+1);
        }
        
        downs[i] = strtol(down.c_str(), NULL, 0);
        ups[i] = strtol(up.c_str(), NULL, 0);

        // Get things ready to look at next level
        start = end + 1;
    }
}


topo_fattree::topo_fattree(ComponentId_t cid, Params& params, int num_ports, int rtr_id, int num_vns) :
    Topology(cid),
    id(rtr_id),
    num_ports(num_ports),
    num_vns(num_vns),
    num_vcs(-1)
{
    string shape = params.find<std::string>("shape");

    vns = new vn_info[num_vns];

    std::vector<std::string> vn_route_algos;
    if ( params.is_value_array("routing_alg") ) {
        params.find_array<std::string>("routing_alg", vn_route_algos);
        if ( vn_route_algos.size() != num_vns ) {
            fatal(CALL_INFO, -1, "ERROR: When specifying routing algorithms per VN, algorithm list length must match number of VNs (%d VNs, %lu algorithms).\n",num_vns,vn_route_algos.size());        
        }
    }
    else {
        std::string route_algo = params.find<std::string>("algorithm", "deterministic");
        for ( int i = 0; i < num_vns; ++i ) vn_route_algos.push_back(route_algo);
    }

    int curr_vc = 0;
    for ( int i = 0; i < num_vns; ++i ) {
        vns[i].start_vc = curr_vc;
        if ( vn_route_algos[i] == "adaptive" ) {
            vns[i].allow_adaptive = true;
            vns[i].num_vcs = 1;
        }
        else if ( vn_route_algos[i] == "deterministic" ) {
            vns[i].allow_adaptive = false;
            vns[i].num_vcs = 1;
        }
        else {
            output.fatal(CALL_INFO,-1,"Unknown routing mode specified: %s\n",vn_route_algos[i].c_str());
        }
        curr_vc += vns[i].num_vcs;
    }
    adaptive_threshold = params.find<double>("adaptive_threshold", 0.5);
    
    int levels = std::count(shape.begin(), shape.end(), ':') + 1;
    int* ups = new int[levels];
    int* downs= new int[levels];

    parseShape(shape, downs, ups);

    int total_hosts = 1;
    for ( int i = 0; i < levels; i++ ) {
        total_hosts *= downs[i];
    }

    int* routers_per_level = new int[levels];
    routers_per_level[0] = total_hosts / downs[0];

    for ( int i = 1; i < levels; i++ ) {
        routers_per_level[i] = routers_per_level[i-1] * ups[i-1] / downs[i];
    }

    int count = 0;
    rtr_level = -1;
    int routers_per_level_group = 1;
    for ( int i = 0; i < levels; i++ ) {
        int lid = id - count;
        count += routers_per_level[i];
        if ( id < count ) {
            rtr_level = i;
            level_id = lid;
            level_group = lid / routers_per_level_group;
            break;
        }
        routers_per_level_group *= ups[i];
    }

    down_ports = downs[rtr_level];
    up_ports = ups[rtr_level];
    
    // Compute reachable IDs
    int rid = 1;
    for ( int i = 0; i <= rtr_level; i++ ) {
        rid *= downs[i];
    }
    down_route_factor = rid / downs[rtr_level];

    low_host = level_group * rid;
    high_host = low_host + rid - 1;
    
}


topo_fattree::~topo_fattree()
{
    delete[] vns;
}

void topo_fattree::route_deterministic(int port, int vc, internal_router_event* ev)  {
    int dest = ev->getDest();
    // Down routes
    if ( dest >= low_host && dest <= high_host ) {
        ev->setNextPort((dest - low_host) / down_route_factor);
    }
    // Up routes
    else {
        ev->setNextPort(down_ports + ((dest/down_route_factor) % up_ports));
    }
}


void topo_fattree::route_packet(int port, int vc, internal_router_event* ev)
{
    route_deterministic(port,vc,ev);
    
    int dest = ev->getDest();
    // Down routes are always deterministic and are already done in route
    if ( dest >= low_host && dest <= high_host ) {
        return;
    }
    // Up routes can be adaptive, so things can change from the normal path
    else {
        int vn = ev->getVN();
        // If we're not adaptive, then we're already routed
        if ( !vns[vn].allow_adaptive ) return;

        // If the port we're supposed to be going to has a buffer with
        // fewer credits than the threshold, adaptively route
        int next_port = ev->getNextPort();
        int vc = ev->getVC();
        int index  = next_port*num_vcs + vc;
        if ( outputCredits[index] >= thresholds[index] ) return;
        
        // Send this on the least loaded port.  For now, just look at
        // current VC, later we may look at overall port loading.  Set
        // the max to be the "natural" port and only adaptively route
        // if it's not the best one (ties go to natural port)
        int max = outputCredits[index];
        // If all ports have zero credits left, then we just set
        // it to the port that it would normally go to without
        // adaptive routing.
        int port = next_port;
        for ( int i = (down_ports * num_vcs) + vc; i < num_ports * num_vcs; i += num_vcs ) {
            if ( outputCredits[i] > max ) {
                max = outputCredits[i];
                port = i / num_vcs;
            }
        }
        ev->setNextPort(port);
    }
}



internal_router_event* topo_fattree::process_input(RtrEvent* ev)
{
    internal_router_event* ire = new internal_router_event(ev);
    ire->setVC(ire->getVN());
    return ire;
}

void topo_fattree::routeInitData(int inPort, internal_router_event* ev, std::vector<int> &outPorts)
{

    if ( ev->getDest() == INIT_BROADCAST_ADDR ) {
        // Send to all my downports except the one it came from
        for ( int i = 0; i < down_ports; i++ ) {
            if ( i != inPort ) outPorts.push_back(i);
        }

        // If I'm not at the top level (no up_ports) an I didn't
        // receive this from an up_port, send to one up port
        if ( up_ports != 0 && inPort < down_ports ) {
            outPorts.push_back(down_ports+(inPort % up_ports));
        }
    }
    else {
        route_deterministic(inPort, 0, ev);
        outPorts.push_back(ev->getNextPort());
    }
}


internal_router_event* topo_fattree::process_InitData_input(RtrEvent* ev)
{
    return new internal_router_event(ev);
}

int
topo_fattree::getEndpointID(int port)
{
    return low_host + port;
}

Topology::PortState topo_fattree::getPortState(int port) const
{
    if ( rtr_level == 0 ) {
        if ( port < down_ports ) return R2N;
        else if ( port >= down_ports ) return R2R;
        else return UNCONNECTED;
    } else {
        return R2R;
    }
}

void topo_fattree::setOutputBufferCreditArray(int const* array, int vcs) {
        num_vcs = vcs;
        outputCredits = array;
        thresholds = new int[num_vcs * num_ports];
        for ( int i = 0; i < num_vcs * num_ports; i++ ) {
            thresholds[i] = outputCredits[i] * adaptive_threshold;
        }
        
}
