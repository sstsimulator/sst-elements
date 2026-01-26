#include <sst_config.h>
#include "anytopo.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdlib.h>
#include <set>
#include <limits>
#include "sst/core/rng/xorshift.h"
#include "sst/core/output.h"
#include "../interfaces/ExtendedRequest.h"
// #include "../interfaces/endpointNIC/SourceRouting.h"

namespace SST {
namespace Merlin {


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
    std::vector<int> vn_ugal_num_valiant = read_array_param<int>(params, "vn_ugal_num_valiant", num_vns, 3);
    std::vector<std::string> vn_dest_tag_algo = read_array_param<std::string>(params, "dest_tag_routing_algo", num_vns, "none");
    std::vector<std::string> vn_sr_algo = read_array_param<std::string>(params, "source_routing_algo", num_vns, "weighted");

    // Set up VN info
    int start_vc = 0;
    for (int i = 0; i < num_vns; ++i) {
        vns[i].routing_mode = (vn_routing_mode[i] == "source_routing") ? source_routing : dest_tag_routing;
        vns[i].src_routing_algo = SR_WEIGHTED;
        vns[i].dest_tag_routing_algo = DT_NONE;
        vns[i].start_vc = start_vc;
        vns[i].num_vcs = vcs_per_vn[i];
        tot_num_vcs += vcs_per_vn[i];
        vns[i].ugal_num_valiant = vn_ugal_num_valiant[i];
        start_vc += vcs_per_vn[i];

        // Set routing algorithm based on mode
        if (vns[i].routing_mode == source_routing) {
            if (vn_sr_algo[i] == "weighted") {
                vns[i].src_routing_algo = SR_WEIGHTED;
            } else if (vn_sr_algo[i] == "UGAL") {
                vns[i].src_routing_algo = SR_UGAL;
            } else if (vn_sr_algo[i] == "UGAL_THRESHOLD") {
                vns[i].src_routing_algo = SR_UGAL_THRESHOLD;
            } else {
                fatal(CALL_INFO, -1, "ERROR: Invalid source routing algorithm: %s\n", vn_sr_algo[i].c_str());
            }
        } else if (vns[i].routing_mode == dest_tag_routing) {
            if (vn_dest_tag_algo[i] == "nonadaptive") vns[i].dest_tag_routing_algo = DT_NONADAPTIVE;
            else if (vn_dest_tag_algo[i] == "adaptive") vns[i].dest_tag_routing_algo = DT_ADAPTIVE;
            else if (vn_dest_tag_algo[i] == "ECMP") vns[i].dest_tag_routing_algo = DT_ECMP;
            else if (vn_dest_tag_algo[i] == "UGAL") vns[i].dest_tag_routing_algo = DT_UGAL;
            else fatal(CALL_INFO, -1, "ERROR: Invalid destination tag routing algorithm: %s\n", vn_dest_tag_algo[i].c_str());
        }
    }

    // Initialize RNG
    rng = new RNG::XORShiftRNG(rtr_id+1);

    // Read adaptive threshold parameter for UGAL routing (default 2.0 matches dragonfly)
    adaptive_threshold = params.find<double>("adaptive_threshold", 2.0);

    // Parse endpoint mappings first (needed to determine SharedArray size)
    params.find_map<int, int>("endpoint_to_port_map", endpoint_to_port_map);
    if (endpoint_to_port_map.empty()) {
        output.fatal(CALL_INFO, -1, "No endpoint-to-port mapping provided\n");
    }

    params.find_map<int, int>("port_to_endpoint_map", port_to_endpoint_map);
    if (port_to_endpoint_map.empty()) {
        output.fatal(CALL_INFO, -1, "No port-to-endpoint mapping provided\n");
    }

    // Initialize SharedArrays with same names as SourceRoutingPlugin uses
    std::string basename = "SourceRoutingPlugin.";

    // Connect to the endpoint-to-router mapping SharedArray (SourceRoutingPlugin creates it)
    int max_endpoint_id = 0;
    for (const auto& entry : endpoint_to_port_map) {
        if (entry.first > max_endpoint_id) max_endpoint_id = entry.first;
    }
    size_t num_endpoints = max_endpoint_id + 1;
    // `endpoint_to_router_shared` is initialized in SourceRoutingPlugin, so just connect here
    endpoint_to_router_shared.initialize(basename + "endpoint_to_router_map", num_endpoints, -1);

    // Initialize simple routing table SharedArray
    simple_routing_table_shared.initialize(basename + "simple_routing_table", num_routers);

