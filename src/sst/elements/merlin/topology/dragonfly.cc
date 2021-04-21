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
//

#include <sst_config.h>
#include <sst/core/shared/sharedArray.h>
#include "sst/core/rng/xorshift.h"

#include "merlin.h"
#include "dragonfly.h"

#include <stdlib.h>
#include <sstream>

using namespace SST::Merlin;

// const uint8_t bit_array::masks[8] = { 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f };


void
RouteToGroup::init_write(const std::string& basename, int group_id, global_route_mode_t route_mode,
                         const dgnflyParams& params, const std::vector<int64_t>& global_link_map,
                         bool config_failed_links, const std::vector<FailedLink>& failed_links_vec)
{
    // Get a shared region
    data.initialize(basename+"group_to_global_port",
                    ((params.g-1) * params.n) * sizeof(RouterPortPair));


    groups = params.g;
    routers = params.a;
    slices = params.n;
    links = params.h;
    gid = group_id;
    global_start = params.p + params.a - 1;
    mode = route_mode;
    consider_failed_links = config_failed_links;

    // Fill in the data
    for ( int i = 0; i < global_link_map.size(); i++ ) {
        // Figure out all the mappings
        int64_t value = global_link_map[i];
        if ( value == -1 ) continue;

        int group = value % (params.g - 1);
        int route_num = value / (params.g - 1);
        int router = i / params.h;
        int port = (i % params.h) + params.p + params.a - 1;

        RouterPortPair rpp;
        rpp.router = router;
        rpp.port = port;
        // sr->modifyArray(group * params.n + route_num, rpp);
        data.write(group * params.n + route_num, rpp);
    }
    data.publish();
    // sr->publish();
    // data = sr->getPtr<const RouterPortPair*>();

    // If we're not doing failed links, we're done
    if ( !config_failed_links ) return;

    // For now, we only support the same number of links between
    // each pair of groups, so we'll start with that many and
    // subtract for failed links.  We'll build it all in a private
    // array and then copy it becuase multiple modification within
    // the shared region is inefficient at best and breaks at
    // worst.
    link_counts.initialize(basename + "group_link_counts",groups * groups,
                           params.n,Shared::SharedObject::NO_VERIFY);

    // Need to have a bit field where each element tells us whether or
    // not that global link is active.  Start at group 0, global link
    // 0 and count from there.
    size_t bf_size = groups * params.a * params.h;
    failed_links.initialize(basename + "downed_links", bf_size);


    // uint8_t* link_counts_ld = new uint8_t[groups * groups];
    // std::fill_n(link_counts_ld, groups * groups, params.n);

    // uint8_t* bf_down = static_cast<uint8_t*>(sr_dl->getRawPtr());
    // uint8_t* bf_down = new uint8_t[bf_size];
    // std::fill_n(bf_down, bf_size, 0);

    for ( auto x : failed_links_vec ) {
        if ( x.low_group >= groups || x.high_group >= groups || x.slice >= params.n ) {
            merlin_abort.fatal(CALL_INFO,1,"Illegal link specification: %d:%d:%d\n",x.low_group, x.high_group, x.slice);
        }
        link_counts.write(x.low_group * groups + x.high_group, link_counts[x.low_group * groups + x.high_group] - 1);
        link_counts.write(x.high_group * groups + x.low_group, link_counts[x.high_group * groups + x.low_group] - 1);

        const RouterPortPair& rp = getRouterPortPairForGroup(x.low_group, x.high_group, x.slice);
        int bit_index = (x.low_group * params.a * params.h) + (rp.router * params.h + rp.port - global_start);
        failed_links.write(bit_index,true);

        const RouterPortPair& rp2 = getRouterPortPairForGroup(x.high_group, x.low_group, x.slice);
        bit_index = (x.high_group * params.a * params.h) + (rp2.router * params.h + rp2.port - global_start);
        failed_links.write(bit_index,true);
    }

    // Publish the link counts
    link_counts.publish();

    // Published the down links data.  Already have a pointer to the
    // data.
    failed_links.publish();
}

void
RouteToGroup::init(const std::string& basename, int group_id, global_route_mode_t route_mode,
                   const dgnflyParams& params, bool config_failed_links)
{
    data.initialize(basename+"group_to_global_port");
    data.publish();

    groups = params.g;
    routers = params.a;
    slices = params.n;
    links = params.h;
    gid = group_id;
    global_start = params.p + params.a - 1;
    mode = route_mode;
    consider_failed_links = config_failed_links;

    if ( !config_failed_links ) return;

    // Get the shared regions and publish them.
    link_counts.initialize(basename + "group_link_counts");
    link_counts.publish();

    size_t bf_size = (groups * params.a * params.h + 7) / 8;
    failed_links.initialize(basename + "downed_links");
    failed_links.publish();
}



const RouterPortPair&
RouteToGroup::getRouterPortPair(int group, int route_number) const
{
    return getRouterPortPairForGroup(gid,group,route_number);
}

