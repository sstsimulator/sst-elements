// TODO: copyright goes here?

// The current name of the Topology class is 'anytopo', 
// however, the name could also be 'arbitrarytopo' or 'generictopo' to better reflect its purpose.

#ifndef COMPONENTS_MERLIN_TOPOLOGY_ANYTOPO_H
#define COMPONENTS_MERLIN_TOPOLOGY_ANYTOPO_H

#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/rng/rng.h>
#include <sst/core/output.h>
#include <unordered_map>
#include <set>
#include <map>
#include <vector>

#include "sst/elements/merlin/router.h"
#include "../interfaces/endpointNIC/SourceRouting.h"

namespace SST {
namespace Merlin {

// Designed and implemented by Ziyue Zhang (UGent-IDLab, FWO), GitHub @ziyuezzy

class topo_any_event : public internal_router_event {
public:
    // // the id of the destination endpoint
    // int dest_EP_id; 
    // // the id of the source endpoint
    // int src_EP_id;  
    // number of hops the packet has traveled
    int num_hops;   
    // the next router id to forward to
    // If source routing is used, this will be read from the encapsulated request (similar to the segment routing header in IPv6)
    // If destination-tag routing is used, this will be determined by the routing table of the current router
    int next_router_id; 

    inline topo_any_event() { 
        num_hops = 0; 
        // dest_EP_id = -1; src_EP_id = -1; 
    }
    virtual ~topo_any_event() {}

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        internal_router_event::serialize_order(ser);
        // SST_SER(dest_EP_id);
        // SST_SER(src_EP_id);
        SST_SER(num_hops);
        SST_SER(next_router_id);
    }
    // TODO: for destination tag routing, could implement valiant router id for the valiant and ugal routing
    // TODO: for destination tag routing, could implement a field for ECMP hashing
private:
    ImplementSerializable(SST::Merlin::topo_any_event)
};

class topo_any: public Topology{

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        topo_any,
        "merlin",
        "anytopo",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "topology object constructed from external file, supports both source routing and per-hop destination-tag routing.",
        SST::Merlin::Topology
    )


    SST_ELI_DOCUMENT_PARAMS(
    )

    int router_id;
    // total number of routers in the network
    int num_routers;

    // The number of ports that connect to endpoints
    int num_R2N_ports;
    // The number of ports that connect to other routers
    int num_R2R_ports;

    // Credit and queue length information
    int const* output_credits;
    int const* output_queue_lengths;
    virtual void setOutputBufferCreditArray(int const* array, int vcs);
    virtual void setOutputQueueLengthsArray(int const* array, int vcs);
    // total number of virtual channels in all virtual networks
    int tot_num_vcs=0;
    RNG::Random* rng;

    // I suppose that the purpose of virtual networks (vns) is to separate different types of traffic, 
    // for example seperating requests and responses for avoiding protocol-level deadlock.
    // Thus the default number of vns is set to 2.
    int num_vns = 2;
    // In commercial switches, the number of available vcs is usually in the order of tens.
    // For example in infiniband switches (ref: https://network.nvidia.com/pdf/whitepapers/WP_InfiniBand_Technology_Overview.pdf), 
    // 16 virtual lanes (their term for virtual channels) are provided, thus that can be splitted into two virtual networks each with 8 vcs.
    // By default, the vc is incremented at every hop for avoiding deadlock, so the number of vc must not be less than the maximum path length.
    // In case path length is longer than the number of available vcs, the vc will wrap around to 0, but that may induce deadlock (a warning will be printed).
    int default_vcs_per_vn = 8;

    topo_any(ComponentId_t cid, Params &params, int num_ports, int rtr_id, int num_vns);
    ~topo_any();

    // This data structure stores which router is connected by which ports.
    // The key is destination router id, value is a set of port ids.
    // The set is for supporting multiple ports connecting to the same destination router.
    // If multiple ports are connected to another router, the current implementation randomly choose one port.
    // However, in common cases, there is only one port for a destination router (no parallel links between routers)
    std::unordered_map<int, std::set<int>> connectivity;

    enum Routing_mode{
        // Note that this need to be specified for every vn
        // If source routing is used, the routed path should be pre-determined before the first forwarding.
        // The routers are only responsible for forwarding the packet to the next hop in the pre-determined path.
        source_routing,
        // If per-hop destination-tag routing is used,
        // every intermediate router makes its own routing decision based on the destination id, 
        // for which the routing table depends on the network topology
        dest_tag_routing
    };

    // This data structure contains the necessary routing information for THIS router
    // key is destination router id, value is a list of next-hop router ids.
    std::map<int, std::vector<int>> Dest_Tag_Routing_Table; 


