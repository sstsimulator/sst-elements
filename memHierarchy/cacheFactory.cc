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
#include "L1CoherenceController.h"
#include "L1IncoherentController.h"
#include "MESICoherenceController.h"
#include "MESIInternalDirectory.h"
#include "IncoherentController.h"


namespace SST{ namespace MemHierarchy{
    using namespace SST::MemHierarchy;
    using namespace std;

Cache* Cache::cacheFactory(ComponentId_t id, Params &params){
 
    /* --------------- Output Class --------------- */
    Output* dbg = new Output();
    int debugLevel = params.find_integer("debug_level", 0);
    
    dbg->init("--->  ", debugLevel, 0,(Output::output_location_t)params.find_integer("debug", 0));
    if(debugLevel < 0 || debugLevel > 10)     dbg->fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");
    dbg->debug(_INFO_,"\n--------------------------- Initializing [Memory Hierarchy] --------------------------- \n\n");

    /* --------------- Get Parameters --------------- */
    string frequency            = params.find_string("cache_frequency", "" );            //Hertz
    string replacement          = params.find_string("replacement_policy", "LRU");
    int associativity           = params.find_integer("associativity", -1);
    int hashFunc                = params.find_integer("hash_function", 0);
    string sizeStr              = params.find_string("cache_size", "");                  //Bytes
    int lineSize                = params.find_integer("cache_line_size", -1);            //Bytes
    int accessLatency           = params.find_integer("access_latency_cycles", -1);      //ns
    int mshrSize                = params.find_integer("mshr_num_entries", -1);           //number of entries
    string preF                 = params.find_string("prefetcher");
    int L1int                   = params.find_integer("L1", 0);
    int LLCint                  = params.find_integer("LLC", 0);
    int LLint                   = params.find_integer("LL", 0);
    int dirAtNextLvl            = params.find_integer("directory_at_next_level", 0);
    string coherenceProtocol    = params.find_string("coherence_protocol", "");
    string statGroups           = params.find_string("stat_group_ids", "");
    string bottomNetwork        = params.find_string("bottom_network", "");
    string topNetwork           = params.find_string("top_network", "");
    int noncacheableRequests    = params.find_integer("force_noncacheable_reqs", 0);
    string cacheType            = params.find_string("cache_type", "inclusive");
    SimTime_t maxWaitTime       = params.find_integer("maxRequestDelay", 0);  // Nanoseconds
    string dirReplacement       = params.find_string("noninclusive_directory_repl", "LRU");
    int dirAssociativity        = params.find_integer("noninclusive_directory_associativity", 1);
    int dirNumEntries           = params.find_integer("noninclusive_directory_entries", 0);
    bool L1 = (L1int == 1);
    bool LLC = (LLCint == 1);
    bool LL = (LLint == 1);
    vector<int> statGroupIds;
    
    istringstream ss(statGroups);
    string token;
    
    statGroupIds.push_back(0);         //Id = 0 prints overall statistics

    while(getline(ss, token, ',')) {
        statGroupIds.push_back(atoi(token.c_str()));
    }

    /* Convert all strings to lower case */
    boost::algorithm::to_lower(coherenceProtocol);
    boost::algorithm::to_lower(replacement);
    boost::algorithm::to_lower(dirReplacement);
    boost::algorithm::to_lower(bottomNetwork);
    boost::algorithm::to_lower(topNetwork);
    boost::algorithm::to_lower(cacheType);

    /* Check user specified all required fields */
    if(frequency.empty())           dbg->fatal(CALL_INFO, -1, "Param not specified: frequency - cache frequency.\n");
    if(-1 >= associativity)         dbg->fatal(CALL_INFO, -1, "Param not specified: associativity\n");
    if(sizeStr.empty())             dbg->fatal(CALL_INFO, -1, "Param not specified: cache_size\n");
    if(-1 == lineSize)              dbg->fatal(CALL_INFO, -1, "Param not specified: cache_line_size - number of bytes in a cacheline (block size)\n");
    if(L1int != 1 && L1int != 0)    dbg->fatal(CALL_INFO, -1, "Param not specified: L1 - should be '1' if cache is an L1, 0 otherwise\n");
    if(LLCint != 1 && LLCint != 0)  dbg->fatal(CALL_INFO, -1, "Param not specified: LLC - should be '1' if cache is an LLC, 0 otherwise\n");
    if(LLint != 1 && LLint != 0)    dbg->fatal(CALL_INFO, -1, "Param not specified: LL - should be '1' if cache is the lowest level coherence entity (e.g., LLC and no directory below), 0 otherwise\n");
    if(accessLatency == -1 )        dbg->fatal(CALL_INFO, -1, "Param not specified: access_latency_cycles - access time for cache\n");
    
    /* Check that parameters are valid */
    if(dirAtNextLvl > 1 || dirAtNextLvl < 0)    
        dbg->fatal(CALL_INFO, -1, "Invalid param: directory_at_next_level - should be '1' if directory exists at next level below this cache, 0 otherwise. You specified '%d'.\n", dirAtNextLvl);
    if(!(bottomNetwork == "" || bottomNetwork == "directory" || bottomNetwork == "cache"))  
        dbg->fatal(CALL_INFO, -1, "Invalid param: bottom_network - valid options are '', 'directory', or 'cache'. You specified '%s'.\n", bottomNetwork.c_str());
    if(!(topNetwork == "" || topNetwork == "cache"))    
        dbg->fatal(CALL_INFO, -1, "Invalid param: top_network - valid options are '' or 'cache'. You specified '%s'\n", topNetwork.c_str());
    if (cacheType != "inclusive" && cacheType != "noninclusive" && cacheType != "noninclusive_with_directory") 
        dbg->fatal(CALL_INFO, -1, "Invalid param: cache_type - valid options are 'inclusive' or 'noninclusive' or 'noninclusive_with_directory'. You specified '%s'.\n", cacheType.c_str());
    if (L1 && cacheType != "inclusive") 
        dbg->fatal(CALL_INFO, -1, "Invalid param: cache_type - must be 'inclusive' for an L1. You specified '%s'.\n", cacheType.c_str());
    if (cacheType == "noninclusive_with_directory") {
        if (dirAssociativity <= -1) dbg->fatal(CALL_INFO, -1, "Param not specified: directory_associativity - this must be specified if cache_type is noninclusive_with_directory. You specified %d\n", dirAssociativity);
        if (dirNumEntries <= 0)     dbg->fatal(CALL_INFO, -1, "Invalid param: noninlusive_directory_entries - must be at least 1 if cache_type is noninclusive_with_directory. You specified %d\n", dirNumEntries);
    }

    /* NACKing to from L1 to the CPU doesnt really happen in CPUs*/
    if(L1 && mshrSize != -1)    dbg->fatal(CALL_INFO, -1, "Invalid param: mshr_num_entries - must be -1 for L1s, memHierarchy assumes L1 MSHR is sized to match the CPU's load/store queue. You specified %d\n", mshrSize);
    
    /* No L2+ cache should realistically have an MSHR that is less than 10-16 entries */
    if(-1 == mshrSize) mshrSize = HUGE_MSHR;
    if(mshrSize < 2)            dbg->fatal(CALL_INFO, -1, "Invalid param: mshr_num_entries - MSHR requires at least 2 entries to avoid deadlock. You specified %d\n", mshrSize);

    /* Update parameters for initialization */
    if (dirAtNextLvl) bottomNetwork = "directory";
    
    /* ---------------- Initialization ----------------- */
    HashFunction* ht;
    if (hashFunc == 1) {
      ht = new LinearHashFunction;
    } else if (hashFunc == 2) {
      ht = new XorHashFunction;
    } else {
      ht = new PureIdHashFunction;
    }
    
    long cacheSize  = SST::MemHierarchy::convertToBytes(sizeStr);
    uint numLines = cacheSize/lineSize;
    uint protocol = 0;
    
    if(coherenceProtocol == "mesi")      protocol = 1;
    else if(coherenceProtocol == "msi")  protocol = 0;
    else if (coherenceProtocol == "none") protocol = 2;
    else dbg->fatal(CALL_INFO,-1, "Invalid param: coherence_protocol - must be 'msi', 'mesi', or 'none'\n");

    CacheArray * cacheArray = NULL;
    CacheArray * dirArray = NULL;
    ReplacementMgr* replManager = NULL;
    ReplacementMgr* dirReplManager = NULL;
    if (cacheType == "inclusive" || cacheType == "noninclusive") {
        if (boost::iequals(replacement, "lru")) replManager = new LRUReplacementMgr(dbg, numLines, associativity, true);
        else if (boost::iequals(replacement, "lfu"))    replManager = new LFUReplacementMgr(dbg, numLines, associativity);
        else if (boost::iequals(replacement, "random")) replManager = new RandomReplacementMgr(dbg, associativity);
        else if (boost::iequals(replacement, "mru"))    replManager = new MRUReplacementMgr(dbg, numLines, associativity, true);
        else if (boost::iequals(replacement, "nmru"))   replManager = new NMRUReplacementMgr(dbg, numLines, associativity);
        else dbg->fatal(CALL_INFO, -1, "Invalid param: replacement_policy - supported policies are 'lru', 'lfu', 'random', 'mru', and 'nmru'. You specified %s.\n", replacement.c_str());
        cacheArray = new SetAssociativeArray(dbg, numLines, lineSize, associativity, replManager, ht, !L1);
    } else if (cacheType == "noninclusive_with_directory") {
        if (boost::iequals(replacement, "lru")) replManager = new LRUReplacementMgr(dbg, numLines, associativity, true);
        else if (boost::iequals(replacement, "lfu"))    replManager = new LFUReplacementMgr(dbg, numLines, associativity);
        else if (boost::iequals(replacement, "random")) replManager = new RandomReplacementMgr(dbg, associativity);
        else if (boost::iequals(replacement, "mru"))    replManager = new MRUReplacementMgr(dbg, numLines, associativity, true);
        else if (boost::iequals(replacement, "nmru"))   replManager = new NMRUReplacementMgr(dbg, numLines, associativity);
        else dbg->fatal(CALL_INFO, -1, "Invalid param: replacement_policy - supported policies are 'lru', 'lfu', 'random', 'mru', and 'nmru'. You specified %s.\n", replacement.c_str());
        if (boost::iequals(dirReplacement, "lru"))          dirReplManager = new LRUReplacementMgr(dbg, dirNumEntries, dirAssociativity, true);
        else if (boost::iequals(dirReplacement, "lfu"))     dirReplManager = new LFUReplacementMgr(dbg, dirNumEntries, dirAssociativity);
        else if (boost::iequals(dirReplacement, "random"))  dirReplManager = new RandomReplacementMgr(dbg, dirAssociativity);
        else if (boost::iequals(dirReplacement, "mru"))     dirReplManager = new MRUReplacementMgr(dbg, dirNumEntries, dirAssociativity, true);
        else if (boost::iequals(dirReplacement, "nmru"))    dirReplManager = new NMRUReplacementMgr(dbg, dirNumEntries, dirAssociativity);
        else dbg->fatal(CALL_INFO, -1, "Invalid param: directory_replacement_policy - supported policies are 'lru', 'lfu', 'random', 'mru', and 'nmru'. You specified %s.\n", replacement.c_str());
        cacheArray = new DualSetAssociativeArray(dbg, static_cast<uint>(lineSize), ht, true, dirNumEntries, dirAssociativity, dirReplManager, numLines, associativity, replManager);
    }
    
    // Auto-detect LLC if directory is present
    if (bottomNetwork == "directory") LLC = true; 

    CacheConfig config = {frequency, cacheArray, dirArray, protocol, dbg, replManager, numLines,
	static_cast<uint>(lineSize),
	static_cast<uint>(mshrSize), L1, LLC, LL, bottomNetwork, topNetwork, statGroupIds,
	static_cast<bool>(noncacheableRequests), maxWaitTime, cacheType};
    return new Cache(id, params, config);
}



Cache::Cache(ComponentId_t id, Params &params, CacheConfig config) : Component(id){
    cf_     = config;
    d_      = cf_.dbg_;
    L1_     = cf_.L1_;
    LLC_    = cf_.LLC_;
    LL_     = cf_.LL_;
    d_->debug(_INFO_,"--------------------------- Initializing [Cache]: %s... \n", this->Component::getName().c_str());
    pMembers();
    errorChecking();
    
    d2_ = new Output();
    d2_->init("", params.find_integer("debug_level", 0), 0,(Output::output_location_t)params.find_integer("debug", 0));

    statsFile_          = params.find_integer("statistics", 0);
    idleMax_            = params.find_integer("idle_max", 10000);
    accessLatency_      = params.find_integer("access_latency_cycles", -1);
    tagLatency_         = params.find_integer("tag_access_latency_cycles",accessLatency_);
    string prefetcher   = params.find_string("prefetcher");
    mshrLatency_        = params.find_integer("mshr_latency_cycles", 0);
    int cacheSliceCount         = params.find_integer("num_cache_slices", 1);
    int sliceID                 = params.find_integer("slice_id", 0);
    string sliceAllocPolicy     = params.find_string("slice_allocation_policy", "rr");
    bool snoopL1Invs    = false;
    if (L1_) snoopL1Invs = (params.find_integer("snoop_l1_invalidations", 0)) ? true : false;
    int dAddr           = params.find_integer("debug_addr",-1);
    if (dAddr != -1) DEBUG_ALL = false;
    else DEBUG_ALL = true;
    DEBUG_ADDR = (Addr)dAddr;
    int lowerIsNoninclusive        = params.find_integer("lower_is_noninclusive", 0);
    
    /* --------------- Check parameters -------------*/
    if (accessLatency_ < 1) d_->fatal(CALL_INFO,-1, "%s, Invalid param: access_latency_cycles - must be at least 1. You specified %" PRIu64 "\n", 
            this->Component::getName().c_str(), accessLatency_);
   
    if (cf_.topNetwork_ == "cache") {
        if (cacheSliceCount == 1) sliceID = 0;
        else if (cacheSliceCount > 1) {
            if (sliceID >= cacheSliceCount) d_->fatal(CALL_INFO,-1, "%s, Invalid param: slice_id - should be between 0 and num_cache_slices-1. You specified %d.\n",
                    this->Component::getName().c_str(), sliceID);
            if (sliceAllocPolicy != "rr") d_->fatal(CALL_INFO,-1, "%s, Invalid param: slice_allocation_policy - supported policy is 'rr' (round-robin). You specified %s.\n",
                    this->Component::getName().c_str(), sliceAllocPolicy.c_str());
        } else {
            d2_->fatal(CALL_INFO, -1, "%s, Invalid param: num_cache_slices - should be 1 or greater. You specified %d.\n", 
                    this->Component::getName().c_str(), cacheSliceCount);
        }
    }

    /* --------------- Prefetcher ---------------*/
    if (prefetcher.empty()) {
	Params emptyParams;
	listener_ = new CacheListener(this, emptyParams);
    } else {
	Params prefetcherParams = params.find_prefix_params("prefetcher." );
	listener_ = dynamic_cast<CacheListener*>(loadSubComponent(prefetcher, this, prefetcherParams));
    }

    listener_->registerResponseCallback(new Event::Handler<Cache>(this, &Cache::handlePrefetchEvent));

    /* ---------------- Latency ---------------- */
    if(mshrLatency_ < 1) intrapolateMSHRLatency();
    
    /* ----------------- MSHR ----------------- */
    mshr_               = new MSHR(d_, cf_.MSHRSize_, this->getName(), DEBUG_ALL, DEBUG_ADDR);
    mshrNoncacheable_   = new MSHR(d_, HUGE_MSHR, this->getName(), DEBUG_ALL, DEBUG_ADDR);
    
    /* ---------------- Links ---------------- */
    lowNetPorts_        = new vector<Link*>();
    highNetPorts_       = new vector<Link*>();

    /* ---------------- Clock ---------------- */
    clockHandler_       = new Clock::Handler<Cache>(this, &Cache::clockTick);
    defaultTimeBase_    = registerClock(cf_.cacheFrequency_, clockHandler_);
    
    registerTimeBase("2 ns", true);       //  TODO:  Is this right?

    /* ---------------- Memory NICs --------------- */
    if (cf_.bottomNetwork_ == "directory" && cf_.topNetwork_ == "") { // cache with a dir below, direct connection to cache above
        if (!isPortConnected("directory")) {
            d_->fatal(CALL_INFO,-1,"%s, Error initializing memNIC for cache: directory port is not conected.\n", 
                    this->Component::getName().c_str());
        }
        MemNIC::ComponentInfo myInfo;
        myInfo.link_port = "directory";
        myInfo.link_bandwidth = params.find_string("network_bw", "1GB/s");
	myInfo.num_vcs = params.find_integer("network_num_vc", 3);
        myInfo.name = getName();
        myInfo.network_addr = params.find_integer("network_address");
        myInfo.type = MemNIC::TypeCache; 
        myInfo.link_inbuf_size = params.find_string("network_input_buffer_size", "1KB");
        myInfo.link_outbuf_size = params.find_string("network_output_buffer_size", "1KB");

        MemNIC::ComponentTypeInfo typeInfo;
        typeInfo.blocksize = cf_.lineSize_;

        bottomNetworkLink_ = new MemNIC(this, myInfo, new Event::Handler<Cache>(this, &Cache::processIncomingEvent));
        bottomNetworkLink_->addTypeInfo(typeInfo);

        topNetworkLink_ = NULL;
    } else if (cf_.bottomNetwork_ == "cache" && cf_.topNetwork_ == "") { // cache with another cache below it on the network
        if (!isPortConnected("cache")) {
            d_->fatal(CALL_INFO,-1,"%s, Error initializing memNIC for cache: cache port is not conected.\n", 
                    this->Component::getName().c_str());
        }
        MemNIC::ComponentInfo myInfo;
        myInfo.link_port = "cache";
        myInfo.link_bandwidth = params.find_string("network_bw", "1GB/s");
	myInfo.num_vcs = params.find_integer("network_num_vc", 3);
        myInfo.name = getName();
        myInfo.network_addr = params.find_integer("network_address");
        myInfo.type = MemNIC::TypeCacheToCache; 
        myInfo.link_inbuf_size = params.find_string("network_input_buffer_size", "1KB");
        myInfo.link_outbuf_size = params.find_string("network_output_buffer_size", "1KB");

        MemNIC::ComponentTypeInfo typeInfo;
        typeInfo.blocksize = cf_.lineSize_;

        bottomNetworkLink_ = new MemNIC(this, myInfo, new Event::Handler<Cache>(this, &Cache::processIncomingEvent));
        bottomNetworkLink_->addTypeInfo(typeInfo);
        
        topNetworkLink_ = NULL;
    } else if (cf_.bottomNetwork_ == "directory" && cf_.topNetwork_ == "cache") {
        if (!isPortConnected("directory")) {
            d_->fatal(CALL_INFO,-1,"%s, Error initializing memNIC for cache: directory port is not conected.\n", 
                    this->Component::getName().c_str());
        }
        
        MemNIC::ComponentInfo myInfo;
        myInfo.link_port = "directory";
        myInfo.link_bandwidth = params.find_string("network_bw", "1GB/s");
	myInfo.num_vcs = params.find_integer("network_num_vc", 3);
        myInfo.name = getName();
        myInfo.network_addr = params.find_integer("network_address");
        myInfo.type = MemNIC::TypeNetworkCache; 
        myInfo.link_inbuf_size = params.find_string("network_input_buffer_size", "1KB");
        myInfo.link_outbuf_size = params.find_string("network_output_buffer_size", "1KB");

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


    /* Register statistics */
    statCacheHits               = registerStatistic<uint64_t>("CacheHits");
    statGetSHitOnArrival        = registerStatistic<uint64_t>("GetSHit_Arrival");   
    statGetXHitOnArrival        = registerStatistic<uint64_t>("GetXHit_Arrival");
    statGetSExHitOnArrival      = registerStatistic<uint64_t>("GetSExHit_Arrival"); 
    statGetSHitAfterBlocked     = registerStatistic<uint64_t>("GetSHit_Blocked");
    statGetXHitAfterBlocked     = registerStatistic<uint64_t>("GetXHit_Blocked");  
    statGetSExHitAfterBlocked   = registerStatistic<uint64_t>("GetSExHit_Blocked");
    statCacheMisses             = registerStatistic<uint64_t>("CacheMisses");
    statGetSMissOnArrival       = registerStatistic<uint64_t>("GetSMiss_Arrival");
    statGetXMissOnArrival       = registerStatistic<uint64_t>("GetXMiss_Arrival");
    statGetSExMissOnArrival     = registerStatistic<uint64_t>("GetSExMiss_Arrival");
    statGetSMissAfterBlocked    = registerStatistic<uint64_t>("GetSMiss_Blocked");
    statGetXMissAfterBlocked    = registerStatistic<uint64_t>("GetXMiss_Blocked");
    statGetSExMissAfterBlocked  = registerStatistic<uint64_t>("GetSExMiss_Blocked");
    statGetS_recv               = registerStatistic<uint64_t>("GetS_recv");
    statGetX_recv               = registerStatistic<uint64_t>("GetX_recv");
    statGetSEx_recv             = registerStatistic<uint64_t>("GetSEx_recv");
    statGetSResp_recv           = registerStatistic<uint64_t>("GetSResp_recv");
    statGetXResp_recv           = registerStatistic<uint64_t>("GetXResp_recv");
    statPutS_recv               = registerStatistic<uint64_t>("PutS_recv");
    statPutM_recv               = registerStatistic<uint64_t>("PutM_recv");
    statPutE_recv               = registerStatistic<uint64_t>("PutE_recv");
    statFetchInv_recv           = registerStatistic<uint64_t>("FetchInv_recv");
    statFetchInvX_recv          = registerStatistic<uint64_t>("FetchInvX_recv");
    statInv_recv                = registerStatistic<uint64_t>("Inv_recv");
    statNACK_recv               = registerStatistic<uint64_t>("NACK_recv");

    if(groupStats_){
        for(unsigned int i = 0; i < cf_.statGroupIds_.size(); i++) {
            if (i > 0 && (cf_.statGroupIds_[i] == 0 || cf_.statGroupIds_[i] == -1)) 
                d_->fatal(CALL_INFO,-1, "Invalid param: stat_group_ids - custom IDs cannot be 0 or -1\n");
            stats_[cf_.statGroupIds_[i]].initialize();
        }
    }
        
    /* --------------- Coherence Controllers --------------- */
    coherenceMgr = NULL;
    bool inclusive = cf_.type_ == "inclusive";
    if (!L1_) {
        if (cf_.protocol_ != 2) {
            if (cf_.type_ != "noninclusive_with_directory") {
                coherenceMgr = new MESIController(this, this->getName(), d_, lowNetPorts_, highNetPorts_, listener_, cf_.lineSize_, accessLatency_, tagLatency_, mshrLatency_, LLC_, LL_, mshr_, cf_.protocol_, 
                    inclusive, lowerIsNoninclusive, bottomNetworkLink_, topNetworkLink_, groupStats_, cf_.statGroupIds_, DEBUG_ALL, DEBUG_ADDR);
            } else {
                coherenceMgr = new MESIInternalDirectory(this, this->getName(), d_, lowNetPorts_, highNetPorts_, listener_, cf_.lineSize_, accessLatency_, tagLatency_, mshrLatency_, LLC_, LL_, mshr_, cf_.protocol_,
                        lowerIsNoninclusive, bottomNetworkLink_, topNetworkLink_, groupStats_, cf_.statGroupIds_, DEBUG_ALL, DEBUG_ADDR);
            }
        } else {
            coherenceMgr = new IncoherentController(this, this->getName(), d_, lowNetPorts_, highNetPorts_, listener_, cf_.lineSize_, accessLatency_, tagLatency_, mshrLatency_, LLC_, LL_, mshr_,
                    inclusive, lowerIsNoninclusive, bottomNetworkLink_, topNetworkLink_, groupStats_, cf_.statGroupIds_, DEBUG_ALL, DEBUG_ADDR);
        }
    } else {
        if (cf_.protocol_ != 2) {
            coherenceMgr = new L1CoherenceController(this, this->getName(), d_, lowNetPorts_, highNetPorts_, listener_, cf_.lineSize_, accessLatency_, tagLatency_, mshrLatency_, LLC_, LL_, mshr_, cf_.protocol_, 
                lowerIsNoninclusive, bottomNetworkLink_, topNetworkLink_, groupStats_, cf_.statGroupIds_, DEBUG_ALL, DEBUG_ADDR, snoopL1Invs);
        } else {
            coherenceMgr = new L1IncoherentController(this, this->getName(), d_, lowNetPorts_, highNetPorts_, listener_, cf_.lineSize_, accessLatency_, tagLatency_, mshrLatency_, LLC_, LL_, mshr_, 
                    lowerIsNoninclusive, bottomNetworkLink_, topNetworkLink_, groupStats_, cf_.statGroupIds_, DEBUG_ALL, DEBUG_ADDR);
        }
    }
    
    /*---------------  Misc --------------- */
}


void Cache::configureLinks(){
    int highNetCount = 0;
    bool lowNetExists = false;

    if(cf_.bottomNetwork_ == "") {
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

    if (cf_.bottomNetwork_ == "" && !lowNetExists) d_->fatal(CALL_INFO, -1, "%s, Error: Low network port was not specified correctly. Please name ports 'low_network_x' where x is the port number and starts at 0\n", this->getName().c_str());
    if (cf_.topNetwork_ == "") {
        if (highNetCount < 1) d_->fatal(CALL_INFO, -1, "%s, Error: High network port was not specified correctly. Please name ports 'high_network_x' where x is the port number and starts at 0\n", this->getName().c_str());
        if (highNetCount > 1) d_->fatal(CALL_INFO, -1, "%s, Error: More than one high network port specified. Please use a 'Bus' component when connecting more than one higher level cache (e.g., 2 L1s to 1 L2)\n", this->getName().c_str());
    }

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
    for(uint64 idx = 12; idx < 16; idx++) y[idx] = 3;
    for(uint64 idx = 16; idx < 26; idx++) y[idx] = 5;

    
    /* L3 */
    for(uint64 idx = 26; idx < 46; idx++) y[idx] = 19;
    for(uint64 idx = 46; idx < 68; idx++) y[idx] = 26;
    for(uint64 idx = 68; idx < N;  idx++) y[idx] = 32;
    
    if (accessLatency_ > N) {
        d_->fatal(CALL_INFO, -1, "%s, Error: cannot intrapolate MSHR latency if cache latency > 200. Cache latency: %" PRIu64 "\n", getName().c_str(), accessLatency_);
    }
    mshrLatency_ = y[accessLatency_];

}

}}