const RouterPortPair&
RouteToGroup::getRouterPortPairForGroup(uint32_t src_group, uint32_t dest_group, uint32_t slice) const
{
    // Look up global port to use
    switch ( mode ) {
    case ABSOLUTE:
        if ( dest_group >= src_group ) dest_group--;
        break;
    case RELATIVE:
        if ( dest_group > src_group ) {
            dest_group = dest_group - src_group - 1;
        }
        else {
            dest_group = groups - src_group + dest_group - 1;
        }
        break;
    default:
        break;
    }

    return data[dest_group*slices + slice];
}

int
RouteToGroup::getValiantGroup(int dest_group, RNG::SSTRandom* rng) const
{
    if ( !consider_failed_links )  {
        int group;
        do {
            group = rng->generateNextUInt32() % groups;
        } while ( group == gid || group == dest_group );
        return group;
    }

    // Need to worry about failed links
    uint32_t possible_mid_groups = groups - 2;

    uint8_t min_links;
    uint8_t req_links;
    uint32_t mid;
    do {
        // We need to generate two random numbers: mid_group plus a
        // random numbers that will determin how many links are needed
        // between groups in order to be considered a valid route.
        // This will weight the valiant groups based on how many
        // downed links there are along the path.  We compare against
        // the minimum link count between src->mid and mid->dest.
        uint32_t state = rng->generateNextUInt32() % (possible_mid_groups * slices);
        mid = state /  slices;
        req_links = state % slices + 1;

        // Need to adjust mid.  We don't consider the src or dest
        // group.
        if ( mid >= gid || mid >= dest_group ) mid++;
        if ( mid >= gid && mid >= dest_group ) mid++;

        // Need to get the lowest number of links between src->mid and
        // mid->dest and compare against the req_links
        min_links = getLinkCount(gid,mid);
        uint8_t links = getLinkCount(mid,dest_group);
        if ( links < min_links ) min_links = links;
    } while ( min_links < req_links );
    // printf("src_group = %d, dest_group = %d, mid_group = %d\n",gid,dest_group,mid);
    return mid;
}


// void
// RouteToGroup::setRouterPortPair(int group, int route_number, const RouterPortPair& pair) {
//     region->modifyArray(group*routes+route_number,pair);
// }


topo_dragonfly::topo_dragonfly(ComponentId_t cid, Params &p, int num_ports, int rtr_id, int num_vns) :
    Topology(cid),
    num_vns(num_vns),
    rtr_id(rtr_id)
{
    params.p = p.find<uint32_t>("hosts_per_router");
    params.a = p.find<uint32_t>("routers_per_group");
    params.k = num_ports;
    params.h = p.find<uint32_t>("intergroup_per_router");
    params.g = p.find<uint32_t>("num_groups");
    params.n = p.find<uint32_t>("intergroup_links");

    group_id = rtr_id / params.a;
    router_id = rtr_id % params.a;

    std::string global_route_mode_s = p.find<std::string>("global_route_mode","absolute");
    if ( global_route_mode_s == "absolute" ) global_route_mode = ABSOLUTE;
    else if ( global_route_mode_s == "relative" ) global_route_mode = RELATIVE;
    else {
        output.fatal(CALL_INFO, -1, "Invalid global_route_mode specified: %s.\n",global_route_mode_s.c_str());
    }

    vns = new vn_info[num_vns];
    std::vector<std::string> vn_route_algos;
    if ( p.is_value_array("algorithm") ) {
        p.find_array<std::string>("algorithm", vn_route_algos);
        if ( vn_route_algos.size() != num_vns ) {
            fatal(CALL_INFO, -1, "ERROR: When specifying routing algorithms per VN, algorithm list length must match number of VNs (%d VNs, %lu algorithms).\n",num_vns,vn_route_algos.size());
        }
    }
    else {
        std::string route_algo = p.find<std::string>("algorithm", "minimal");
        for ( int i = 0; i < num_vns; ++i ) vn_route_algos.push_back(route_algo);
    }

    adaptive_threshold = p.find<double>("adaptive_threshold",2.0);

    bool config_failed_links = p.find<bool>("config_failed_links","false");

    // Set up the RouteToGroup object

    if ( rtr_id == 0 ) {
        // Get the global link map
        std::vector<int64_t> global_link_map;
        p.find_array<int64_t>("global_link_map", global_link_map);

        std::vector<FailedLink> failed_links;
        p.find_array<FailedLink>("failed_links", failed_links);
        group_to_global_port.init_write("network_", group_id, global_route_mode, params, global_link_map,
                                        config_failed_links, failed_links);
    }
    else {
        group_to_global_port.init("network_", group_id, global_route_mode, params, config_failed_links);
    }

    // Setup the routing algorithms
    int curr_vc = 0;
    for ( int i = 0; i < num_vns; ++i ) {
        vns[i].start_vc = curr_vc;
        vns[i].bias = 50;
        if ( !vn_route_algos[i].compare("valiant") ) {
            if ( params.g <= 2 ) {
                /* 2 or less groups... no point in valiant */
                vns[i].algorithm = MINIMAL;
                vns[i].num_vcs = 2;
            } else {
                vns[i].algorithm = VALIANT;
                vns[i].num_vcs = 3;
            }
        }
        else if ( !vn_route_algos[i].compare("adaptive-local") ) {
            vns[i].algorithm = ADAPTIVE_LOCAL;
            vns[i].num_vcs = 3;
        }
        else if ( !vn_route_algos[i].compare("minimal") ) {
            vns[i].algorithm = MINIMAL;
            vns[i].num_vcs = 2;
        }
        else if ( !vn_route_algos[i].compare("ugal") ) {
            vns[i].algorithm = UGAL;
            vns[i].num_vcs = 3;
        }
        else if ( !vn_route_algos[i].compare("min-a") ) {
            vns[i].algorithm = MIN_A;
            vns[i].num_vcs = 2;
        }
        else {
            fatal(CALL_INFO_LONG,1,"ERROR: Unknown routing algorithm specified: %s\n",vn_route_algos[i].c_str());
        }
        curr_vc += vns[i].num_vcs;
    }

    rng = new RNG::XORShiftRNG(rtr_id+1);

    output.verbose(CALL_INFO, 1, 1, "%u:%u:  ID: %u   Params:  p = %u  a = %u  k = %u  h = %u  g = %u\n",
            group_id, router_id, rtr_id, params.p, params.a, params.k, params.h, params.g);
}


