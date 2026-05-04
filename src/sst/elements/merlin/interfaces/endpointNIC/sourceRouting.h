// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_MERLIN_SRNIC_H
#define COMPONENTS_MERLIN_SRNIC_H

#include "endpointNIC.h"
#include "NICPlugin.h"
#include "../ExtendedRequest.h"
#include <sst/core/rng/mersenne.h>
#include <sst/core/shared/sharedArray.h>
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
    virtual const std::deque<int> selectPath(int dest_router, const std::vector<path_with_weight>& paths) = 0;
    virtual void reset() {}
};

// Random weighted path selection algorithm
class RandomWeightedSelection : public PathSelectionAlgorithm {
private:
    SST::RNG::Random* rng;

public:
    RandomWeightedSelection(ComponentId_t cid);
    ~RandomWeightedSelection();
    const std::deque<int> selectPath(int dest_router, const std::vector<path_with_weight>& paths) override;
};

// Weighted round-robin path selection algorithm
class WeightedRoundRobinSelection : public PathSelectionAlgorithm {
private:
    std::map<int, std::map<size_t, int>> dest_path_counters;
    std::map<int, std::map<size_t, int>> dest_path_weights;

public:
    WeightedRoundRobinSelection();
    ~WeightedRoundRobinSelection();
    const std::deque<int> selectPath(int dest_router, const std::vector<path_with_weight>& paths) override;
    void reset() override;
};

class SourceRoutingPlugin : public NICPlugin
{
public:
    // Shared data accessible by both SourceRoutingPlugin and topology classes
    // Using SharedArray instead of static variables for proper SubComponent support
    // Array index = endpoint ID, value = router ID (-1 means uninitialized)
    Shared::SharedArray<int> endpoint_to_router_shared;
    Shared::SharedArray<routing_entries> routing_table_shared;

private:
    PathSelectionAlgorithm* path_selector;
    size_t myRtrID;
    size_t num_routers;
    Output output;

    // Store endpoint ID (set during init)
    SST::Interfaces::SimpleNetwork::nid_t endpoint_id;

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
        {"EP_id",                    "Endpoint ID assigned to this plugin. Inherited from the parent NIC.", ""},
        {"num_routers",              "Total number of routers in the network.", ""},
        {"endpoint_router_mapping",  "Mapping from endpoint ID to router ID, passed as Python dict, e.g. {0:0, 1:0, 2:1, ...}.", ""},
        {"routing_entry_string",     "Compact routing table string for this source router. Format: 'dest1:weight1,hop1,hop2;weight2,hop3|dest2:weight1,hop1'.", ""},
        {"path_selection_algorithm", "Algorithm for selecting among weighted paths: 'random_weighted' or 'weighted_round_robin'.", "random_weighted"},
        {"verbose_level",            "Verbosity level for output messages (higher value = more output).", "0"},
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
    const std::deque<int> selectPath(int dest_router);
    inline size_t lookupRtrForEndpoint(SST::Interfaces::SimpleNetwork::nid_t endpoint) {
        return endpoint_to_router_shared[endpoint];
    }
};

} // namespace Merlin
} // namespace SST

#endif
