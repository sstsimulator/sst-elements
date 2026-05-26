// TODO: copyright goes here

#ifndef COMPONENTS_MERLIN_SRNIC_H
#define COMPONENTS_MERLIN_SRNIC_H

#include "endpointNIC.h"
#include "NICPlugin.h"
#include "../ExtendedRequest.h"
#include <sst/core/rng/mersenne.h>
#include <map>
#include <vector>
#include <string>

namespace SST {
namespace Merlin {

// Define a type for path with weight, where the float is the weight and the deque<int> is the sequence of hop router IDs
typedef std::pair<float, std::deque<int>> path_with_weight;

// The index is destination router ID, the value is a vector of weighted paths (each path is a vector of hop router IDs) with associated weights
typedef std::vector<std::vector<path_with_weight>> routing_entries;

// Abstract interface for path selection algorithms
class PathSelectionAlgorithm {
public:
    virtual ~PathSelectionAlgorithm() {}
    virtual std::deque<int> selectPath(int dest_router, const std::vector<path_with_weight>& paths) = 0;
    virtual void reset() {}
};

// Random weighted path selection algorithm
class RandomWeightedSelection : public PathSelectionAlgorithm {
private:
    SST::RNG::Random* rng;
    
public:
    RandomWeightedSelection(ComponentId_t cid);
    ~RandomWeightedSelection();
    std::deque<int> selectPath(int dest_router, const std::vector<path_with_weight>& paths) override;
};

// Weighted round-robin path selection algorithm
class WeightedRoundRobinSelection : public PathSelectionAlgorithm {
private:
    std::map<int, std::map<size_t, int>> dest_path_counters;
    std::map<int, std::map<size_t, int>> dest_path_weights;
    
public:
    WeightedRoundRobinSelection();
    ~WeightedRoundRobinSelection();
    std::deque<int> selectPath(int dest_router, const std::vector<path_with_weight>& paths) override;
    void reset() override;
};

class SourceRoutingPlugin : public NICPlugin
{
public:
    // Static shared data accessible by both SourceRoutingPlugin and topology classes
    static std::map<int, int> endpoint_to_router_map;  // Shared endpoint-to-router mapping
    
    // Static accessor methods for external access
    static std::vector<routing_entries>& getRoutingTable() { return routing_table; }
    static std::map<int, int>& getEndpointToRouterMap() { return endpoint_to_router_map; }

private:
    PathSelectionAlgorithm* path_selector;
    size_t myRtrID;
    size_t num_routers;
    Output output;
    
    // Store endpoint ID (set during init)
    SST::Interfaces::SimpleNetwork::nid_t endpoint_id;
    static std::vector<routing_entries> routing_table; // Shared routing table for all routers

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        SourceRoutingPlugin,
        "merlin",
        "sourceRoutingPlugin",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Source routing plugin that adds source routing headers to packets",
        SST::Merlin::NICPlugin
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"routing_entry_string", "Compact routing table string format: dest1:weight1,hop1,hop2;weight2,hop3|dest2:weight1,hop1", ""},
        {"endpoint_router_mapping", "Mapping from endpoint ID to router ID, passed as Python dict", ""},
        {"path_selection_algorithm", "Algorithm for selecting paths from routing table", "random_weighted"},
        {"num_routers", "Number of routers in the network", ""}
    )

    SourceRoutingPlugin(ComponentId_t cid, Params& params);
    virtual ~SourceRoutingPlugin();

    // NICPlugin interface
    virtual SST::Interfaces::SimpleNetwork::Request* processOutgoing(
        SST::Interfaces::SimpleNetwork::Request* req, int vn) override;
    
    virtual SST::Interfaces::SimpleNetwork::Request* processIncoming(
        SST::Interfaces::SimpleNetwork::Request* req, int vn) override;
    
    virtual std::string getPluginName() const override { return "SourceRoutingPlugin"; }
    
    virtual void plugin_init(unsigned int phase) override;

    // Configuration methods
    void setSourceRoutingEntriesForSourceRtr(const routing_entries& entries, int src_router);
    void setPathSelectionAlgorithm(PathSelectionAlgorithm* algorithm);
    
    // Static method for parsing routing entries - can be called from anywhere
    static routing_entries parseRoutingEntryFromString(const std::string& routing_str, const size_t num_routers, Output& out);

private:
    void parseRoutingEntry(Params& params);
    void initializePathSelectionAlgorithm(const std::string& algorithm_name);
    std::deque<int> selectPath(int dest_router);
    inline size_t lookupRtrForEndpoint(SST::Interfaces::SimpleNetwork::nid_t endpoint) {
        return endpoint_to_router_map[endpoint];
    }
};

} // namespace Merlin
} // namespace SST

#endif