topo_dragonfly::~topo_dragonfly()
{
    delete[] vns;
}


void topo_dragonfly::route_nonadaptive(int port, int vc, internal_router_event* ev)
{
    topo_dragonfly_event *td_ev = static_cast<topo_dragonfly_event*>(ev);

    // Break this up by port type
    uint32_t next_port = 0;
    if ( (uint32_t)port < params.p ) {
        // Host ports

        if ( td_ev->dest.group == td_ev->src_group ) {
            // Packets stays within the group
            if ( td_ev->dest.router == router_id ) {
                // Stays within the router
                next_port = td_ev->dest.host;
            }
            else {
                // Route to the router specified by mid_group.  If
                // this is a direct route then mid_group will equal
                // router and packet will go direct.
                next_port = port_for_router(td_ev->dest.mid_group);
            }
        }
        else {
            // Packet is leaving group.  Simply route to group
            // specified by mid_group.  If this is a direct route then
            // mid_group will be set to group.
            next_port = port_for_group(td_ev->dest.mid_group, td_ev->global_slice);
        }
    }
    else if ( (uint32_t)port < ( params.p + params.a - 1) ) {
        // Intragroup links

        if ( td_ev->dest.group == group_id ) {
            // In final group
            if ( td_ev->dest.router == router_id ) {
                // In final router, route to host port
                next_port = td_ev->dest.host;
            }
            else {
                // This is a valiantly routed packet within a group.
                // Need to increment the VC and route to correct
                // router.
                td_ev->setVC(vc+1);
                next_port = port_for_router(td_ev->dest.router);
            }
        }
        else {
            // Not in correct group, should route out one of the
            // global links
            if ( td_ev->dest.mid_group != group_id ) {
                next_port = port_for_group(td_ev->dest.mid_group, td_ev->global_slice);
            } else {
                next_port = port_for_group(td_ev->dest.group, td_ev->global_slice);
            }
        }
    }
    else { // global
        /* Came in from another group.  Increment VC */
        td_ev->setVC(vc+1);
        if ( td_ev->dest.group == group_id ) {
            if ( td_ev->dest.router == router_id ) {
                // In final router, route to host port
                next_port = td_ev->dest.host;
            }
            else {
                // Go to final router
                next_port = port_for_router(td_ev->dest.router);
            }
        }
        else {
            // Just passing through on a valiant route.  Route
            // directly to final group
            next_port = port_for_group(td_ev->dest.group, td_ev->global_slice);
        }
    }


    output.verbose(CALL_INFO, 1, 1, "%u:%u, Recv: %d/%d  Setting Next Port/VC:  %u/%u\n", group_id, router_id, port, vc, next_port, td_ev->getVC());
    td_ev->setNextPort(next_port);
}

