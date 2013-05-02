#ifndef SIMPLE_CACHE_SIMPLE_CACHE_H_
#define SIMPLE_CACHE_SIMPLE_CACHE_H_

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/element.h>
#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include "cache_messages.h"
#include "MemoryControllerMap.h"
#include "hash_table.h"
#include "cache_types.h"
#include "simpleMC/simple_mc.h"

class simple_cache : public SST::Component {
public:
//    virtual int Setup() {  // Renamed per Issue 70 - ALevine
    virtual void setup() { 
        std::cout << "zesto simple cache " << node_id << " start" << std::endl; 
//        return 0;
    }
//    virtual int Finish() {  // Renamed per Issue 70 - ALevine
    virtual void finish() {
        std::cout << "Simple Cache " << node_id << " finished" << std::endl;
        print_stats(std::cout);
//        return 0;
    }
        

     //simple_cache (int nid, VECTOR<int> mc_map, simple_cache_settings my_settings);
    simple_cache (SST::ComponentId_t nid, SST::Component::Params_t& params);
    ~simple_cache (void);

    //void set_mc_mask (void);
    //int lookup_mc_id (paddr_t addr);

    int get_node_id() { return node_id; }
    virtual void handle_request (SST::Event *ev);
    virtual void handle_response (SST::Event *ev); // from the network we get mem_req
//    virtual void handle_cache_response (int port, cache_req *request); //lower level cache responds with cache_req

    hash_table *my_table;

    void print_stats(std::ostream&);

private:
    int node_id;
    bool send_st_complete; //indicating whether or not ST_COMPLETE should be sent.
    MemoryControllerMap* mc_map;

    simple_cache_settings settings;

    //stats
    uint64_t n_loads; //number of loads
    uint64_t n_load_hits; //load hits

    SST::Link * link_upper; //to previous level cache or to processor
    SST::Link * link_lower; //to next level cache or to network

    simple_cache():SST::Component(-1) {} //for serialization 
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SST::Component);
        ar & BOOST_SERIALIZATION_NVP(n_loads);
        ar & BOOST_SERIALIZATION_NVP(n_load_hits);
        ar & BOOST_SERIALIZATION_NVP(link_upper);
    }
    
    template<class Archive>
    void load(Archive & ar, const unsigned int version) 
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SST::Component);
        ar & BOOST_SERIALIZATION_NVP(n_loads);
        ar & BOOST_SERIALIZATION_NVP(n_load_hits);
        ar & BOOST_SERIALIZATION_NVP(link_upper);
        //resture links
        link_upper->setFunctor(new SST::Event::Handler<simple_cache>(this,&simple_cache::handle_request));
    }
      
    BOOST_SERIALIZATION_SPLIT_MEMBER()


};



#endif // SIMPLE_CACHE_SIMPLE_CACHE_H_