    Parse_routing_info(params);

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
        simple_routing_table_shared.write(router_id, entries);
        simple_routing_table_shared.publish();
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
    // Use the shared endpoint-to-router mapping
    if (EP_id < 0 || EP_id >= static_cast<int>(endpoint_to_router_shared.size())) {
        fatal(CALL_INFO, -1, "ERROR: Endpoint ID %d out of range (0-%zu)", EP_id, endpoint_to_router_shared.size() - 1);
    }
    int router = endpoint_to_router_shared[EP_id];
    if (router == -1) {
        fatal(CALL_INFO, -1, "ERROR: Endpoint %d not found in endpoint_to_router mapping", EP_id);
    }
    return router;
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
    int vn = ev->getVN();
    if (vns[vn].routing_mode == source_routing) {
        if (vns[vn].src_routing_algo == SR_UGAL) {
            route_packet_SR_UGAL(input_port, vc, static_cast<topo_any_event*>(ev));
        } else if (vns[vn].src_routing_algo == SR_UGAL_THRESHOLD) {
            route_packet_SR_UGAL_THRESHOLD(input_port, vc, static_cast<topo_any_event*>(ev));
        } else {
            route_packet_SR(static_cast<topo_any_event*>(ev));
        }
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
        sr_path = simple_routing_table_shared[router_id][dest_router][0].second;
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
        ev->next_router_id = sr_path.front(); // update next router id
        ev->num_hops++;
        if (ev->num_hops > tot_num_vcs) {
            fatal(CALL_INFO, -1, "ERROR: Number of hops exceeded the total number of VCs %d \n", tot_num_vcs);
        }
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
            ev->next_router_id = sr_path->front(); // update next router id
            sr_path->pop_front(); // remove the current router from the path
            ev->num_hops++;
            if (ev->num_hops > tot_num_vcs) {
                fatal(CALL_INFO, -1, "ERROR: Number of hops exceeded the total number of VCs %d \n", tot_num_vcs);
            }
            ev->setVC(ev->num_hops);
            fwd_port = getPortToRouter(ev->next_router_id);

            output.verbose(CALL_INFO, 2, 0, "Intermediate router %d routing packet to next router %d via port %d\n",
                router_id, ev->next_router_id, fwd_port);
        }

        if (fwd_port == -1) {
            fatal(CALL_INFO, -1, "ERROR: No port found to forward to router %d\n", ev->next_router_id);
        }

        // // Print path information
        // std::string path_str = "[";
        // for (size_t i = 0; i < sr_path->size(); ++i) {
        //     if (i > 0) path_str += ", ";
        //     path_str += std::to_string((*sr_path)[i]);
        // }
        // path_str += "]";

        // output.verbose(CALL_INFO, 3, 0, "Router %d forwarding packet (src EP %d, dst EP %d) to port %d, remaining path: %s\n",
        //               router_id, ev->getSrc(), ev->getDest(), fwd_port, path_str.c_str());
        output.verbose(CALL_INFO, 3, 0, "Router %d forwarding packet (src EP %d, dst EP %d) to port %d",
                      router_id, ev->getSrc(), ev->getDest(), fwd_port);

        ev->setNextPort(fwd_port);
    } else {
        fatal(CALL_INFO, -1, "timed packet should always have source routing metadata");
    }

}