void topo_dragonfly::route_ugal(int port, int vc, internal_router_event* ev)
{
    topo_dragonfly_event *td_ev = static_cast<topo_dragonfly_event*>(ev);
    int vn = ev->getVN();

    // Need to determine where we are in the routing.  We can check
    // this based on input port

    // Input port
    if ( port < params.p ) {
        // Packet stays in group.
        if ( td_ev->dest.group == group_id ) {
            // Check to see if the dest is in the same router
            if ( td_ev->dest.router == router_id ) {
                td_ev->setNextPort(td_ev->dest.host);
                return;
            }

            // Adaptive routing when packet stays in group.
            // Check to see if we take the valiant route or direct route.

            int direct_route_port = port_for_router(td_ev->dest.router);
            int direct_route_weight = output_queue_lengths[direct_route_port * num_vcs + vc];

            int valiant_route_port = port_for_router(td_ev->dest.mid_group);
            int valiant_route_weight = output_queue_lengths[valiant_route_port * num_vcs + vc];

            if ( direct_route_weight <= 2 * valiant_route_weight + vns[vn].bias ) {
                td_ev->setNextPort(direct_route_port);
            }
            else {
                td_ev->setNextPort(valiant_route_port);
            }

            return;
        }

        // Packet leaves the group
        else {
            // printf("Routing packet with dest.group = %d and dest.mid_group = %d\n",td_ev->dest.group,td_ev->dest.mid_group);
            // Need to find the lowest weighted route.  Loop over all
            // the slices.
            int min_weight = std::numeric_limits<int>::max();
            std::vector<std::pair<int,int> > min_ports;
            for ( int i = 0; i < params.n; ++i ) {
                // Direct routes
                int weight;
                int port = port_for_group(td_ev->dest.group, i);
                if ( port != -1 ) {
                    weight = output_queue_lengths[port * num_vcs + vc];

                    if ( weight == min_weight ) {
                        min_ports.emplace_back(port,i);
                    }
                    else if ( weight < min_weight ) {
                        min_weight = weight;
                        min_ports.clear();
                        min_ports.emplace_back(port,i);
                    }
                }

                // Valiant routes
                port = port_for_group(td_ev->dest.mid_group, i);
                if ( port != -1 ) {
                    weight = 2 * output_queue_lengths[port * num_vcs + vc] + vns[vn].bias;

                    if ( weight == min_weight ) {
                        min_ports.emplace_back(port,i);
                    }
                    else if ( weight < min_weight ) {
                        min_weight = weight;
                        min_ports.clear();
                        min_ports.emplace_back(port,i);
                    }
                }
            }

            auto& route = min_ports[rng->generateNextUInt32() % min_ports.size()];
            td_ev->setNextPort(route.first);
            td_ev->global_slice = route.second;
            return;
        }
    }

    // Intragroup links
    else if ( port < ( params.p + params.a -1 ) ) {
        // In final group
        if ( td_ev->dest.group == group_id ) {
            if ( td_ev->dest.router == router_id ) {
                // In final router, route to host port
                td_ev->setNextPort(td_ev->dest.host);
                return;
            }
            else {
                // This is a valiantly routed packet within a group.
                // Need to increment the VC and route to correct
                // router.  We'll use VC2 to avoid interfering with
                // valiant traffic routing through the group.
                td_ev->setVC(vc+2);
                td_ev->setNextPort(port_for_router(td_ev->dest.router));
                return;
            }
        }
        // Need to route it over the global link
        else {
            if ( td_ev->dest.mid_group == group_id ) {
                // In valiant group, just route out to next group
                td_ev->setNextPort( port_for_group(td_ev->dest.group, td_ev->global_slice) );
                return;
            }

            // It's possible that there are links to both the valiant
            // and direct group on the same slice (we don't detect the
            // case where there are routes to both but on a different
            // slice).  We'll need to check both and if they both
            // exist take the port with the least weight.

            // Check direct route first
            int direct_port = port_for_group(td_ev->dest.group, td_ev->global_slice);
            int valiant_port = port_for_group(td_ev->dest.mid_group, td_ev->global_slice);

            // Need to see if these are global ports on this router.
            // If not, then we won't consider them.  At least one of
            // these will be a global port from this router.
            int min_port = std::numeric_limits<int>::max();
            if ( direct_port != -1 && is_port_global(direct_port) ) {
                min_port = direct_port;
            }

                if ( valiant_port != -1 && is_port_global(valiant_port) ) {
                if ( min_port == std::numeric_limits<int>::max() ) {
                    min_port = valiant_port;
                }
                else {
                    // Need to check weights
                    int direct_weight = output_queue_lengths[direct_port * num_vcs + vc];
                    int valiant_weight = 2 * output_queue_lengths[valiant_port * num_vcs + vc] + vn[vns].bias;
                    if ( direct_weight > valiant_weight ) {
                        min_port = valiant_port;
                    }
                }
            }
            td_ev->setNextPort(min_port);
            return;
        }
    }
    // Came in from global routes
    else {
        // Need to increment the VC
        vc++;
        td_ev->setVC(vc);

        // See if we are in the target group
        if ( td_ev->dest.group == group_id ) {
            if ( td_ev->dest.router == router_id ) {
                // Deliver to output port
                td_ev->setNextPort(td_ev->dest.host);
                return;
            }
            else {
                // Need to route to the dest router
                td_ev->setNextPort(port_for_router(td_ev->dest.router));
                return;
            }
        }

        // Just routing through.  Need to look at all possible routes
        // to the dest group and pick the lowest weighted route
        int min_weight = std::numeric_limits<int>::max();
        std::vector<std::pair<int,int> > min_ports;

        // Look through all routes.  If the port is in current router,
        // weight with 1, other weight with 2
        for ( int i = 0; i < params.n; ++i ) {
            int port = port_for_group(td_ev->dest.group, i);
            if ( port == -1 ) continue;
            int weight = output_queue_lengths[port * num_vcs + vc];

            if ( !is_port_global(port) ) weight *= 2;

            if ( weight == min_weight ) {
                min_ports.emplace_back(port,i);
            }
            else if ( weight < min_weight ) {
                min_weight = weight;
                min_ports.clear();
                min_ports.emplace_back(port,i);
            }
        }
        auto& route = min_ports[rng->generateNextUInt32() % min_ports.size()];
        td_ev->setNextPort(route.first);
        td_ev->global_slice = route.second;
        return;
    }

}

