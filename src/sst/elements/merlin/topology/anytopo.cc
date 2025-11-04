#include <sst_config.h>
#include "anytopo.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdlib.h>
#include <set>
#include "sst/core/rng/xorshift.h"
#include "sst/core/output.h"
#include "../interfaces/ExtendedRequest.h"
// #include "../interfaces/endpointNIC/SourceRouting.h"

namespace SST {
namespace Merlin {

std::vector<routing_entries> topo_any::simple_routing_table;

// Forward declaration of helper function
static std::vector<std::vector<std::string>> parse_param_entries(const std::string& param_str, char entry_delim = ';', char token_delim = ',');

topo_any::topo_any(ComponentId_t cid, Params& params, int num_ports, int rtr_id, int num_vns):
    Topology(cid), router_id(rtr_id), num_vns(num_vns), output(getSimulationOutput()){

    num_routers = params.find<int>("num_routers", 1);
    num_R2N_ports = params.find<int>("num_R2N_ports", 1);
    num_R2R_ports = params.find<int>("num_R2R_ports", 1);
    if (num_R2R_ports != num_ports - num_R2N_ports){
        fatal(CALL_INFO, -1, "Number of ports mismatch");
    }

    vns = new vn_info[num_vns];

    // Read parameters for vn_info
    std::vector<std::string> vn_routing_mode = read_array_param<std::string>(params, "routing_mode", num_vns, "source_routing");
    std::vector<int> vcs_per_vn = read_array_param<int>(params, "vcs_per_vn", num_vns, default_vcs_per_vn);
    std::vector<int> vn_ugal_bias = read_array_param<int>(params, "vn_ugal_bias", num_vns, 50);
    std::vector<std::string> vn_dest_tag_algo = read_array_param<std::string>(params, "dest_tag_routing_algo", num_vns, "none");

    // Set up VN info
    int start_vc = 0;
    for (int i = 0; i < num_vns; ++i) {
        vns[i].routing_mode = (vn_routing_mode[i] == "source_routing") ? source_routing : dest_tag_routing;
        vns[i].vn_dest_tag_routing_algo = none;
        vns[i].start_vc = start_vc;
        vns[i].num_vcs = vcs_per_vn[i];
        tot_num_vcs += vcs_per_vn[i];
        vns[i].ugal_bias = vn_ugal_bias[i];
        start_vc += vcs_per_vn[i];

        // If destination tag routing , set the algorithm
        if (vns[i].routing_mode == dest_tag_routing) {
            if (vn_dest_tag_algo[i] == "nonadaptive") vns[i].vn_dest_tag_routing_algo = nonadaptive;
            else if (vn_dest_tag_algo[i] == "adaptive") vns[i].vn_dest_tag_routing_algo = adaptive;
            else if (vn_dest_tag_algo[i] == "ECMP") vns[i].vn_dest_tag_routing_algo = ECMP;
            else if (vn_dest_tag_algo[i] == "UGAL") vns[i].vn_dest_tag_routing_algo = UGAL;
            else fatal(CALL_INFO, -1, "ERROR: Invalid destination tag routing algorithm: %s\n", vn_dest_tag_algo[i].c_str());
        }
    }

    // Initialize RNG
    rng = new RNG::XORShiftRNG(rtr_id+1);

    if (simple_routing_table.empty()) {
        simple_routing_table.resize(num_routers);
    }

    Parse_routing_info(params);

    params.find_map<int, int>("endpoint_to_port_map", endpoint_to_port_map);
    if (endpoint_to_port_map.empty()) {
        output.fatal(CALL_INFO, -1, "No endpoint-to-port mapping provided\n");
    }

    params.find_map<int, int>("port_to_endpoint_map", port_to_endpoint_map);
    if (port_to_endpoint_map.empty()) {
        output.fatal(CALL_INFO, -1, "No port-to-endpoint mapping provided\n");
    }

    output.setVerboseLevel(params.find<int>("verbose_level", 0));
}

void SST::Merlin::topo_any::Parse_routing_info(SST::Params &params)
{
    // Read and parse connectivity information from parameter
    // Syntax: "dest router id, port id 1, port id 2; dest router id, ..."
    std::string connectivity_str = params.find<std::string>("connectivity", "");
    if (!connectivity_str.empty())
    {
        auto entries = parse_param_entries(connectivity_str);
        for (const auto& tokens : entries) {
            if (tokens.size() >= 2) {
                try {
                    int dest_router = std::stoi(tokens[0]);
                    std::set<int> port_set;
                    for (size_t i = 1; i < tokens.size(); ++i) {
                        port_set.insert(std::stoi(tokens[i]));
                    }
                    connectivity[dest_router] = port_set;
                }
                catch (const std::exception &e) {
                    output.output("WARNING: Failed to parse connectivity entry: %s\n", tokens[0].c_str());
                }
            }
        }
    }
    else
    {
        fatal(CALL_INFO, -1, "ERROR: the 'connectivity' map is not found in the parameters for anytopo");
    }

    // Read and parse the simple source routing table for untimed routing
    // Try string format first
    std::string routing_str = params.find<std::string>("simple_routing_entry_string", "");

    if (!routing_str.empty()) {
        output.verbose(CALL_INFO, 1, 0, "Loading routing entry from string parameter\n");
        routing_entries entries = SourceRoutingPlugin::parseRoutingEntryFromString(routing_str, num_routers, output);
        simple_routing_table[router_id] = entries;
        output.verbose(CALL_INFO, 1, 0, "Loaded simple routing table entry with %zu destinations\n", entries.size());
    } else { // Maybe other methods to parse routing entries in future
        output.output("Warning: No simple routing entry provided. Source routing on the untimed packets will not function.\n");
        return;
    }

    // Read and parse routing table information if dest_tag_routing is used
    // Syntax: "dest router id, next hop id_1, next hop id_2; dest router id, next hop id_1, next hop id_2; ..."
    bool needs_dest_tag_routing_table = false;
    for (int i = 0; i < num_vns; ++i)
    {
        if (vns[i].routing_mode == dest_tag_routing)
        {
            needs_dest_tag_routing_table = true;
            break;
        }
    }
    if (needs_dest_tag_routing_table)
    {
        std::string routing_table_str = params.find<std::string>("routing_table", "");
        if (!routing_table_str.empty())
        {
            auto entries = parse_param_entries(routing_table_str);
            for (const auto& tokens : entries) {
                if (tokens.size() >= 2) {
                    try {
                        int dest_router = std::stoi(tokens[0]);
                        std::vector<int> next_hops;
                        for (size_t i = 1; i < tokens.size(); ++i) {
                            next_hops.push_back(std::stoi(tokens[i]));
                        }
                        Dest_Tag_Routing_Table[dest_router] = next_hops;
                    }
                    catch (const std::exception &e) {
                        output.output("WARNING: Failed to parse routing table entry: %s\n", tokens[0].c_str());
                    }
                }
            }
        }
        else
        {
            output.output("WARNING: dest_tag_routing is enabled but no routing_table parameter provided\n");
        }
    }
}


// Helper function to parse a delimited parameter string into a vector of vector of strings
static std::vector<std::vector<std::string>> parse_param_entries(const std::string& param_str, char entry_delim, char token_delim) {
    std::vector<std::vector<std::string>> entries;
    std::istringstream param_stream(param_str);
    std::string entry;
    while (std::getline(param_stream, entry, entry_delim)) {
        // Trim whitespace
        entry.erase(0, entry.find_first_not_of(" \t"));
        entry.erase(entry.find_last_not_of(" \t") + 1);
        if (!entry.empty()) {
            std::istringstream entry_stream(entry);
            std::string token;
            std::vector<std::string> tokens;
            while (std::getline(entry_stream, token, token_delim)) {
                token.erase(0, token.find_first_not_of(" \t"));
                token.erase(token.find_last_not_of(" \t") + 1);
                if (!token.empty()) {
                    tokens.push_back(token);
                }
            }
            if (!tokens.empty()) {
                entries.push_back(tokens);
            }
        }
    }
    return entries;
}

topo_any::~topo_any() {
    delete[] vns;
    delete rng;
}

void topo_any::setOutputBufferCreditArray(int const* array, int vcs) {
    output_credits      = array;
    assert(vcs==tot_num_vcs);
}

void topo_any::setOutputQueueLengthsArray(int const* array, int vcs) {
    output_queue_lengths = array;
    assert(vcs==tot_num_vcs);
}

Topology::PortState topo_any::getPortState(int port_id) const {
    if (port_id < num_R2R_ports) {
        return R2R;
    } else if (port_id < num_R2R_ports + num_R2N_ports) {
        return R2N;
    } else{
        return UNCONNECTED;
    }
}

//===============================================================
// The following methods now assume one port per endpoint.
// In case of multiple ports per endpoint, a mapping is then needed.

int topo_any::getEndpointID(int port_id) {
    if (port_id < num_R2R_ports) {
        assert(!Topology::isHostPort(port_id));
        return -1;
    }else{
        return port_to_endpoint_map.at(port_id);
        // return router_id * num_R2N_ports + (port_id - num_R2R_ports);
    }
}

int topo_any::get_router_id(int EP_id) const
{
    // Use the shared endpoint-to-router mapping from SourceRoutingPlugin
    auto& ep_map = SourceRoutingPlugin::endpoint_to_router_map;
    auto it = ep_map.find(EP_id);
    if (it != ep_map.end()) {
        return it->second;
    }else {
        fatal(CALL_INFO, -1, "WARNING: Endpoint %d not found in endpoint_to_router_map.", EP_id);
    }
}

int topo_any::get_dest_local_port(int dest_EP_id) const
{
    return endpoint_to_port_map.at(dest_EP_id);
}

//===============================================================

int topo_any::getPortToRouter(int target_router_id) const {
    // for now ports are randomly selected
    std::set<int> ports = getPortsToRouter(target_router_id);
    int idx = rng->generateNextInt32() % ports.size();
    auto it = ports.begin();
    std::advance(it, idx);
    return *it;
}

std::set<int>& topo_any::getPortsToRouter(int target_router_id) const {
    auto it = connectivity.find(target_router_id);
    if (it == connectivity.end()) {
        fatal(CALL_INFO, -1, "ERROR: Router %d is not directly connected to this router %d\n", target_router_id, this->router_id);
    }
    return const_cast<std::set<int>&>(it->second);
}

internal_router_event* topo_any::process_input(RtrEvent* ev) {
    int vn = ev->getRouteVN();
    topo_any_event * returned_ev = new topo_any_event();
    returned_ev->setEncapsulatedEvent(ev);
    returned_ev->setVC(vns[vn].start_vc);
    return returned_ev;
}

void topo_any::route_packet(int input_port, int vc, internal_router_event* ev) {
    if (vns[ev->getVN()].routing_mode == source_routing) {
        route_packet_SR(static_cast<topo_any_event*>(ev));
    } else {
        route_packet_dest_tag(input_port, vc, static_cast<topo_any_event*>(ev));
    }
}

void topo_any::route_untimed_packet(int input_port, int vc, internal_router_event* ev) {
    // For untimed packets, always use simple routing table
    route_simple(static_cast<topo_any_event*>(ev));
}

void topo_any::route_simple(topo_any_event* ev) {
    std::deque<int> sr_path;

    int dest_EP_id = ev->getDest();
    int dest_router = get_router_id(dest_EP_id);

    // Look up the simple routing table
    if (dest_router == router_id){
        // Destination is local, use empty path
        sr_path = {};
    }
    else {
        // Use the first path (simple routing doesn't have weighted selection)
        sr_path = simple_routing_table[router_id][dest_router][0].second;
        if (sr_path.empty()) {
            fatal(CALL_INFO, -1, "ERROR: No valid routing path found from router %d to router %d in simple routing table\n",
                  router_id, dest_router);
        }
        output.verbose(CALL_INFO, 2, 0,
            "Using simple routing table for untimed packet from router %d to router %d\n",
            router_id, dest_router);
    }

    // Route the packet based on the path
    int fwd_port = -1;
    if (sr_path.empty() || sr_path.back() == router_id) {
        // Arriving at destination router, forward to endpoint

        int dest_EP_id = ev->getDest();
        if (get_router_id(dest_EP_id) != router_id) {
            fatal(CALL_INFO, -1, "ERROR: destination endpoint %d is not contained within this router %d\n",
                dest_EP_id, router_id);
        }
        ev->next_router_id = -1; // no next router
        fwd_port = get_dest_local_port(dest_EP_id);
    } else {
        // Intermediate hop - determine the next router and forward to the corresponding port
        // assert that there is a front element
        assert(!sr_path.empty());
        ev->next_router_id = sr_path.front(); // update next router id and increment the hop count
        ev->setVC(ev->num_hops);
        fwd_port = getPortToRouter(ev->next_router_id);
    }

    if (fwd_port == -1) {
        fatal(CALL_INFO, -1, "ERROR: No port found to forward to router %d\n", ev->next_router_id);
    }
    ev->setNextPort(fwd_port);
}

void topo_any::route_packet_SR(topo_any_event* ev) {
    // For source routing, the path should already be in the encapsulated event's metadata
    // For untimed packets (during init/complete phase), fall back to simple_routing_table

    // Inspect the request from the encapsulated event
    auto req = ev->inspectRequest();
    if (req == nullptr) {
        fatal(CALL_INFO, -1, "ERROR: Source routing event missing encapsulated request\n");
    }

    // Try to get the request as ExtendedRequest and extract path from metadata
    ExtendedRequest* ext_req = dynamic_cast<ExtendedRequest*>(req);

    if (ext_req) {

        // Try to get pointer to metadata without copying
        SourceRoutingMetadata* sr_meta_ptr = ext_req->getMetadataPtr<SourceRoutingMetadata>("SourceRouting");
        std::deque<int>* sr_path;

        if (sr_meta_ptr) {
                sr_path = &(sr_meta_ptr->path);
        } else {
            fatal(CALL_INFO, -1, "DEBUG anytopo: No SourceRouting metadata found?");
        }

        // Route the packet based on the path
        int fwd_port = -1;
        if (sr_path->empty() || sr_path->back() == router_id) {
            // Arriving at destination router, forward to endpoint

            int dest_EP_id = ev->getDest();
            if (dest_EP_id == -1){
                fatal(CALL_INFO, -1, "ERROR: Source routing packet missing destination endpoint ID\n");
            }

            if (get_router_id(dest_EP_id) != router_id) {
                fatal(CALL_INFO, -1, "ERROR: destination endpoint %d is not contained within this router %d\n",
                    dest_EP_id, router_id);
            }
            ev->next_router_id = -1; // no next router
            fwd_port = get_dest_local_port(dest_EP_id);

            output.verbose(CALL_INFO, 2, 0, "destination router %d forwarding packet to dest EP %d via port %d\n",
                router_id, dest_EP_id, fwd_port);
        } else {
            // Intermediate hop - determine the next router and forward to the corresponding port
            // assert that there is a front element
            assert(!sr_path->empty());
            ev->next_router_id = sr_path->front(); // update next router id and increment the hop count
            sr_path->pop_front(); // remove the current router from the path
            ev->setVC(ev->num_hops);
            fwd_port = getPortToRouter(ev->next_router_id);

            output.verbose(CALL_INFO, 2, 0, "Intermediate router %d routing packet to next router %d via port %d\n",
                router_id, ev->next_router_id, fwd_port);
        }

        if (fwd_port == -1) {
            fatal(CALL_INFO, -1, "ERROR: No port found to forward to router %d\n", ev->next_router_id);
        }
        ev->setNextPort(fwd_port);
    } else {
        fatal(CALL_INFO, -1, "timed packet should always have source routing metadata");
    }

}

void topo_any::route_packet_dest_tag(int input_port, int vc, topo_any_event* ev) {
    int dest_EP_id = ev->getDest();
    int dest_router = get_router_id(dest_EP_id);

    if (dest_router == router_id) {
        // Destination is local
        ev->setNextPort(get_dest_local_port(dest_EP_id));
    }else{
        // Route to next hop based on routing table
        auto it = Dest_Tag_Routing_Table.find(dest_router);
        if (it != Dest_Tag_Routing_Table.end() && !it->second.empty()) {
            //TODO: call different routing algorithms, set ev->next_router_id
            ev->next_router_id = it->second[0]; // Simple selection for temporary testing

            int fwd_port = getPortToRouter(ev->next_router_id);
            ev->setNextPort(fwd_port);
            ev->num_hops++;
            ev->setVC(ev->num_hops); // The setting of VC could be off-loaded into topology-specific routing algorithm
        }else{
            fatal(CALL_INFO, -1, "ERROR: No routing entry found for destination router %d\n", dest_router);
        }
    }
}

void topo_any::routeUntimedData(int input_port, internal_router_event* ev, std::vector<int> &outPorts) {
    if ( ev->getDest() == UNTIMED_BROADCAST_ADDR ) {
        fatal(CALL_INFO_LONG,1,"ERROR: routeUntimedData for UNTIMED_BROADCAST_ADDR not yet implemented");
        // We need a topology-independent network-broadcasting protocol here,
        // but I don't see any use case of UNTIMED_BROADCAST_ADDR for now in the whole sst-element source code?
    }else{
        route_untimed_packet(input_port, ev->getVC(), ev);
        outPorts.push_back(ev->getNextPort());
    }
}

internal_router_event* topo_any::process_UntimedData_input(RtrEvent* ev) {
    // if UNTIMED_BROADCAST_ADDR need to be adopted, probably some tracking will be needed here.
    // ...

    topo_any_event* _ev = new topo_any_event();
    _ev->setEncapsulatedEvent(ev);
    return _ev;
}

}
}