void topo_any::route_packet_SR_UGAL(int input_port, int vc, topo_any_event* ev) {
    // UGAL: Weight-based routing matching dragonfly logic
    // Compares path_length * queue_length for both minimal and valiant routes
    // Chooses the route with minimum weight (no threshold bias)

    int dest_EP_id = ev->getDest();
    int dest_router = get_router_id(dest_EP_id);
    int vn = ev->getVN();

    if (dest_router == router_id) {
        // Destination is local
        ev->setNextPort(get_dest_local_port(dest_EP_id));
        return;
    }

    // Try to get the encapsulated request to check/store path
    auto req = ev->inspectRequest();
    if (req == nullptr) {
        fatal(CALL_INFO, -1, "ERROR: UGAL routing event missing encapsulated request\n");
    }

    ExtendedRequest* ext_req = dynamic_cast<ExtendedRequest*>(req);
    if (!ext_req) {
        fatal(CALL_INFO, -1, "ERROR: UGAL routing requires ExtendedRequest\n");
    }

    if (ev->num_hops == 0) {
        // First hop: make UGAL decision with weight calculation
        std::map<std::vector<int>, int> possible_paths; // path -> weight

        // Get routing table for this router
        const routing_entries& routing_table = simple_routing_table_shared[router_id];

        // Generate valiant paths
        int num_valiant = vns[vn].ugal_num_valiant;
        int attempts = 0;
        int max_attempts = num_valiant * 10;

        while (possible_paths.size() < static_cast<size_t>(num_valiant) && attempts < max_attempts) {
            attempts++;
            int intermediate_router = rng->generateNextUInt64() % num_routers;
            if (intermediate_router == router_id || intermediate_router == dest_router) continue;

            if (intermediate_router >= static_cast<int>(routing_table.size()) ||
                routing_table[intermediate_router].empty()) continue;

            size_t path_idx = rng->generateNextUInt32() % routing_table[intermediate_router].size();
            std::deque<int> first_segment = routing_table[intermediate_router][path_idx].second;

            const routing_entries& intermediate_table = simple_routing_table_shared[intermediate_router];
            if (dest_router >= static_cast<int>(intermediate_table.size()) ||
                intermediate_table[dest_router].empty()) continue;

            size_t dest_path_idx = rng->generateNextUInt32() % intermediate_table[dest_router].size();
            std::deque<int> second_segment = intermediate_table[dest_router][dest_path_idx].second;

            // Compose complete valiant path
            std::vector<int> complete_path(first_segment.begin(), first_segment.end());
            complete_path.insert(complete_path.end(), second_segment.begin(), second_segment.end());

            if (possible_paths.count(complete_path)) continue;

            // Calculate weight for first hop
            if (first_segment.empty()) continue;
            int next_router = first_segment.front();
            std::set<int>& ports = getPortsToRouter(next_router);
            if (ports.empty()) continue;

            int next_port = *ports.begin();
            int queue_length = output_queue_lengths[next_port * tot_num_vcs + vns[vn].start_vc];

            // Weight: path_length * queue_length (no bias - pure weight comparison)
            possible_paths[complete_path] = complete_path.size() * queue_length;
        }

        // Add minimal paths to destination
        if (dest_router < static_cast<int>(routing_table.size()) && !routing_table[dest_router].empty()) {
            for (const auto& weighted_path : routing_table[dest_router]) {
                const std::deque<int>& path = weighted_path.second;
                if (path.empty()) continue;

                int next_router = path.front();
                std::set<int>& ports = getPortsToRouter(next_router);
                if (ports.empty()) continue;

                int next_port = *ports.begin();
                int queue_length = output_queue_lengths[next_port * tot_num_vcs + vns[vn].start_vc];

                // Weight: path_length * queue_length
                std::vector<int> path_vec(path.begin(), path.end());
                possible_paths[path_vec] = path.size() * queue_length;
            }
        }

        if (possible_paths.empty()) {
            fatal(CALL_INFO, -1, "ERROR: No valid paths found for UGAL routing from router %d to router %d\n",
                  router_id, dest_router);
        }

        // Choose path with minimum weight
        int min_weight = std::numeric_limits<int>::max();
        std::vector<std::vector<int>> min_routes;

        for (const auto& p : possible_paths) {
            int weight = p.second;
            const std::vector<int>& path = p.first;

            if (weight == min_weight) {
                min_routes.push_back(path);
            } else if (weight < min_weight) {
                min_weight = weight;
                min_routes.clear();
                min_routes.push_back(path);
            }
        }

        const std::vector<int>& chosen_path = min_routes[rng->generateNextUInt32() % min_routes.size()];

        // Store the chosen path in SourceRoutingMetadata
        SourceRoutingMetadata sr_meta;
        sr_meta.path = std::deque<int>(chosen_path.begin(), chosen_path.end());
        ext_req->setMetadata("SourceRouting", sr_meta);

        // Now route using the standard source routing logic
        route_packet_SR(ev);

    } else {
        // Not first hop: use standard source routing
        route_packet_SR(ev);
    }
}