void topo_dragonfly::route_mina(int port, int vc, internal_router_event* ev)
{
    topo_dragonfly_event *td_ev = static_cast<topo_dragonfly_event*>(ev);
    int vn = ev->getVN();

    // Need to determine where we are in the routing.  We can check
    // this based on input port

    // Input port
    if ( port < params.p ) {
        // Packet stays in group.
        if ( td_ev->dest.group == group_id ) {

            // Check to see if the dest is in the same router
            if ( td_ev->dest.router == router_id ) {
                td_ev->setNextPort(td_ev->dest.host);
                return;
            }

            // No adaptive routing, just route to destination router
            td_ev->setNextPort(port_for_router(td_ev->dest.router));
            return;
        }

        // Packet leaves the group
        else {
            // Need to find the lowest weighted route.  Loop over all
            // the slices, looking only at minimal routes.  For now,
            // just weight all paths equally.
            int min_weight = std::numeric_limits<int>::max();
            std::vector<std::pair<int,int> > min_ports;
            for ( int i = 0; i < params.n; ++i ) {

                // Direct routes
                int weight;
                int port = port_for_group(td_ev->dest.group, i);
                if ( port != -1 ) {
                    int hops = hops_to_router(td_ev->dest.group, td_ev->dest.router, i);
                    // Weight by hop count, thus favoring shorter
                    // paths.  The "+ hops" on the end is to make
                    // shorter paths win ties.
                    weight = hops * output_queue_lengths[port * num_vcs + vc] + hops;

                    if ( weight == min_weight ) {
                        min_ports.emplace_back(port,i);
                    }
                    else if ( weight < min_weight ) {
                        min_weight = weight;
                        min_ports.clear();
                        min_ports.emplace_back(port,i);
                    }
                }
            }

            auto& route = min_ports[rng->generateNextUInt32() % min_ports.size()];
            td_ev->setNextPort(route.first);
            td_ev->global_slice = route.second;
            return;
        }
    }

    // Intragroup links
    else if ( port < ( params.p + params.a -1 ) ) {
        // In final group
        if ( td_ev->dest.group == group_id ) {
            if ( td_ev->dest.router == router_id ) {
                // In final router, route to host port
                td_ev->setNextPort(td_ev->dest.host);
                return;
            }
            else {
                // Shouldn't happen with minimal traffic
                merlin_abort.fatal(CALL_INFO,1,"INTERNAL ERROR: routing error with min-a routing.");
            }
        }
        // Need to route it over the global link
        else {
            // Find the route.  We stored the global slice when
            // initially routing, so this should be a global link.
            td_ev->setNextPort(port_for_group(td_ev->dest.group, td_ev->global_slice));
            return;
        }
    }
    // Came in from global routes
    else {
        // Need to increment the VC
        vc++;
        td_ev->setVC(vc);

        // See if we are in the target group
        if ( td_ev->dest.group == group_id ) {
            if ( td_ev->dest.router == router_id ) {
                // Deliver to output port
                td_ev->setNextPort(td_ev->dest.host);
                return;
            }
            else {
                // Need to route to the dest router
                td_ev->setNextPort(port_for_router(td_ev->dest.router));
                return;
            }
        }
        else {
            // Shouldn't happen for minimal routing
            merlin_abort.fatal(CALL_INFO,1,"INTERNAL ERROR: routing error with min-a routing.");
        }
    }
    return;

}

