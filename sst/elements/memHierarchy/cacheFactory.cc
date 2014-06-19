// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
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

#define N   200
#define HUGE_MSHR 100000

namespace SST{ namespace MemHierarchy{
    using namespace SST::MemHierarchy;
    using namespace std;

Cache* Cache::cacheFactory(ComponentId_t _id, Params &_params){
 
    /* --------------- Output Class --------------- */
    Output* dbg = new Output();
    int debugLevel = _params.find_integer("debug_level", 0);
    if(debugLevel < 0 || debugLevel > 10)     _abort(Cache, "Debugging level must be betwee 0 and 10. \n");
    
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
    int uncachedRequests        = _params.find_integer("uncache_all_requests", 0);
    bool L1 = (L1int == 1);
    vector<int> statGroupIds;
    
    istringstream ss(statGroups);
    string token;
    
    statGroupIds.push_back(0);         //Id = 0 prints overall statistics

    while(getline(ss, token, ',')) {
        statGroupIds.push_back(atoi(token.c_str()));
    }

    /* Check user specified all required fields */
    if(frequency.empty())           _abort(Cache, "No cache frequency specified (usually frequency = cpu frequency).\n");
    if(-1 >= associativity)         _abort(Cache, "Associativity was not specified.\n");
    if(sizeStr.empty())             _abort(Cache, "Cache size was not specified. \n")
    if(-1 == lineSize)              _abort(Cache, "Line size was not specified (blocksize).\n");
    if(L1int != 1 && L1int != 0)    _abort(Cache, "Not specified whether cache is L1 (0 or 1)\n");
    if(accessLatency == -1 )        _abort(Cache, "Access time not specified\n");
    if(dirAtNextLvl > 1 ||
       dirAtNextLvl < 0)            _abort(Cache, "Did not specified correctly where there exists a directory controller at higher level cache");
    long cacheSize = SST::MemHierarchy::convertToBytes(sizeStr);
    
    /* NACKing to from L1 to the CPU doesnt really happen in CPUs*/
    if(L1 && mshrSize != -1)    _abort(Cache, "The parameter 'mshr_num_entries' is not valid in an L1 since MemHierarchy"
                                "assumes an L1 MSHR size matches the size of the load/store queue unit of the CPU.  An L1"
                                "will always be available to the CPU");
    if(-1 == mshrSize)          mshrSize = HUGE_MSHR;
    
    /* No L2+ cache should realistically have an MSHR that is less than 10-16 entries */
    if(10 > mshrSize)           _abort(Cache, "MSHR should be at least of 10 MSHR entries long");

    
    /* ---------------- Initialization ----------------- */
    HashFunction* ht = new PureIdHashFunction;
    boost::algorithm::to_lower(coherenceProtocol);
    boost::algorithm::to_lower(replacement);
    
    uint numLines = cacheSize/lineSize;
    uint protocol = 0;
    
    if(coherenceProtocol == "mesi")      protocol = 1;
    else if(coherenceProtocol == "msi")  protocol = 0;
    else
        _abort(Cache, "Not supported protocol\n");

    ReplacementMgr* replManager;
    if (boost::iequals(replacement, "lru"))         replManager = new LRUReplacementMgr(dbg, numLines, true);
    else if (boost::iequals(replacement, "lfu"))    replManager = new LFUReplacementMgr(dbg, numLines);
    else if (boost::iequals(replacement, "random")) replManager = new RandomReplacementMgr(dbg, associativity);
    else if (boost::iequals(replacement, "mru"))    replManager = new MRUReplacementMgr(dbg, numLines, true);
    else _abort(Cache, "Replacement policy was not entered correctly or is not supported.\n");
    
    CacheArray* cacheArray = new SetAssociativeArray(dbg, cacheSize, lineSize, associativity, replManager, ht, !L1);

    CacheConfig config = {frequency, cacheArray, protocol, dbg, replManager, numLines, lineSize, mshrSize, L1, dirAtNextLvl, statGroupIds, uncachedRequests};
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
    string prefetcher   = _params.find_string("prefetcher");
    mshrLatency_        = _params.find_integer("mshr_latency_cycles", 0);
    
    
    /* --------------- Prefetcher ---------------*/
    if (prefetcher.empty()) listener_ = new CacheListener();
    else {
        listener_ = dynamic_cast<CacheListener*>(loadModule(prefetcher, _params));
        assert(listener_);
    }
    listener_->setOwningComponent(this);
    listener_->registerResponseCallback(new Event::Handler<Cache>(this, &Cache::handlePrefetchEvent));

    
    /* ---------------- Latency ---------------- */
    assert(accessLatency_ >= 1);
    if(mshrLatency_ < 1) intrapolateMSHRLatency();
    
    assert(mshrLatency_ >= 1 && mshrLatency_ <= 200);



    /* ----------------- MSHR ----------------- */
    mshr_               = new MSHR(this, cf_.MSHRSize_);
    mshrUncached_       = new MSHR(this, HUGE_MSHR);
    
    /* ---------------- Links ---------------- */
    lowNetPorts_        = new vector<Link*>();
    highNetPorts_       = new vector<Link*>();
    
    /* ---------------- Clock ---------------- */
    clockHandler_       = new Clock::Handler<Cache>(this, &Cache::clockTick);
    defaultTimeBase_    = registerClock(cf_.cacheFrequency_, clockHandler_);
    
    registerTimeBase("2 ns", true);       //  TODO:  Is this right?

    /* ---------------- Directory Controller --------------- */
    if (cf_.dirControllerExists_) {
        assert(isPortConnected("directory"));
        MemNIC::ComponentInfo myInfo;
        myInfo.link_port = "directory";
        myInfo.link_bandwidth = _params.find_string("network_bw", "1GB/s");
		myInfo.num_vcs = _params.find_integer("network_num_vc", 3);
        myInfo.name = getName();
        myInfo.network_addr = _params.find_integer("network_address");
        myInfo.type = MemNIC::TypeCache;
        myInfo.typeInfo.cache.blocksize = cf_.lineSize_;
        myInfo.typeInfo.cache.num_blocks = cf_.numLines_;

        directoryLink_ = new MemNIC(this, myInfo, new Event::Handler<Cache>(this, &Cache::processIncomingEvent));
    } else {
        directoryLink_ = NULL;
    }
    
    
    /* ------------- Member variables intialization ------------- */
    
    configureLinks();
    groupStats_                         = (cf_.statGroupIds_.size() < 2) ? false : true;
    clockOn_                            = true;
    idleCount_                          = 0;
    memNICIdleCount_                    = 0;
    memNICIdle_                         = false;
    timestamp_                          = 0;
    totalUpgradeLatency_                = 0;
    mshrHits_                           = 0;
    
    if(groupStats_){
        for(unsigned int i = 0; i < cf_.statGroupIds_.size(); i++)
            stats_[cf_.statGroupIds_[i]].initialize();
    }
        
    /* --------------- Coherence Controllers --------------- */
    sharersAware_ = (L1_) ? false : true;
    topCC_ = (!L1_) ? new MESITopCC(this, d_, cf_.protocol_, cf_.numLines_, cf_.lineSize_, accessLatency_, mshrLatency_, highNetPorts_) :
                      new TopCacheController(this, d_, cf_.lineSize_, accessLatency_, mshrLatency_, highNetPorts_);
    bottomCC_ = new MESIBottomCC(this, this->getName(), d_, lowNetPorts_, listener_, cf_.lineSize_, accessLatency_, mshrLatency_, L1_, directoryLink_, groupStats_, cf_.statGroupIds_);
   
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

    if(!cf_.dirControllerExists_){
        for(uint id = 0 ; id < 200; id++) {
            string linkName = "low_network_" + boost::lexical_cast<std::string>(id);
            SST::Link* link = configureLink(linkName, "50ps", new Event::Handler<Cache>(this, &Cache::processIncomingEvent));
            if(link){
                d_->debug(_INFO_,"Low Netowork Link ID: %u \n", (uint)link->getId());
                lowNetPorts_->push_back(link);
                lowNetExists = true;
            }else break;
            
        }
    }

    for(uint id = 0 ; id < 200; id++) {
        string linkName = "high_network_" + boost::lexical_cast<std::string>(id);
        SST::Link* link = configureLink(linkName, "50ps", new Event::Handler<Cache>(this, &Cache::processIncomingEvent));  //TODO: fix
        if(link) {
            d_->debug(_INFO_,"High Network Link ID: %u \n", (uint)link->getId());
            highNetPorts_->push_back(link);
            highNetCount++;
        }else break;
    }
    
    
    if(!cf_.dirControllerExists_) BOOST_ASSERT_MSG(lowNetExists, buf);
    BOOST_ASSERT_MSG(highNetCount > 0,  buf2);
    BOOST_ASSERT_MSG(highNetCount < 2,  buf3);
    selfLink_ = configureSelfLink("Self", "50ps", new Event::Handler<Cache>(this, &Cache::handleSelfEvent));
}



void Cache::intrapolateMSHRLatency(){
    int y[N];

    if(L1_){
        mshrLatency_ = 1;
        return;
    }
    
    /* L2 */
    assert_msg(accessLatency_ > 4, "L2 should have a latency bigger than 4 cycles.");
    y[0] = 0;
    y[1] = 1;
    for(int idx = 2;  idx < 12; idx++) y[idx] = 2;
    for(int idx = 13; idx < 16; idx++) y[idx] = 3;
    for(int idx = 17; idx < 26; idx++) y[idx] = 5;

    
    /* L3 */
    for(int idx = 27; idx < 46; idx++) y[idx] = 19;
    for(int idx = 47; idx < 68; idx++) y[idx] = 26;
    for(int idx = 69; idx < N;  idx++) y[idx] = 32;
    
    assert_msg(accessLatency_ > 4, "Cache access latencies greater than 200 cycles not supported.");

    assert(accessLatency_ >= 0 && accessLatency_ <= N);
    mshrLatency_ = y[accessLatency_];
        assert(mshrLatency_   >= 1 && mshrLatency_ <= 200);



}

}}