void topo_any::route_packet_SR_UGAL_THRESHOLD(int input_port, int vc, topo_any_event* ev) {
    // UGAL_THRESHOLD: Matches dragonfly's adaptive threshold logic
    // Uses valiant route if: minimal_queue_length > valiant_queue_length * adaptive_threshold
    // This mirrors dragonfly's credit-based comparison (inverted for queue lengths)

    int dest_EP_id = ev->getDest();
    int dest_router = get_router_id(dest_EP_id);
    int vn = ev->getVN();

    if (dest_router == router_id) {
        // Destination is local
        ev->setNextPort(get_dest_local_port(dest_EP_id));
        return;
    }

    // Try to get the encapsulated request to check/store path
    auto req = ev->inspectRequest();
    if (req == nullptr) {
        fatal(CALL_INFO, -1, "ERROR: UGAL_THRESHOLD routing event missing encapsulated request\n");
    }

    ExtendedRequest* ext_req = dynamic_cast<ExtendedRequest*>(req);
    if (!ext_req) {
        fatal(CALL_INFO, -1, "ERROR: UGAL_THRESHOLD routing requires ExtendedRequest\n");
    }

    if (ev->num_hops == 0) {
        // First hop: make UGAL decision using threshold-based comparison
        // Get routing table for this router
        const routing_entries& routing_table = simple_routing_table_shared[router_id];

        // Find best minimal path (lowest queue length)
        int best_minimal_queue = std::numeric_limits<int>::max();
        std::vector<int> best_minimal_path;

        if (dest_router < static_cast<int>(routing_table.size()) && !routing_table[dest_router].empty()) {
            for (const auto& weighted_path : routing_table[dest_router]) {
                const std::deque<int>& path = weighted_path.second;
                if (path.empty()) continue;

                int next_router = path.front();
                std::set<int>& ports = getPortsToRouter(next_router);
                if (ports.empty()) continue;

                int next_port = *ports.begin();
                int queue_length = output_queue_lengths[next_port * tot_num_vcs + vns[vn].start_vc];

                if (queue_length < best_minimal_queue) {
                    best_minimal_queue = queue_length;
                    best_minimal_path = std::vector<int>(path.begin(), path.end());
                }
            }
        }

        // Generate and evaluate valiant paths
        int best_valiant_queue = std::numeric_limits<int>::max();
        std::vector<int> best_valiant_path;

        int num_valiant = vns[vn].ugal_num_valiant;
        int attempts = 0;
        int max_attempts = num_valiant * 10;
        int valiant_paths_found = 0;

        while (valiant_paths_found < num_valiant && attempts < max_attempts) {
            attempts++;
            int intermediate_router = rng->generateNextUInt64() % num_routers;
            if (intermediate_router == router_id || intermediate_router == dest_router) continue;

            if (intermediate_router >= static_cast<int>(routing_table.size()) ||
                routing_table[intermediate_router].empty()) continue;

            size_t path_idx = rng->generateNextUInt32() % routing_table[intermediate_router].size();
            std::deque<int> first_segment = routing_table[intermediate_router][path_idx].second;

            const routing_entries& intermediate_table = simple_routing_table_shared[intermediate_router];
            if (dest_router >= static_cast<int>(intermediate_table.size()) ||
                intermediate_table[dest_router].empty()) continue;

            size_t dest_path_idx = rng->generateNextUInt32() % intermediate_table[dest_router].size();
            std::deque<int> second_segment = intermediate_table[dest_router][dest_path_idx].second;

            // Compose complete valiant path
            std::vector<int> complete_path(first_segment.begin(), first_segment.end());
            complete_path.insert(complete_path.end(), second_segment.begin(), second_segment.end());

            // Calculate queue length for first hop
            if (first_segment.empty()) continue;
            int next_router = first_segment.front();
            std::set<int>& ports = getPortsToRouter(next_router);
            if (ports.empty()) continue;

            int next_port = *ports.begin();
            int queue_length = output_queue_lengths[next_port * tot_num_vcs + vns[vn].start_vc];

            valiant_paths_found++;
            if (queue_length < best_valiant_queue) {
                best_valiant_queue = queue_length;
                best_valiant_path = complete_path;
            }
        }

        // Make routing decision based on threshold comparison
        // In dragonfly: valiant_credits > direct_credits * threshold means use valiant
        // With queue lengths (inverse): direct_queue > valiant_queue * threshold means use valiant
        std::vector<int> chosen_path;

        if (best_minimal_path.empty() && best_valiant_path.empty()) {
            fatal(CALL_INFO, -1, "ERROR: No valid paths found for UGAL_THRESHOLD routing from router %d to router %d\n",
                  router_id, dest_router);
        } else if (best_minimal_path.empty()) {
            // Only valiant path available
            chosen_path = best_valiant_path;
        } else if (best_valiant_path.empty()) {
            // Only minimal path available
            chosen_path = best_minimal_path;
        } else {
            // Both paths available - apply threshold comparison
            // Use valiant if minimal queue is significantly worse (more congested)
            if (best_minimal_queue > (int)((double)best_valiant_queue * adaptive_threshold)) {
                chosen_path = best_valiant_path;
            } else {
                chosen_path = best_minimal_path;
            }
        }

        // Store the chosen path in SourceRoutingMetadata
        SourceRoutingMetadata sr_meta;
        sr_meta.path = std::deque<int>(chosen_path.begin(), chosen_path.end());
        ext_req->setMetadata("SourceRouting", sr_meta);

        // Now route using the standard source routing logic
        route_packet_SR(ev);

    } else {
        // Not first hop: use standard source routing
        route_packet_SR(ev);
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
            if (ev->num_hops > tot_num_vcs) {
                fatal(CALL_INFO, -1, "ERROR: Number of hops exceeded the total number of VCs %d \n", tot_num_vcs);
            }
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