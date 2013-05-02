#ifndef SIMPLE_MC_SIMPLE_MC_H
#define SIMPLE_MC_SIMPLE_MC_H

#include <map>
#include <vector>
#include <iostream>


#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/element.h>
#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include "simpleCache/cache_messages.h"
#include "simpleCache/MemoryControllerMap.h"

//! This class emulates a MC. Upon receiving a request it simply returns a response
//! after certain delay.
class SimpleMC : public SST::Component {
public:
    SST::Link * link_ni;

    SimpleMC(SST::ComponentId_t id, SST::Component::Params_t &params);

//    int Setup() {  // Renamed per Issue 70 - ALevine
    void setup() { 
        std::cout << "Simple MC " << m_nid << " starts" << std::endl;
//        return 0;
    }
    
//    int Finish() {  // Renamed per Issue 70 - ALevine
    void finish() {
        std::cout << "Simple MC " << m_nid << " finished" << std::endl;
        //print_stats(std::cout);
//        return 0;
    }

    int get_nid() { return m_nid; }

    void handle_incoming(SST::Event *ev);

    void print_config(std::ostream&);
    void print_stats(std::ostream&);
private:
    int m_nid;  //node id
    SST::SimTime_t m_latency;
    bool m_send_st_response;  //send response for stores

    //for stats
    struct Req_info {
        Req_info(int t, int oid, int sid, paddr_t a) : type(t), org_id(oid), src_id(sid), addr(a) {}
        int type; //ld or st
        int org_id; //originator id
        int src_id; //source id; could be different from originator id, e.g., when it's from an LLS cache
	paddr_t addr;
    };

    std::map<int, int> m_ld_misses; //number of ld misses per source
    std::map<int, int> m_stores;
    std::multimap<SST::SimTime_t, Req_info> m_req_info; //request info in time order
};


//! This memory controller map assumes there is only 1 memory controller, whose ID
//! is given in the constructor.
class SimpleMcMap1 : public MemoryControllerMap {
public:
    SimpleMcMap1(int nodeId) : m_nodeId(nodeId)
    {}

    int lookup(paddr_t addr)
    {
        return m_nodeId;
    }
private:
    int m_nodeId;
};


//! This memory controller map distributes the memory lines (based on cache line)
//! among the controllers.
class SimpleMcMap : public MemoryControllerMap {
public:
    SimpleMcMap(std::vector<int>& nodeIds, int line_size);

    int lookup(paddr_t addr);

private:
    std::vector<int> m_nodeIds;
    int m_offset_bits;
    paddr_t  m_mc_selector_mask;
};


#endif //SIMPLE_MC_SIMPLE_MC_H