void topo_dragonfly::route_adaptive_local(int port, int vc, internal_router_event* ev)
{
    int vn = ev->getVN();
    if ( vns[vn].algorithm != ADAPTIVE_LOCAL ) return;

    // For now, we make the adaptive routing decision only at the
    // input to the network and at the input to a group for adaptively
    // routed packets
    if ( port >= params.p && port < (params.p + params.a-1) ) return;


    topo_dragonfly_event *td_ev = static_cast<topo_dragonfly_event*>(ev);

    // Adaptive routing when packet stays in group
    if ( port < params.p && td_ev->dest.group == group_id ) {
        // If we're at the correct router, no adaptive needed
        if ( td_ev->dest.router == router_id) return;

        int direct_route_port = port_for_router(td_ev->dest.router);
        int direct_route_credits = output_credits[direct_route_port * num_vcs + vc];

        int valiant_route_port = port_for_router(td_ev->dest.mid_group);
        int valiant_route_credits = output_credits[valiant_route_port * num_vcs + vc];

        if ( valiant_route_credits > (int)((double)direct_route_credits * adaptive_threshold) ) {
            td_ev->setNextPort(valiant_route_port);
        }
        else {
            td_ev->setNextPort(direct_route_port);
        }

        return;
    }

    // If the dest is in the same group, no need to adaptively route
    if ( td_ev->dest.group == group_id ) return;


    // Based on the output queue depths, choose minimal or valiant
    // route.  We'll chose the direct route unless the remaining
    // output credits for the direct route is half of the valiant
    // route.  We'll look at two slice for each direct and indirect,
    // giving us a total of 4 routes we are looking at.  For packets
    // which came in adaptively on global links, look at two direct
    // routes and chose between the.
    int direct_slice1 = td_ev->global_slice_shadow;
    // int direct_slice2 = td_ev->global_slice;
    int direct_slice2 = (td_ev->global_slice_shadow + 1) % params.n;
    int direct_route_port1 = port_for_group(td_ev->dest.group, direct_slice1, 0 );
    int direct_route_port2 = port_for_group(td_ev->dest.group, direct_slice2, 1 );
    int direct_route_credits1 = output_credits[direct_route_port1 * num_vcs + vc];
    int direct_route_credits2 = output_credits[direct_route_port2 * num_vcs + vc];
    int direct_slice;
    int direct_route_port;
    int direct_route_credits;
    if ( direct_route_credits1 > direct_route_credits2 ) {
        direct_slice = direct_slice1;
        direct_route_port = direct_route_port1;
        direct_route_credits = direct_route_credits1;
    }
    else {
        direct_slice = direct_slice2;
        direct_route_port = direct_route_port2;
        direct_route_credits = direct_route_credits2;
    }

    int valiant_slice = 0;
    int valiant_route_port = 0;
    int valiant_route_credits = 0;

    if ( port >= (params.p + params.a-1) ) {
        // Global port, no indirect routes.  Set credits negative so
        // it will never get chosen
        valiant_route_credits = -1;
    }
    else {
        int valiant_slice1 = td_ev->global_slice;
        // int valiant_slice2 = td_ev->global_slice;
        int valiant_slice2 = (td_ev->global_slice + 1) % params.n;
        int valiant_route_port1 = port_for_group(td_ev->dest.mid_group_shadow, valiant_slice1, 2 );
        int valiant_route_port2 = port_for_group(td_ev->dest.mid_group_shadow, valiant_slice2, 3 );
        int valiant_route_credits1 = output_credits[valiant_route_port1 * num_vcs + vc];
        int valiant_route_credits2 = output_credits[valiant_route_port2 * num_vcs + vc];
        if ( valiant_route_credits1 > valiant_route_credits2 ) {
            valiant_slice = valiant_slice1;
            valiant_route_port = valiant_route_port1;
            valiant_route_credits = valiant_route_credits1;
        }
        else {
            valiant_slice = valiant_slice2;
            valiant_route_port = valiant_route_port2;
            valiant_route_credits = valiant_route_credits2;
        }
    }


    if ( valiant_route_credits > (int)((double)direct_route_credits * adaptive_threshold) ) { // Use valiant route
        td_ev->dest.mid_group = td_ev->dest.mid_group_shadow;
        td_ev->setNextPort(valiant_route_port);
        td_ev->global_slice = valiant_slice;
    }
    else { // Use direct route
        td_ev->dest.mid_group = td_ev->dest.group;
        td_ev->setNextPort(direct_route_port);
        td_ev->global_slice = direct_slice;
    }

}

void topo_dragonfly::route_packet(int port, int vc, internal_router_event* ev) {
    int vn = ev->getVN();
    if ( vns[vn].algorithm == UGAL ) return route_ugal(port,vc,ev);
    if ( vns[vn].algorithm == MIN_A ) return route_mina(port,vc,ev);
    route_nonadaptive(port,vc,ev);
    route_adaptive_local(port,vc,ev);
}

