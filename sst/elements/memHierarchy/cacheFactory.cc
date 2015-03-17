// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   cacheFactory.cc
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */


#include <sst_config.h>
#include <boost/algorithm/string/predicate.hpp>
#include "hash.h"
#include "cacheController.h"
#include "util.h"
#include "cacheListener.h"
#include <sst/core/params.h>
#include <boost/lexical_cast.hpp>
#include "mshr.h"


namespace SST{ namespace MemHierarchy{
    using namespace SST::MemHierarchy;
    using namespace std;

Cache* Cache::cacheFactory(ComponentId_t _id, Params &_params){
 
    /* --------------- Output Class --------------- */
    Output* dbg = new Output();
    int debugLevel = _params.find_integer("debug_level", 0);
    if(debugLevel < 0 || debugLevel > 10)     _abort(Cache, "Debugging level must be between 0 and 10. \n");
    
    dbg->init("--->  ", debugLevel, 0,(Output::output_location_t)_params.find_integer("debug", 0));
    dbg->debug(_INFO_,"\n--------------------------- Initializing [Memory Hierarchy] --------------------------- \n\n");

    /* --------------- Get Parameters --------------- */
    string frequency            = _params.find_string("cache_frequency", "" );            //Hertz
    string replacement          = _params.find_string("replacement_policy", "LRU");
    int associativity           = _params.find_integer("associativity", -1);
    string sizeStr              = _params.find_string("cache_size", "");                  //Bytes
    int lineSize                = _params.find_integer("cache_line_size", -1);            //Bytes
    int accessLatency           = _params.find_integer("access_latency_cycles", -1);                 //ns
    int mshrSize                = _params.find_integer("mshr_num_entries", -1);           //number of entries
    string preF                 = _params.find_string("prefetcher");
    int L1int                   = _params.find_integer("L1", 0);
    int dirAtNextLvl            = _params.find_integer("directory_at_next_level", 0);
    string coherenceProtocol    = _params.find_string("coherence_protocol", "");
    string statGroups           = _params.find_string("stat_group_ids", "");
    string bottomNetwork        = _params.find_string("bottom_network", "");
    string topNetwork           = _params.find_string("top_network", "");
    int noncacheableRequests    = _params.find_integer("force_noncacheable_reqs", 0);
    SimTime_t maxWaitTime       = _params.find_integer("maxRequestDelay", 0);  // Nanoseconds
    bool L1 = (L1int == 1);
    vector<int> statGroupIds;
    
    istringstream ss(statGroups);
    string token;
    
    statGroupIds.push_back(0);         //Id = 0 prints overall statistics

    while(getline(ss, token, ',')) {
        statGroupIds.push_back(atoi(token.c_str()));
    }

    /* Check user specified all required fields */
    if(frequency.empty())           dbg->fatal(CALL_INFO, -1, "Param not specified: frequency - cache frequency\n");
    if(-1 >= associativity)         dbg->fatal(CALL_INFO, -1, "Param not specified: associativity\n");
    if(sizeStr.empty())             dbg->fatal(CALL_INFO, -1, "Param not specified: cache_size\n");
    if(-1 == lineSize)              dbg->fatal(CALL_INFO, -1, "Param not specified: cache_line_size - number of bytes in a cacheline (block size)\n");
    if(L1int != 1 && L1int != 0)    dbg->fatal(CALL_INFO, -1, "Param not specified: L1 - should be '1' if cache is an L1, 0 otherwise\n");
    if(accessLatency == -1 )        dbg->fatal(CALL_INFO, -1, "Param not specified: access_latency_cycles - access time for cache\n");
    
    /* Check that parameters are valid */
    if(dirAtNextLvl > 1 || dirAtNextLvl < 0)    dbg->fatal(CALL_INFO, -1, "Invalid param: directory_at_next_level - should be '1' if directory exists at next level below this cache, 0 otherwise\n");
    if(!(bottomNetwork == "" || bottomNetwork == "directory" || bottomNetwork == "cache"))  dbg->fatal(CALL_INFO, -1, "Invalid param: bottom_network - valid options are '', 'directory', or 'cache'\n");
    if(!(topNetwork == "" || topNetwork == "cache"))    dbg->fatal(CALL_INFO, -1, "Invalid param: top_network - valid options are '' or 'cache'\n");

    /* NACKing to from L1 to the CPU doesnt really happen in CPUs*/
    if(L1 && mshrSize != -1)    dbg->fatal(CALL_INFO, -1, "Invalid param: mshr_num_entries - must be -1 for L1s, memHierarchy assumes L1 MSHR is sized to match the CPU's load/store queue\n");
    
    /* No L2+ cache should realistically have an MSHR that is less than 10-16 entries */
    if(-1 == mshrSize) mshrSize = HUGE_MSHR;
    if(mshrSize < 2)            dbg->fatal(CALL_INFO, -1, "Invalid param: mshr_num_entries - MSHR requires at least 2 entries to avoid deadlock\n");

    /* Update parameters for initialization */
    if (dirAtNextLvl) bottomNetwork = "directory";
    
    /* ---------------- Initialization ----------------- */
    HashFunction* ht = new PureIdHashFunction;
    boost::algorithm::to_lower(coherenceProtocol);
    boost::algorithm::to_lower(replacement);
    
    long cacheSize = SST::MemHierarchy::convertToBytes(sizeStr);
    uint numLines = cacheSize/lineSize;
    uint protocol = 0;
    
    if(coherenceProtocol == "mesi")      protocol = 1;
    else if(coherenceProtocol == "msi")  protocol = 0;
    else
        dbg->fatal(CALL_INFO,-1, "Invalid param: coherence_protocol - must be 'msi' or 'mesi'\n");

    ReplacementMgr* replManager;
    if (boost::iequals(replacement, "lru"))         replManager = new LRUReplacementMgr(dbg, numLines, associativity, true);
    else if (boost::iequals(replacement, "lfu"))    replManager = new LFUReplacementMgr(dbg, numLines, associativity);
    else if (boost::iequals(replacement, "random")) replManager = new RandomReplacementMgr(dbg, associativity);
    else if (boost::iequals(replacement, "mru"))    replManager = new MRUReplacementMgr(dbg, numLines, associativity, true);
    else if (boost::iequals(replacement, "nmru"))   replManager = new NMRUReplacementMgr(dbg, numLines, associativity);
    else dbg->fatal(CALL_INFO, -1, "Invalid param: replacement_policy - supported policies are 'lru', 'lfu', 'random', 'mru', and 'nmru'\n");
    
    CacheArray* cacheArray = new SetAssociativeArray(dbg, cacheSize, lineSize, associativity, replManager, ht, !L1);

    CacheConfig config = {frequency, cacheArray, protocol, dbg, replManager, numLines,
	static_cast<uint>(lineSize),
	static_cast<uint>(mshrSize), L1, bottomNetwork, topNetwork, statGroupIds,
	static_cast<bool>(noncacheableRequests), maxWaitTime};
    return new Cache(_id, _params, config);
}



Cache::Cache(ComponentId_t _id, Params &_params, CacheConfig _config) : Component(_id){
    cf_ = _config;
    d_  = cf_.dbg_;
    L1_ = cf_.L1_;
    d_->debug(_INFO_,"--------------------------- Initializing [Cache]: %s... \n", this->Component::getName().c_str());
    pMembers();
    errorChecking();
    
    d2_ = new Output();
    d2_->init("", _params.find_integer("debug_level", 0), 0,(Output::output_location_t)_params.find_integer("debug", 0));

    statsFile_          = _params.find_integer("statistics", 0);
    idleMax_            = _params.find_integer("idle_max", 10000);
    accessLatency_      = _params.find_integer("access_latency_cycles", -1);
    tagLatency_         = _params.find_integer("tag_access_latency_cycles",accessLatency_);
    string prefetcher   = _params.find_string("prefetcher");
    mshrLatency_        = _params.find_integer("mshr_latency_cycles", 0);
    int cacheSliceCount         = _params.find_integer("num_cache_slices", 1);
    int sliceID                 = _params.find_integer("slice_id", 0);
    string sliceAllocPolicy     = _params.find_string("slice_allocation_policy", "rr");
    bool snoopL1Invs    = false;
    if (L1_) snoopL1Invs = (_params.find_integer("snoop_l1_invalidations", 0)) ? true : false;
    
    /* --------------- Check parameters -------------*/
    if (accessLatency_ < 1) d_->fatal(CALL_INFO,-1, "Invalid param: access_latency_cycles - must be at least 1\n");
   
    if (cf_.topNetwork_ == "cache") {
        if (cacheSliceCount == 1) sliceID = 0;
        else if (cacheSliceCount > 1) {
            if (sliceID >= cacheSliceCount) d_->fatal(CALL_INFO,-1, "Invalid param: slice_id - should be between 0 and num_cache_slices-1\n");
            if (sliceAllocPolicy != "rr") d_->fatal(CALL_INFO,-1, "Invalid param: slice_allocation_policy - supported policy is 'rr' (round-robin)\n");
        } else {
            d2_->fatal(CALL_INFO, -1, "Invalid param: num_cache_slices - should be 1 or greater\n");
        }
    }

    /* --------------- Prefetcher ---------------*/
    if (prefetcher.empty()) {
	listener_ = new CacheListener();
    } else {
	Params prefetcherParams = _params.find_prefix_params("prefetcher." );
	listener_ = dynamic_cast<CacheListener*>(loadModule(prefetcher, prefetcherParams));
    }

    listener_->setOwningComponent(this);
    listener_->registerResponseCallback(new Event::Handler<Cache>(this, &Cache::handlePrefetchEvent));

    /* ---------------- Latency ---------------- */
    if(mshrLatency_ < 1) intrapolateMSHRLatency();
    
    /* ----------------- MSHR ----------------- */
    mshr_               = new MSHR(d_, cf_.MSHRSize_);
    mshrNoncacheable_   = new MSHR(d_, HUGE_MSHR);
    
    /* ---------------- Links ---------------- */
    lowNetPorts_        = new vector<Link*>();
    highNetPorts_       = new vector<Link*>();

    nextLevelCacheNames_ = new vector<string>; 
    /* ---------------- Clock ---------------- */
    clockHandler_       = new Clock::Handler<Cache>(this, &Cache::clockTick);
    defaultTimeBase_    = registerClock(cf_.cacheFrequency_, clockHandler_);
    
    registerTimeBase("2 ns", true);       //  TODO:  Is this right?

    /* ---------------- Memory NICs --------------- */
    if (cf_.bottomNetwork_ == "directory" && cf_.topNetwork_ == "") { // cache with a dir below, direct connection to cache above
        if (!isPortConnected("directory")) {
            d_->fatal(CALL_INFO,-1,"Error initializing memNIC for cache: directory port is not conected\n");
        }
        MemNIC::ComponentInfo myInfo;
        myInfo.link_port = "directory";
        myInfo.link_bandwidth = _params.find_string("network_bw", "1GB/s");
	myInfo.num_vcs = _params.find_integer("network_num_vc", 3);
        myInfo.name = getName();
        myInfo.network_addr = _params.find_integer("network_address");
        myInfo.type = MemNIC::TypeCache; 
        myInfo.link_inbuf_size = _params.find_string("network_input_buffer_size", "1KB");
        myInfo.link_outbuf_size = _params.find_string("network_output_buffer_size", "1KB");

        MemNIC::ComponentTypeInfo typeInfo;
        typeInfo.blocksize = cf_.lineSize_;

        bottomNetworkLink_ = new MemNIC(this, myInfo, new Event::Handler<Cache>(this, &Cache::processIncomingEvent));
        bottomNetworkLink_->addTypeInfo(typeInfo);

        topNetworkLink_ = NULL;
    } else if (cf_.bottomNetwork_ == "cache" && cf_.topNetwork_ == "") { // cache with another cache below it on the network
        if (!isPortConnected("cache")) {
            d_->fatal(CALL_INFO,-1,"Error initializing memNIC for cache: cache port is not conected\n");
        }
        MemNIC::ComponentInfo myInfo;
        myInfo.link_port = "cache";
        myInfo.link_bandwidth = _params.find_string("network_bw", "1GB/s");
	myInfo.num_vcs = _params.find_integer("network_num_vc", 3);
        myInfo.name = getName();
        myInfo.network_addr = _params.find_integer("network_address");
        myInfo.type = MemNIC::TypeCacheToCache; 
        myInfo.link_inbuf_size = _params.find_string("network_input_buffer_size", "1KB");
        myInfo.link_outbuf_size = _params.find_string("network_output_buffer_size", "1KB");

        MemNIC::ComponentTypeInfo typeInfo;
        typeInfo.blocksize = cf_.lineSize_;

        bottomNetworkLink_ = new MemNIC(this, myInfo, new Event::Handler<Cache>(this, &Cache::processIncomingEvent));
        bottomNetworkLink_->addTypeInfo(typeInfo);
        
        topNetworkLink_ = NULL;
    } else if (cf_.bottomNetwork_ == "directory" && cf_.topNetwork_ == "cache") {
        if (!isPortConnected("directory")) {
            d_->fatal(CALL_INFO,-1,"Error initializing memNIC for cache: directory port is not conected\n");
        }
        
        MemNIC::ComponentInfo myInfo;
        myInfo.link_port = "directory";
        myInfo.link_bandwidth = _params.find_string("network_bw", "1GB/s");
	myInfo.num_vcs = _params.find_integer("network_num_vc", 3);
        myInfo.name = getName();
        myInfo.network_addr = _params.find_integer("network_address");
        myInfo.type = MemNIC::TypeNetworkCache; 
        myInfo.link_inbuf_size = _params.find_string("network_input_buffer_size", "1KB");
        myInfo.link_outbuf_size = _params.find_string("network_output_buffer_size", "1KB");

        MemNIC::ComponentTypeInfo typeInfo;
        uint64_t addrRangeStart = 0;
        uint64_t addrRangeEnd = (uint64_t)-1;
        uint64_t interleaveSize = 0;
        uint64_t interleaveStep = 0;
        if (cacheSliceCount > 1) {
            if (sliceAllocPolicy == "rr") {
                addrRangeStart = sliceID*cf_.lineSize_;
                interleaveSize = cf_.lineSize_;
                interleaveStep = cacheSliceCount*cf_.lineSize_;
            }
        }
        typeInfo.rangeStart     = addrRangeStart;
        typeInfo.rangeEnd       = addrRangeEnd;
        typeInfo.interleaveSize = interleaveSize;
        typeInfo.interleaveStep = interleaveStep;
        typeInfo.blocksize      = cf_.lineSize_;
        
        bottomNetworkLink_ = new MemNIC(this, myInfo, new Event::Handler<Cache>(this, &Cache::processIncomingEvent));
        bottomNetworkLink_->addTypeInfo(typeInfo);
        
        topNetworkLink_ = bottomNetworkLink_;
    } else {
        bottomNetworkLink_ = NULL;
        topNetworkLink_ = NULL;
    }
    
    /* ------------- Member variables intialization ------------- */
    
    configureLinks();
    groupStats_             = (cf_.statGroupIds_.size() < 2) ? false : true;
    clockOn_                = true;
    idleCount_              = 0;
    memNICIdleCount_        = 0;
    memNICIdle_             = false;
    timestamp_              = 0;
    totalUpgradeLatency_    = 0;
    upgradeCount_           = 0;
    missLatency_GetS_IS     = 0;
    missLatency_GetS_M      = 0;
    missLatency_GetX_IM     = 0;
    missLatency_GetX_SM     = 0;
    missLatency_GetX_M      = 0;
    missLatency_GetSEx_IM   = 0;
    missLatency_GetSEx_SM   = 0;
    missLatency_GetSEx_M    = 0;



    if(groupStats_){
        for(unsigned int i = 0; i < cf_.statGroupIds_.size(); i++) {
            if (i > 0 && (cf_.statGroupIds_[i] == 0 || cf_.statGroupIds_[i] == -1)) 
                d_->fatal(CALL_INFO,-1, "Invalid param: stat_group_ids - custom IDs cannot be 0 or -1\n");
            stats_[cf_.statGroupIds_[i]].initialize();
        }
    }
        
    /* --------------- Coherence Controllers --------------- */
    sharersAware_ = (L1_) ? false : true;
    topCC_ = (!L1_) ? new MESITopCC(this, d_, cf_.protocol_, cf_.numLines_, cf_.lineSize_, accessLatency_, tagLatency_, mshrLatency_, highNetPorts_, topNetworkLink_) :
                      new TopCacheController(this, d_, cf_.lineSize_, accessLatency_, tagLatency_, mshrLatency_, highNetPorts_, snoopL1Invs);
    bottomCC_ = new MESIBottomCC(this, this->getName(), d_, lowNetPorts_, listener_, cf_.lineSize_, accessLatency_, tagLatency_, mshrLatency_, L1_, bottomNetworkLink_, groupStats_, cf_.statGroupIds_);
   
    /*---------------  Misc --------------- */
    cf_.rm_->setTopCC(topCC_);  cf_.rm_->setBottomCC(bottomCC_);
    
    bottomCC_->setName(this->getName());
    topCC_->setName(this->getName());
}


void Cache::configureLinks(){
    char buf[200], buf2[200], buf3[200];
    int highNetCount = 0;
    bool lowNetExists = false;
    sprintf(buf2, "Error:  High network port was not specified correctly on componenent %s.  Please name ports \'high_network_x' where x is the port number and starts at 0\n", this->getName().c_str());
    sprintf(buf,  "Error:  Low network port was not specified correctly on component %s.  Please name ports \'low_network_x' where x is the port number and starts at 0\n", this->getName().c_str());
    sprintf(buf3, "Error:  More than one high network port specified in %s.  Please use a 'Bus' component when connecting more than one higher level cache (eg. 2 L1s, 1 L2)\n", this->getName().c_str());

    if(cf_.bottomNetwork_ == ""){
        for(uint id = 0 ; id < 200; id++) {
            string linkName = "low_network_" + boost::lexical_cast<std::string>(id);
            SST::Link* link = configureLink(linkName, "50ps", new Event::Handler<Cache>(this, &Cache::processIncomingEvent));
            if(link){
                d_->debug(_INFO_,"Low Network Link ID: %u \n", (uint)link->getId());
                lowNetPorts_->push_back(link);
                lowNetExists = true;
            }else break;
            
        }
    }

    if(cf_.topNetwork_ == "") {
        for(uint id = 0 ; id < 200; id++) {
            string linkName = "high_network_" + boost::lexical_cast<std::string>(id);
            SST::Link* link = configureLink(linkName, "50ps", new Event::Handler<Cache>(this, &Cache::processIncomingEvent));  //TODO: fix
            if(link) {
                d_->debug(_INFO_,"High Network Link ID: %u \n", (uint)link->getId());
                highNetPorts_->push_back(link);
                highNetCount++;
            } else break;
        }
    }
    
    if(cf_.bottomNetwork_ == "") BOOST_ASSERT_MSG(lowNetExists, buf);
    if(cf_.topNetwork_ == "") BOOST_ASSERT_MSG(highNetCount > 0,  buf2);
    if(cf_.topNetwork_ == "") BOOST_ASSERT_MSG(highNetCount < 2,  buf3);
    selfLink_ = configureSelfLink("Self", "50ps", new Event::Handler<Cache>(this, &Cache::handleSelfEvent));
}



void Cache::intrapolateMSHRLatency(){
    uint64 N = 200; // max cache latency supported by the intrapolation method
    int y[N];

    if(L1_){
        mshrLatency_ = 1;
        return;
    }
    
    /* L2 */
    y[0] = 0;
    y[1] = 1;
    for(uint64 idx = 2;  idx < 12; idx++) y[idx] = 2;
    for(uint64 idx = 13; idx < 16; idx++) y[idx] = 3;
    for(uint64 idx = 17; idx < 26; idx++) y[idx] = 5;

    
    /* L3 */
    for(uint64 idx = 27; idx < 46; idx++) y[idx] = 19;
    for(uint64 idx = 47; idx < 68; idx++) y[idx] = 26;
    for(uint64 idx = 69; idx < N;  idx++) y[idx] = 32;
    
    if (accessLatency_ > N) {
        d_->fatal(CALL_INFO, -1, "Error: cannot intrapolate MSHR latency if cache latency > 200. Cache latency: %d\n", accessLatency_);
    }
    mshrLatency_ = y[accessLatency_];

}

}}
