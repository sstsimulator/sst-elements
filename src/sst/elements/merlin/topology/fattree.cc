// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
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
    // cout << "levels: " << levels << endl;
    // cout << "shape: " << shape << endl;
    for ( int i = 0; i < levels; i++ ) {
        end = shape.find(':',start);
        size_t length = end - start;
        std::string sub = shape.substr(start,length);
        // cout << "sub: " << sub << endl;
        
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

topo_fattree::topo_fattree(Component* comp, Params& params) :
    Topology(comp),
    num_vcs(-1),
    allow_adaptive(false)
{
    num_ports = params.find<int>("num_ports");
    string shape = params.find<std::string>("fattree:shape");
    id = params.find<int>("id");

    string routing_alg = params.find<std::string>("fattree:routing_alg", "deterministic");
    if ( routing_alg == "adaptive" ) {
        allow_adaptive = true;
    }

    adaptive_threshold = params.find<double>("fattree:adaptive_threshold", 0.5);
    // std::cout << "routing_alg: " << routing_alg << std::endl;
    // std::cout << "adaptive_threshold: " << adaptive_threshold << std::endl;
    
    int levels = std::count(shape.begin(), shape.end(), ':') + 1;
    int* ups = new int[levels];
    int* downs= new int[levels];

    // cout << "shape: " << shape << endl;
    // cout << "levels: " << levels << endl;
    parseShape(shape, downs, ups);
    // for ( int i = 0; i < levels; i++ ) {
    //     cout << "Level " << i << ": down = " << downs[i] << ", up = " << ups[i] << endl;
    // }

    int total_hosts = 1;
    for ( int i = 0; i < levels; i++ ) {
        total_hosts *= downs[i];
    }
    // cout << "total_hosts = " << total_hosts << endl;

    int* routers_per_level = new int[levels];
    routers_per_level[0] = total_hosts / downs[0];

    for ( int i = 1; i < levels; i++ ) {
        routers_per_level[i] = routers_per_level[i-1] * ups[i-1] / downs[i];
    }

    // for ( int i = 0; i < levels; i++ ) {
    //     cout << "Level " << i << " routers = " << routers_per_level[i] << endl;
    // }

    int count = 0;
    rtr_level = -1;
    int routers_per_level_group = 1;
    for ( int i = 0; i < levels; i++ ) {
        int lid = id - count;
        count += routers_per_level[i];
        // cout << i << " " << count << " " << id << " " << lid << endl;
        if ( id < count ) {
            rtr_level = i;
            level_id = lid;
            level_group = lid / routers_per_level_group;
            break;
        }
        routers_per_level_group *= ups[i];
    }

    // cout << "my router level = " << rtr_level << ", level id = " << level_id <<
    //     ", level_group = " << level_group << endl;
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
    
    // cout << "low host = " << low_host << ", high host = " << high_host <<
    //     ", down_route_factor = " << down_route_factor << endl;

    // cout << "Routing Table:" << endl;
    // RtrEvent* rev = new RtrEvent();
    // internal_router_event* ev = new internal_router_event(rev);
    // for ( int i = 0; i < total_hosts; i++ ) {
    //     ev->getEncapsulatedEvent()->dest = i;
    //     route(0, 0, ev);
    //     cout << "  " << i << "   " << ev->getNextPort() << endl; 
    // }
}


topo_fattree::~topo_fattree()
{
}

void topo_fattree::route(int port, int vc, internal_router_event* ev)  {
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


void topo_fattree::reroute(int port, int vc, internal_router_event* ev)
{
    int dest = ev->getDest();
    // Down routes are always deterministic and are already done in route
    if ( dest >= low_host && dest <= high_host ) {
        return;
    }
    // Up routes can be adaptive, so things can change from the normal path
    else {
        // If we're not adaptive, then we're already routed
        if ( !allow_adaptive ) return;

        // If the port we're supposed to be going to has a buffer with
        // fewer credits than the threshold, adaptively route
        int next_port = ev->getNextPort();
        int vc = ev->getVC();
        int index  = next_port*num_vcs + vc;
        if ( outputCredits[index] >= thresholds[index] ) return;
        
        // std::cout << "Going to adaptively route" << std::endl;
        // std::cout << down_ports << ", " << num_vcs << ", " << vc << ", " << num_ports << std::endl;

        // Send this on the least loaded port.  For now, just look at
        // current VC, later we may look at overall port loading.  Set
        // the max to be the "natural" port and only adaptively route
        // if it's not the best one (ties go to natural port)
        int max = outputCredits[index];
        // If all ports have zero credits left, then we just set
        // it to the port that it would normally go to without
        // adaptive routing.
        // int port = down_ports + ((dest/down_route_factor) % up_ports);
        int port = next_port;
        for ( int i = (down_ports * num_vcs) + vc; i < num_ports * num_vcs; i += num_vcs ) {
            if ( outputCredits[i] > max ) {
                max = outputCredits[i];
                port = i / num_vcs;
                // std::cout << port << std::endl;
            }
        }
        // std::cout << "Port: " << port << std::endl;
        ev->setNextPort(port);
    }
    // cout << "id: " << id << ", dest = " << dest << ", next port = " << (down_ports + (dest % up_ports)) << endl;
}



internal_router_event* topo_fattree::process_input(RtrEvent* ev)
{
    internal_router_event* ire = new internal_router_event(ev);
    ire->setVC(ev->request->vn);
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
        route(inPort, 0, ev);
        outPorts.push_back(ev->getNextPort());
    }
    // cout << "routeInitData()" << endl;
    // if ( ev->getDest() == INIT_BROADCAST_ADDR ) {
    //     switch (rtr_level) {
    //     case 1:
    //         /* Downstream  - send to hosts*/
    //         for ( int port = 0 ; port < edge_loading ; port++ ) {
    //             if ( port != inPort )
    //                 outPorts.push_back(port);
    //         }

    //         if ( inPort < num_ports/2 ) {
    //             /* Upstream  - send to 1 upper-level router*/
    //             outPorts.push_back(num_ports/2);
    //         }
    //         break;
    //     case 2:
    //         if ( inPort < (num_ports/2) ) {
    //             /* came from Downstream, send to 1 upper-level router */
    //             outPorts.push_back(num_ports/2);
    //             for ( int port = 0 ; port < num_ports/2 ; port++ ) {
    //                 /* also, send downstream withing this router */
    //                 if ( port != inPort )
    //                     outPorts.push_back(port);
    //             }
    //         } else {
    //             /* came from Upstream */
    //             for ( int port = 0 ; port < num_ports/2 ; port++ ) {
    //                 outPorts.push_back(port);
    //             }
    //         }
    //         break;
    //     case 3:
    //         /* Send to all Downstream (except from incoming port) */
    //         for ( int port = 0 ; port < num_ports ; port++ ) {
    //             if ( port != inPort )
    //                 outPorts.push_back(port);
    //         }
    //         break;
    //     default:
    //         output.fatal(CALL_INFO, -1, "Bad level %d\n", rtr_level);
    //     }
    // } else {
    //     route(inPort, 0, ev);
    //     outPorts.push_back(ev->getNextPort());
    // }
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
        // std::cout << "id = " << id << std::endl;
        // for ( int i = 0; i < num_ports; i++ ) {
        //     for ( int j = 0; j < num_vcs; j++ ) {
        //         std::cout << "Port " << i << ", VC " << j << " credits = " << outputCredits[i*num_vcs+j] << std::endl;
        //     }
        // }
        thresholds = new int[num_vcs * num_ports];
        for ( int i = 0; i < num_vcs * num_ports; i++ ) {
            thresholds[i] = outputCredits[i] * adaptive_threshold;
        }
        
}