internal_router_event* topo_dragonfly::process_input(RtrEvent* ev)
{
    dgnflyAddr dstAddr = {0, 0, 0, 0};
    idToLocation(ev->getDest(), &dstAddr);
    int vn = ev->getRouteVN();

    switch (vns[vn].algorithm) {
    case MINIMAL:
    case MIN_A:
        if ( dstAddr.group == group_id ) {
            dstAddr.mid_group = dstAddr.router;
        }
        else {
            dstAddr.mid_group = dstAddr.group;
        }
        break;
    case VALIANT:
    case ADAPTIVE_LOCAL:
    case UGAL:
        if ( dstAddr.group == group_id ) {
            // staying within group, set mid_group to be an intermediate router within group
            do {
                dstAddr.mid_group = rng->generateNextUInt32() % params.a;
                // dstAddr.mid_group = router_id;
            }
            while ( dstAddr.mid_group == router_id );
            // dstAddr.mid_group = dstAddr.group;
        } else {
            dstAddr.mid_group = group_to_global_port.getValiantGroup(dstAddr.group, rng);
            // do {
            //     dstAddr.mid_group = rng->generateNextUInt32() % params.g;
            //     // dstAddr.mid_group = rand() % params.g;
            // } while ( dstAddr.mid_group == group_id || dstAddr.mid_group == dstAddr.group );
        }
        break;
    }
    dstAddr.mid_group_shadow = dstAddr.mid_group;

    topo_dragonfly_event *td_ev = new topo_dragonfly_event(dstAddr);
    td_ev->src_group = group_id;
    td_ev->setEncapsulatedEvent(ev);
    td_ev->setVC(vns[vn].start_vc);
    td_ev->global_slice = ev->getTrustedSrc() % params.n;
    td_ev->global_slice_shadow = ev->getTrustedSrc() % params.n;

    if ( td_ev->getTraceType() != SST::Interfaces::SimpleNetwork::Request::NONE ) {
        output.output("TRACE(%d): process_input():"
                      " mid_group_shadow = %u\n",
                      td_ev->getTraceID(),
                      td_ev->dest.mid_group_shadow);
    }
    return td_ev;
}

std::pair<int,int>
topo_dragonfly::getDeliveryPortForEndpointID(int ep_id)
{
    return std::make_pair<int,int>(ep_id / params.p, ep_id % params.p);
}


int
topo_dragonfly::routeControlPacket(CtrlRtrEvent* ev)
{
    const auto& dest = ev->getDest();

    int dest_id = dest.addr;

    // Check to see if this event is for the current router. If so,
    // return -1/
    if ( dest.addr_is_router ) {
        if ( dest_id == rtr_id ) return -1;
        // If this is a router ID, multiply by number of hosts per
        // router (params.p) in order to get the id for an endpoint in
        // this router.  Then we can use the idToLocation() function.
        else dest_id *= params.p;
    }
    // Addr is an endpoint id, check to see if the event is actually
    // for a router and, if so, if it is for the current router
    else if ( dest.addr_for_router && ((dest_id / params.p) == rtr_id) ) {
        return -1;
    }

    dgnflyAddr addr;
    idToLocation(dest_id,&addr);

    // Just get next port on minimal route
    int next_port;
    if ( addr.group != group_id ) {
        next_port = port_for_group(addr.group, 0 /* global slice */);
    }
    else if ( addr.router != router_id ) {
        next_port = port_for_router(addr.router);
    }
    else {
        next_port = addr.host;
    }
    return next_port;


}



void topo_dragonfly::routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts)
{
    bool broadcast_to_groups = false;
    topo_dragonfly_event *td_ev = static_cast<topo_dragonfly_event*>(ev);
    if ( td_ev->dest.host == (uint32_t)INIT_BROADCAST_ADDR ) {
        if ( (uint32_t)port >= (params.p + params.a-1) ) {
            /* Came in from another group.
             * Send to locals, and other routers in group
             */
            for ( uint32_t p  = 0 ; p < (params.p + params.a-1) ; p++ ) {
                outPorts.push_back((int)p);
            }
        } else if ( (uint32_t)port >= params.p ) {
            /* Came in from another router in group.
             * send to hosts
             * if this is the source group, send to other groups
             */
            for ( uint32_t p = 0 ; p < params.p ; p++ ) {
                outPorts.push_back((int)p);
            }
            if ( td_ev->src_group == group_id ) {
                broadcast_to_groups = true;
            }
        } else {
            /* Came in from a host
             * Send to all other hosts and routers in group, and all groups
             */
            // for ( int p = 0 ; p < (int)params.k ; p++ ) {
            for ( int p = 0 ; p < (int)(params.p + params.a - 1) ; p++ ) {
                if ( p != port )
                    outPorts.push_back((int)p);
            }
            broadcast_to_groups = true;
        }

        if ( broadcast_to_groups ) {
            for ( int p = 0; p < (int)(params.g - 1); p++ ) {
                const RouterPortPair& pair = group_to_global_port.getRouterPortPair(p,0);
                if ( pair.router == router_id ) outPorts.push_back((int)(pair.port));
            }
        }
    } else {
        // Not all data structures used for routing during run are
        // initialized yet, so we need to just do a quick minimal
        // routing scheme for init.
        // route(port, 0, ev);

        // Minimal Route
        // printf("Routing from source %d to dest %d.  In group %d, router %d\n",td_ev->getSrc(),td_ev->getDest(),group_id,router_id);
        int next_port;
        if ( td_ev->dest.group != group_id ) {
            next_port = port_for_group_init(td_ev->dest.group, td_ev->global_slice);
        }
        else if ( td_ev->dest.router != router_id ) {
            next_port = port_for_router(td_ev->dest.router);
        }
        else {
            next_port = td_ev->dest.host;
        }
        // printf("next_port = %d\n",next_port);
        outPorts.push_back(next_port);
    }

}


