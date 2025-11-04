// TODO: copyright goes here

#include <sst_config.h>
#include "SourceRouting.h"
#include <sstream>

namespace SST {
namespace Merlin {

// Initialize static members
std::vector<routing_entries> SourceRoutingPlugin::routing_table;
std::map<int, int> SourceRoutingPlugin::endpoint_to_router_map = std::map<int, int>();

RandomWeightedSelection::RandomWeightedSelection(ComponentId_t cid) {
    rng = new SST::RNG::MersenneRNG(cid);
}

RandomWeightedSelection::~RandomWeightedSelection() {
    delete rng;
}

std::deque<int> RandomWeightedSelection::selectPath(int dest_router, const std::vector<path_with_weight>& paths) {
    if (paths.empty()) {
        return std::deque<int>();
    }

    if (paths.size() == 1) {
        return paths[0].second;
    }

    float total_weight = 0.0f;
    for (const auto& path_entry : paths) {
        total_weight += path_entry.first;
    }

    if (total_weight == 0.0f) {
        return paths[0].second;
    }

    float random_val = rng->nextUniform() * total_weight;
    float cumulative_weight = 0.0f;

    for (const auto& path_entry : paths) {
        cumulative_weight += path_entry.first;
        if (random_val <= cumulative_weight) {
            return path_entry.second;
        }
    }

    return paths.back().second;
}

WeightedRoundRobinSelection::WeightedRoundRobinSelection() {
}

WeightedRoundRobinSelection::~WeightedRoundRobinSelection() {
}

std::deque<int> WeightedRoundRobinSelection::selectPath(int dest_router, const std::vector<path_with_weight>& paths) {
    if (paths.empty()) {
        return std::deque<int>();
    }

    if (paths.size() == 1) {
        return paths[0].second;
    }

    if (dest_path_counters.find(dest_router) == dest_path_counters.end()) {
        for (size_t i = 0; i < paths.size(); i++) {
            int weight_count = static_cast<int>(paths[i].first * 100);
            dest_path_weights[dest_router][i] = weight_count;
            dest_path_counters[dest_router][i] = 0;
        }
    }

    size_t selected_path = 0;
    float best_ratio = -1.0f;

    for (size_t i = 0; i < paths.size(); i++) {
        int current_count = dest_path_counters[dest_router][i];
        int target_weight = dest_path_weights[dest_router][i];

        if (target_weight == 0) continue;

        float ratio = static_cast<float>(current_count) / target_weight;
        if (best_ratio < 0 || ratio < best_ratio) {
            best_ratio = ratio;
            selected_path = i;
        }
    }

    dest_path_counters[dest_router][selected_path]++;
    return paths[selected_path].second;
}

void WeightedRoundRobinSelection::reset() {
    dest_path_counters.clear();
    dest_path_weights.clear();
}

SourceRoutingPlugin::SourceRoutingPlugin(ComponentId_t cid, Params& params) :
    NICPlugin(cid, params),
    path_selector(nullptr),
    endpoint_id(-1)
{
    output.init(getName() + ": ", 0, 0, Output::STDOUT);

    // Get endpoint ID from params (set by EndpointNIC)
    endpoint_id = params.find<SST::Interfaces::SimpleNetwork::nid_t>("EP_id", -1);
    if (endpoint_id == static_cast<SST::Interfaces::SimpleNetwork::nid_t>(-1)) {
        output.fatal(CALL_INFO, -1, "EP_id parameter not provided to SourceRoutingPlugin\n");
    }

    // Initialize path selection algorithm
    std::string algorithm = params.find<std::string>("path_selection_algorithm", "random_weighted");
    initializePathSelectionAlgorithm(algorithm);

    if (endpoint_to_router_map.empty()) {
        // Parse endpoint-to-router mapping
        // Expected format from Python: dict like "{0:0, 1:0, 2:1, 3:1, ...}"
        params.find_map<int, int>("endpoint_router_mapping", endpoint_to_router_map);
        if (endpoint_to_router_map.empty()) {
            output.fatal(CALL_INFO, -1, "No endpoint-to-router mapping provided\n");
        }

        // TODO: print out this map
        for (const auto& entry : endpoint_to_router_map) {
            output.verbose(CALL_INFO, 2, 0, "Endpoint %d -> Router %d\n", entry.first, entry.second);
        }

        output.verbose(CALL_INFO, 1, 0, "Loaded endpoint-to-router mapping for %zu endpoints\n",
                    endpoint_to_router_map.size());
    }

    // Now we can safely use endpoint_id
    if (endpoint_to_router_map.find(endpoint_id) == endpoint_to_router_map.end()) {
        output.fatal(CALL_INFO, -1, "Endpoint ID %d not found in endpoint-to-router mapping\n", endpoint_id);
    }

    myRtrID = endpoint_to_router_map[endpoint_id];
    num_routers = params.find<size_t>("num_routers", 0);
    assert(num_routers >= myRtrID && num_routers > 0);

    if (routing_table.empty()) {
        routing_table.resize(num_routers);
    }

    // Parse routing table parameters, if not present already
    if (routing_table[myRtrID].empty()) {
        parseRoutingEntry(params);
    }
}

SourceRoutingPlugin::~SourceRoutingPlugin() {
    delete path_selector;
}

void SourceRoutingPlugin::plugin_init(unsigned int phase)
{
}

SST::Interfaces::SimpleNetwork::Request* SourceRoutingPlugin::processOutgoing(
    SST::Interfaces::SimpleNetwork::Request* req, int vn)
{
    if (!req) return nullptr;

    // Convert to ExtendedRequest if not already
    ExtendedRequest* ext_req = dynamic_cast<ExtendedRequest*>(req);
    if (!ext_req) {
        ext_req = new ExtendedRequest(req);
        delete req;
    }

    // Check if path metadata already exists
    SourceRoutingMetadata sr_meta;
    if (!ext_req->getMetadata("SourceRouting", sr_meta) || sr_meta.path.empty()) {
        // Lookup path in routing table and set it as metadata
        std::deque<int> path = selectPath(lookupRtrForEndpoint(ext_req->dest));
        SourceRoutingMetadata new_sr_meta(path);
        ext_req->setMetadata("SourceRouting", new_sr_meta);
    }

    // //TODO: print output req src and dest and assigned path:=======
    // std::cout << "Outgoing packet from endpoint " << endpoint_id  << " rtr " << myRtrID
    //           << " to endpoint " << ext_req->dest << " rtr " << lookupRtrForEndpoint(ext_req->dest) << " assigned path: ";
    // for (int hop : sr_meta.path) {
    //     std::cout << hop << " ";
    // }
    // std::cout << std::endl;
    // //==========================================================

    return ext_req;
}

SST::Interfaces::SimpleNetwork::Request* SourceRoutingPlugin::processIncoming(
    SST::Interfaces::SimpleNetwork::Request* req, int vn)
{
    // For incoming packets, just pass through
    // The path metadata remains in the ExtendedRequest if needed for debugging/stats
    return req;
}

void SourceRoutingPlugin::parseRoutingEntry(Params& params) {
    // Try string format first
    std::string routing_str = params.find<std::string>("routing_entry_string", "");

    if (!routing_str.empty()) {
        output.verbose(CALL_INFO, 1, 0, "Loading routing entry from string parameter\n");
        routing_entries entries = parseRoutingEntryFromString(routing_str, num_routers, output);
        routing_table[myRtrID] = entries;
        output.verbose(CALL_INFO, 1, 0, "Loaded routing table entry with %zu destinations\n", entries.size());
    } else { // Maybe other methods to parse routing entries in future
        output.output("Warning: No routing entry provided. Source routing will not function.\n");
        return;
    }
}

routing_entries SourceRoutingPlugin::parseRoutingEntryFromString(const std::string& routing_str, const size_t num_routers, Output& out) {
    // Parse format: dest1:w1,hop1,hop2;w2,hop3|dest2:w1,hop1...
    // This is a static method that can be called from anywhere

    routing_entries entries;
    entries.resize(num_routers);
    std::stringstream dest_stream(routing_str);
    std::string dest_entry;

    // Split by | for each destination
    while (std::getline(dest_stream, dest_entry, '|')) {
        if (dest_entry.empty()) continue;

        size_t colon_pos = dest_entry.find(':');
        if (colon_pos == std::string::npos) continue;

        try {
            int dest_id = std::stoi(dest_entry.substr(0, colon_pos));
            std::string paths_str = dest_entry.substr(colon_pos + 1);

            std::vector<path_with_weight> paths;
            std::stringstream path_stream(paths_str);
            std::string path_entry;

            // Split by ; for each path
            while (std::getline(path_stream, path_entry, ';')) {
                if (path_entry.empty()) continue;

                std::stringstream hop_stream(path_entry);
                std::string hop_str;
                std::vector<std::string> hop_parts;

                // Split by , for weight and hops
                while (std::getline(hop_stream, hop_str, ',')) {
                    hop_parts.push_back(hop_str);
                }

                if (hop_parts.empty()) continue;

                float weight = std::stof(hop_parts[0]);
                std::deque<int> path;

                for (size_t i = 1; i < hop_parts.size(); i++) {
                    path.push_back(std::stoi(hop_parts[i]));
                }

                paths.push_back(std::make_pair(weight, path));
            }

            if (!paths.empty()) {
                assert( entries[dest_id].empty() && "Duplicate routing entry for destination" );
                entries[dest_id] = paths;
            } else {
                out.output("Warning: No valid paths found for destination %d in routing entry\n", dest_id);
            }

        } catch (const std::exception& e) {
            out.output("Warning: Failed to parse routing entry: %s\n", e.what());
            continue;
        }
    }

    return entries;
}

void SourceRoutingPlugin::setSourceRoutingEntriesForSourceRtr(const routing_entries& entries, int src_router) {
    routing_table[src_router] = entries;
}

void SourceRoutingPlugin::setPathSelectionAlgorithm(PathSelectionAlgorithm* algorithm) {
    delete path_selector;
    path_selector = algorithm;
}

void SourceRoutingPlugin::initializePathSelectionAlgorithm(const std::string& algorithm_name) {
    delete path_selector;

    if (algorithm_name == "random_weighted") {
        path_selector = new RandomWeightedSelection(getId());
    } else if (algorithm_name == "weighted_round_robin") {
        path_selector = new WeightedRoundRobinSelection();
    } else {
        path_selector = new RandomWeightedSelection(getId());
    }
}

std::deque<int> SourceRoutingPlugin::selectPath(int dest_router) {

    if (dest_router == myRtrID){
        output.verbose(CALL_INFO, 1, 0, "Assigning empty path for endpoints in the same router. \n");
        return std::deque<int>();
    }else{
        return path_selector->selectPath(dest_router, routing_table[myRtrID][dest_router]);
    }
}

} // namespace Merlin
} // namespace SST