    // Note that this need to be specified for every vn
    // If destination-tag routing is used, there are many possible routing algorithms. And these algorithms are independent on the routing table.
    // However, only "nonadaptive" is implemented for now, as the main objective of this project was source routing.
    // You can try to implement other routing algorithms yourself (could be a bachelor/master project...), 
    // however, you can also just use that defined in the existing network topology classes.
    enum Dest_Tag_Routing_Algo{
        none,
        // the routing decision does not adapt to the traffic
        nonadaptive,
        // the routing decision adapts to the traffic, NOT YET implemented
        adaptive,
        // Classical ECMP routing, NOT YET implemented
        ECMP,
        // UGAL routing, NOT YET implemented
        UGAL
    };

    void Parse_routing_info(SST::Params &params);
    virtual void route_packet(int input_port, int vc, internal_router_event* ev);
    void route_packet_SR(topo_any_event* ev);
    void route_packet_dest_tag(int input_port, int vc, topo_any_event* ev);
    virtual internal_router_event* process_input(RtrEvent* ev);
    // virtual int routeControlPacket(CtrlRtrEvent* ev); //TODO:??
    virtual void routeUntimedData(int input_port, internal_router_event* ev, std::vector<int> &outPorts);
    virtual internal_router_event* process_UntimedData_input(RtrEvent* ev);
    
    virtual void getVCsPerVN(std::vector<int>& vcs_per_vn) {
        for ( int i = 0; i < num_vns; ++i ) {
            vcs_per_vn[i] = vns[i].num_vcs;
        }
    }
    // note that the port_id starts from 0 to num_R2R_ports-1 for R2R ports, 
    // and from num_R2R_ports to num_R2R_ports+num_R2N_ports-1 for R2N ports
    virtual PortState getPortState(int port_id) const;
    // will return -1 if the port is not an R2N port
    virtual int getEndpointID(int port_id);


private:

    // a helper function to read an array of parameters, with a default value
    template<typename T>
    std::vector<T> read_array_param(const Params& params, const std::string& key, int num, const T& default_val) {
        std::vector<T> arr;
        if (params.is_value_array(key)) {
            params.find_array<T>(key, arr);
            if (arr.size() != num) {
                fatal(CALL_INFO, -1, "ERROR: %s array length must match the input number (%d expected, %lu found)\n", key.c_str(), num, arr.size());
            }
        } else if (params.contains(key)) {
            // Single value provided, populate all with that value
            T val = params.find<T>(key);
            arr.resize(num, val);
        } else {
            // Nothing found, populate with default_val and warn
            arr.resize(num, default_val);
            if( router_id == 0 ) {// only print the warning once
                try {
                    if constexpr (std::is_arithmetic<T>::value) {
                        output.output("WARNING: Parameter '%s' not found for anytopo, using default value: %s\n", key.c_str(), std::to_string(default_val).c_str());
                    } else if constexpr (std::is_same<T, std::string>::value) {
                        output.output("WARNING: Parameter '%s' not found for anytopo, using default value: %s\n", key.c_str(), default_val.c_str());
                    } else {
                        output.output("WARNING: Parameter '%s' not found for anytopo, using default value (unprintable type)\n", key.c_str());
                    }
                } catch (...) {
                    output.output("WARNING: Parameter '%s' not found for anytopo, using default value (unprintable type)\n", key.c_str());
                }
            }
        }
        return arr;
    }

    struct vn_info {
        // this is the offset of the first vc id of this vn
        int start_vc;
        // number of vcs in this vn
        int num_vcs;
        // used for adaptive routing, NOT YET implemented
        int ugal_bias;
        Routing_mode routing_mode;
        // only used if routing_mode == dest_tag_routing
        Dest_Tag_Routing_Algo vn_dest_tag_routing_algo;
    };

    vn_info* vns; 
    // calculate the destination router id based on the destination endpoint id
    int get_router_id(int dest_EP_id) const;
    // calculate the local port id (R2N port) based on the destination endpoint id
    int get_dest_local_port(int dest_EP_id) const;

    // for now ports are randomly selected
    int getPortToRouter(int target_router_id) const;
    std::set<int>& getPortsToRouter(int target_router_id) const;

    // void route_nonadaptive(int port, int vc, internal_router_event* ev, int dest_router);
    // void route_nonadaptive_weighted(int port, int vc, internal_router_event* ev, int dest_router);
    // void route_valiant(int port, int vc, internal_router_event* ev, int dest_router);
    // void route_ugal(int port, int vc, internal_router_event* ev, int dest_router, int num_VAL);
    // void route_ugal_precise(int port, int vc, internal_router_event* ev, int dest_router, int num_VAL);
    // void route_ugal_threshold(int port, int vc, internal_router_event* ev, int dest_router, int num_VAL);
    // void route_adaptive(int port, int vc, internal_router_event* ev, int dest_router);

    Output &output;
    static std::vector<routing_entries> simple_routing_table; // Shared routing table for all routers (declared here, defined in .cc)

};

}
}


#endif