internal_router_event* topo_dragonfly::process_InitData_input(RtrEvent* ev)
{
    dgnflyAddr dstAddr;
    idToLocation(ev->getDest(), &dstAddr);
    topo_dragonfly_event *td_ev = new topo_dragonfly_event(dstAddr);
    td_ev->src_group = group_id;
    td_ev->setEncapsulatedEvent(ev);

    return td_ev;
}





Topology::PortState topo_dragonfly::getPortState(int port) const
{
    if ( is_port_endpoint(port) ) return R2N;
    else if ( is_port_local_group(port) ) return R2R;
    else {
        // These are global ports, see if they are failed
        if ( group_to_global_port.isFailedPort(RouterPortPair(router_id,port)) ) return FAILED;
        else return R2R;
    }
}

std::string topo_dragonfly::getPortLogicalGroup(int port) const
{
    if ( (uint32_t)port < params.p ) return "host";
    if ( (uint32_t)port >= params.p && (uint32_t)port < (params.p + params.a - 1) ) return "group";
    else return "global";
}

int
topo_dragonfly::getEndpointID(int port)
{
    return (group_id * (params.a /*rtr_per_group*/ * params.p /*hosts_per_rtr*/)) +
        (router_id * params.p /*hosts_per_rtr*/) + port;
}

void
topo_dragonfly::setOutputBufferCreditArray(int const* array, int vcs)
{
    output_credits = array;
    num_vcs = vcs;
}

void
topo_dragonfly::setOutputQueueLengthsArray(int const* array, int vcs)
{
    output_queue_lengths = array;
    num_vcs = vcs;
}

void topo_dragonfly::idToLocation(int id, dgnflyAddr *location)
{
    if ( id == INIT_BROADCAST_ADDR) {
        location->group = (uint32_t)INIT_BROADCAST_ADDR;
        location->mid_group = (uint32_t)INIT_BROADCAST_ADDR;
        location->router = (uint32_t)INIT_BROADCAST_ADDR;
        location->host = (uint32_t)INIT_BROADCAST_ADDR;
    } else {
        uint32_t hosts_per_group = params.p * params.a;
        location->group = id / hosts_per_group;
        location->router = (id % hosts_per_group) / params.p;
        location->host = id % params.p;
    }
}


int32_t topo_dragonfly::router_to_group(uint32_t group)
{

    if ( group < group_id ) {
        return group / params.h;
    } else if ( group > group_id ) {
        return (group-1) / params.h;
    } else {
        output.fatal(CALL_INFO, -1, "Trying to find router to own group.\n");
        return 0;
    }
}


int32_t topo_dragonfly::hops_to_router(uint32_t group, uint32_t router, uint32_t slice)
{
    int hops = 1;
    const RouterPortPair& pair = group_to_global_port.getRouterPortPair(group,slice);
    if ( pair.router != router_id ) hops++;
    const RouterPortPair& pair2 = group_to_global_port.getRouterPortPairForGroup(group, group_id, slice);
    if ( pair2.router != router ) hops++;
    return hops;
}

/* returns local router port if group can't be reached from this router */
int32_t topo_dragonfly::port_for_group(uint32_t group, uint32_t slice, int id)
{
    const RouterPortPair& pair = group_to_global_port.getRouterPortPair(group,slice);
    if ( group_to_global_port.isFailedPort(pair) ) {
        // printf("******** Skipping failed port ********\n");
        return -1;
    }

    if ( pair.router == router_id ) {
        return pair.port;
    } else {
        return port_for_router(pair.router);
    }
}

// Always ignore failed links during init
int32_t topo_dragonfly::port_for_group_init(uint32_t group, uint32_t slice)
{
    // TraceFunction trace(CALL_INFO_LONG);
    // trace.output("Routing from group %d to group %u over slice %u\n",group_id,group,slice);
    const RouterPortPair& pair = group_to_global_port.getRouterPortPair(group,slice);
    // trace.output("This maps to router %d and port %d\n",pair.router,pair.port);

    if ( pair.router == router_id ) {
        return pair.port;
    } else {
        return port_for_router(pair.router);
    }
}


int32_t topo_dragonfly::port_for_router(uint32_t router)
{
    uint32_t tgt = params.p + router;
    if ( router > router_id ) tgt--;
    return tgt;
}
