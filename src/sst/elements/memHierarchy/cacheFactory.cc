// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
#include <sst/core/stringize.h>
#include "hash.h"
#include "cacheController.h"
#include "util.h"
#include "cacheListener.h"
#include <sst/core/params.h>
#include "mshr.h"
#include "L1CoherenceController.h"
#include "L1IncoherentController.h"


namespace SST{ namespace MemHierarchy{
    using namespace SST::MemHierarchy;
    using namespace std;

Cache* Cache::cacheFactory(ComponentId_t id, Params &params) {
 
    /* --------------- Output Class --------------- */
    Output* dbg = new Output();
    int debugLevel = params.find<int>("debug_level", 1);
    
    dbg->init("--->  ", debugLevel, 0,(Output::output_location_t)params.find<int>("debug", 0));
    if (debugLevel < 0 || debugLevel > 10)     dbg->fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");
    dbg->debug(_INFO_,"\n--------------------------- Initializing [Memory Hierarchy] --------------------------- \n\n");

    // Find deprecated parameters and warn/fatal
    // Currently deprecated parameters are: 'LLC', statistcs, network_num_vc, directory_at_next_level, bottom_network, top_network
    Output out("", 1, 0, Output::STDOUT);
    bool found;
    params.find<int>("LL", 0, found);
    if (found) {
        out.output("cacheFactory, ** Found deprecated parameter: LL ** The value of this parameter is now auto-detected. Remove this parameter from your input deck to eliminate this message.\n");
    }
    params.find<int>("LLC", 0, found);
    if (found) {
        out.output("cacheFactory, ** Found deprecated parameter: LLC ** The value of this parameter is now auto-detected. Remove this parameter from your input deck to eliminate this message.\n");
    }
    params.find<int>("bottom_network", 0, found);
    if (found) {
        out.output("cacheFactory, ** Found deprecated parameter: bottom_network ** The value of this parameter is now auto-detected. Remove this parameter from your input deck to eliminate this message.\n");
    }
    params.find<int>("top_network", 0, found);
    if (found) {
        out.output("cacheFactory, ** Found deprecated parameter: top_network ** The value of this parameter is now auto-detected. Remove this parameter from your input deck to eliminate this message.\n");
    }
    params.find<int>("directory_at_next_level", 0, found);
    if (found) {
        out.output("cacheFactory, ** Found deprecated parameter: directory_at_next_level ** The value of this parameter is now auto-detected. Remove this parameter from your input deck to eliminate this message.\n");
    }
    params.find<int>("network_num_vc", 0, found);
    if (found) {
        out.output("cacheFactory, ** Found deprecated parameter: network_num_vc ** MemHierarchy does not use multiple virtual channels. Remove this parameter from your input deck to eliminate this message.\n");
    }
    params.find<int>("lower_is_noninclusive", 0, found);
    if (found) {
        out.output("cacheFactory, ** Found deprecated parameter: lower_is_noninclusive ** The value of this parameter is now auto-detected. Remove this parameter from your input deck to eliminate this message.\n");
    }

    /* --------------- Get Parameters --------------- */
    string frequency            = params.find<std::string>("cache_frequency", "" );            //Hertz
    string replacement          = params.find<std::string>("replacement_policy", "LRU");
    int associativity           = params.find<int>("associativity", -1);
    int hashFunc                = params.find<int>("hash_function", 0);
    string sizeStr              = params.find<std::string>("cache_size", "");                  //Bytes
    int lineSize                = params.find<int>("cache_line_size", 64);            //Bytes
    int accessLatency           = params.find<int>("access_latency_cycles", -1);      //ns
    int mshrSize                = params.find<int>("mshr_num_entries", -1);           //number of entries
    string preF                 = params.find<std::string>("prefetcher");
    bool L1                     = params.find<bool>("L1", false);
    string coherenceProtocol    = params.find<std::string>("coherence_protocol", "mesi");
    bool noncacheableRequests   = params.find<bool>("force_noncacheable_reqs", false);
    string cacheType            = params.find<std::string>("cache_type", "inclusive");
    SimTime_t maxWaitTime       = params.find<SimTime_t>("maxRequestDelay", 0);  // Nanoseconds
    string dirReplacement       = params.find<std::string>("noninclusive_directory_repl", "LRU");
    int dirAssociativity        = params.find<int>("noninclusive_directory_associativity", 1);
    int dirNumEntries           = params.find<int>("noninclusive_directory_entries", 0);
    
    /* Convert all strings to lower case */
    to_lower(coherenceProtocol);
    to_lower(replacement);
    to_lower(dirReplacement);
    to_lower(cacheType);

    /* Check user specified all required fields */
    if (frequency.empty())           dbg->fatal(CALL_INFO, -1, "Param not specified: frequency - cache frequency.\n");
    if (-1 >= associativity)         dbg->fatal(CALL_INFO, -1, "Param not specified: associativity\n");
    if (sizeStr.empty())             dbg->fatal(CALL_INFO, -1, "Param not specified: cache_size\n");
    if (-1 == lineSize)              dbg->fatal(CALL_INFO, -1, "Param not specified: cache_line_size - number of bytes in a cacheline (block size)\n");
    if (accessLatency == -1 )        dbg->fatal(CALL_INFO, -1, "Param not specified: access_latency_cycles - access time for cache\n");
    
    /* Ensure that cache level/coherence/inclusivity params are valid */
    if (cacheType != "inclusive" && cacheType != "noninclusive" && cacheType != "noninclusive_with_directory") 
        dbg->fatal(CALL_INFO, -1, "Invalid param: cache_type - valid options are 'inclusive' or 'noninclusive' or 'noninclusive_with_directory'. You specified '%s'.\n", cacheType.c_str());
    
    if (cacheType == "noninclusive_with_directory") {
        if (dirAssociativity <= -1) dbg->fatal(CALL_INFO, -1, "Param not specified: directory_associativity - this must be specified if cache_type is noninclusive_with_directory. You specified %d\n", dirAssociativity);
        if (dirNumEntries <= 0)     dbg->fatal(CALL_INFO, -1, "Invalid param: noninlusive_directory_entries - must be at least 1 if cache_type is noninclusive_with_directory. You specified %d\n", dirNumEntries);
    }
    
    if (L1) {
        if (cacheType != "inclusive")
            dbg->fatal(CALL_INFO, -1, "Invalid param: cache_type - must be 'inclusive' for an L1. You specified '%s'.\n", cacheType.c_str());
    } else {
        if (coherenceProtocol == "none" && cacheType != "noninclusive") 
            dbg->fatal(CALL_INFO, -1, "Invalid param combo: cache_type and coherence_protocol - non-coherent caches are noninclusive. You specified: cache_type = '%s', coherence_protocol = '%s'\n", 
                    cacheType.c_str(), coherenceProtocol.c_str()); 
    }

    /* NACKing to from L1 to the CPU doesnt really happen in CPUs*/
    //if (L1 && mshrSize != -1)   dbg->fatal(CALL_INFO, -1, "Invalid param: mshr_num_entries - must be -1 for L1s, memHierarchy assumes L1 MSHR is sized to match the CPU's load/store queue. You specified %d\n", mshrSize);
    
    /* Ensure mshr size is large enough to avoid deadlock*/
    if (-1 == mshrSize) mshrSize = HUGE_MSHR;
    if (mshrSize < 2)   dbg->fatal(CALL_INFO, -1, "Invalid param: mshr_num_entries - MSHR requires at least 2 entries to avoid deadlock. You specified %d\n", mshrSize);

    /* ---------------- Initialization ----------------- */
    HashFunction* ht;
    if (hashFunc == 1) {
      ht = new LinearHashFunction;
    } else if (hashFunc == 2) {
      ht = new XorHashFunction;
    } else {
      ht = new PureIdHashFunction;
    }
    
    fixByteUnits(sizeStr); // Convert e.g., KB to KiB for unit alg
    UnitAlgebra ua(sizeStr);
    if (!ua.hasUnits("B")) {
        dbg->fatal(CALL_INFO, -1, "Invalid param: cache_size - must have units of bytes (B). Ex: '32KiB'. SI units are ok. You specified '%s'\n", sizeStr.c_str());
    }
    uint64_t cacheSize = ua.getRoundedValue();
    uint numLines = cacheSize/lineSize;
    CoherenceProtocol protocol = CoherenceProtocol::MSI;
    
    if (coherenceProtocol == "mesi")      protocol = CoherenceProtocol::MESI;
    else if (coherenceProtocol == "msi")  protocol = CoherenceProtocol::MSI;
    else if (coherenceProtocol == "none") protocol = CoherenceProtocol::NONE;
    else dbg->fatal(CALL_INFO,-1, "Invalid param: coherence_protocol - must be 'msi', 'mesi', or 'none'\n");

    CacheArray * cacheArray = NULL;
    CacheArray * dirArray = NULL;
    ReplacementMgr* replManager = NULL;
    ReplacementMgr* dirReplManager = NULL;
    if (cacheType == "inclusive" || cacheType == "noninclusive") {
        if (SST::strcasecmp(replacement, "lru")) replManager = new LRUReplacementMgr(dbg, numLines, associativity, true);
        else if (SST::strcasecmp(replacement, "lfu"))    replManager = new LFUReplacementMgr(dbg, numLines, associativity);
        else if (SST::strcasecmp(replacement, "random")) replManager = new RandomReplacementMgr(dbg, associativity);
        else if (SST::strcasecmp(replacement, "mru"))    replManager = new MRUReplacementMgr(dbg, numLines, associativity, true);
        else if (SST::strcasecmp(replacement, "nmru"))   replManager = new NMRUReplacementMgr(dbg, numLines, associativity);
        else dbg->fatal(CALL_INFO, -1, "Invalid param: replacement_policy - supported policies are 'lru', 'lfu', 'random', 'mru', and 'nmru'. You specified %s.\n", replacement.c_str());
        cacheArray = new SetAssociativeArray(dbg, numLines, lineSize, associativity, replManager, ht, !L1);
    } else if (cacheType == "noninclusive_with_directory") {
        if (SST::strcasecmp(replacement, "lru")) replManager = new LRUReplacementMgr(dbg, numLines, associativity, true);
        else if (SST::strcasecmp(replacement, "lfu"))    replManager = new LFUReplacementMgr(dbg, numLines, associativity);
        else if (SST::strcasecmp(replacement, "random")) replManager = new RandomReplacementMgr(dbg, associativity);
        else if (SST::strcasecmp(replacement, "mru"))    replManager = new MRUReplacementMgr(dbg, numLines, associativity, true);
        else if (SST::strcasecmp(replacement, "nmru"))   replManager = new NMRUReplacementMgr(dbg, numLines, associativity);
        else dbg->fatal(CALL_INFO, -1, "Invalid param: replacement_policy - supported policies are 'lru', 'lfu', 'random', 'mru', and 'nmru'. You specified %s.\n", replacement.c_str());
        if (SST::strcasecmp(dirReplacement, "lru"))          dirReplManager = new LRUReplacementMgr(dbg, dirNumEntries, dirAssociativity, true);
        else if (SST::strcasecmp(dirReplacement, "lfu"))     dirReplManager = new LFUReplacementMgr(dbg, dirNumEntries, dirAssociativity);
        else if (SST::strcasecmp(dirReplacement, "random"))  dirReplManager = new RandomReplacementMgr(dbg, dirAssociativity);
        else if (SST::strcasecmp(dirReplacement, "mru"))     dirReplManager = new MRUReplacementMgr(dbg, dirNumEntries, dirAssociativity, true);
        else if (SST::strcasecmp(dirReplacement, "nmru"))    dirReplManager = new NMRUReplacementMgr(dbg, dirNumEntries, dirAssociativity);
        else dbg->fatal(CALL_INFO, -1, "Invalid param: directory_replacement_policy - supported policies are 'lru', 'lfu', 'random', 'mru', and 'nmru'. You specified %s.\n", replacement.c_str());
        cacheArray = new DualSetAssociativeArray(dbg, static_cast<uint>(lineSize), ht, true, dirNumEntries, dirAssociativity, dirReplManager, numLines, associativity, replManager);
    }
    

    CacheConfig config = {frequency, cacheArray, dirArray, protocol, dbg, replManager, numLines,
	static_cast<uint>(lineSize),
	static_cast<uint>(mshrSize), L1,
	noncacheableRequests, maxWaitTime, cacheType};
    return new Cache(id, params, config);
}



Cache::Cache(ComponentId_t id, Params &params, CacheConfig config) : Component(id) {
    cf_     = config;
    d_      = cf_.dbg_;
    d_->debug(_INFO_,"--------------------------- Initializing [Cache]: %s... \n", this->Component::getName().c_str());
    pMembers();
    errorChecking();
    
    d2_ = new Output();
    d2_->init("", params.find<int>("debug_level", 1), 0,(Output::output_location_t)params.find<int>("debug", SST::Output::NONE));
   
    Output out("", 1, 0, Output::STDOUT);
    
    int stats                   = params.find<int>("statistics", 0);
    accessLatency_              = params.find<uint64_t>("access_latency_cycles", 0);
    tagLatency_                 = params.find<uint64_t>("tag_access_latency_cycles",accessLatency_);
    string prefetcher           = params.find<std::string>("prefetcher");
    mshrLatency_                = params.find<uint64_t>("mshr_latency_cycles", 0);
    maxRequestsPerCycle_        = params.find<int>("max_requests_per_cycle",-1);
    string packetSize           = params.find<std::string>("min_packet_size", "8B");
    bool snoopL1Invs            = false;
    if (cf_.L1_) snoopL1Invs    = params.find<bool>("snoop_l1_invalidations", false);
    int64_t dAddr               = params.find<int64_t>("debug_addr",-1);
    if (dAddr != -1) DEBUG_ALL = false;
    else DEBUG_ALL = true;
    DEBUG_ADDR = (Addr)dAddr;
    bool found;
    
    maxOutstandingPrefetch_     = params.find<int>("max_outstanding_prefetch", cf_.MSHRSize_ / 2, found);
    dropPrefetchLevel_          = params.find<int>("drop_prefetch_mshr_level", cf_.MSHRSize_ - 2, found);
    if (!found && cf_.MSHRSize_ == 2) { // MSHR min size is 2
        dropPrefetchLevel_ = cf_.MSHRSize_ - 1;
    } else if (found && dropPrefetchLevel_ >= cf_.MSHRSize_) {
        dropPrefetchLevel_ = cf_.MSHRSize_ - 1; // Always have to leave one free for deadlock avoidance
    }

    if (maxRequestsPerCycle_ == 0) {
        maxRequestsPerCycle_ = -1;  // Simplify compare
    }
    requestsThisCycle_ = 0;

    /* --------------- Check parameters -------------*/
    if (accessLatency_ < 1) d_->fatal(CALL_INFO,-1, "%s, Invalid param: access_latency_cycles - must be at least 1. You specified %" PRIu64 "\n", 
            this->Component::getName().c_str(), accessLatency_);
  
    if (stats != 0) {
        out.output("%s, **WARNING** The 'statistics' parameter is deprecated: memHierarchy statistics have been moved to the Statistics API. Please see sst-info for available statistics and update your configuration accordingly.\nNO statistics will be printed otherwise!\n", this->Component::getName().c_str());
    }
    UnitAlgebra packetSize_ua(packetSize);
    if (!packetSize_ua.hasUnits("B")) {
        d_->fatal(CALL_INFO, -1, "%s, Invalid param: min_packet_size - must have units of bytes (B). Ex: '8B'. SI units are ok. You specified '%s'\n", this->Component::getName().c_str(), packetSize.c_str());
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
    if (mshrLatency_ < 1) intrapolateMSHRLatency();
    
    /* ----------------- MSHR ----------------- */
    mshr_               = new MSHR(d_, cf_.MSHRSize_, this->getName(), DEBUG_ALL, DEBUG_ADDR);
    mshrNoncacheable_   = new MSHR(d_, HUGE_MSHR, this->getName(), DEBUG_ALL, DEBUG_ADDR);
    
    /* ---------------- Clock ---------------- */
    clockHandler_       = new Clock::Handler<Cache>(this, &Cache::clockTick);
    defaultTimeBase_    = registerClock(cf_.cacheFrequency_, clockHandler_);
    
    registerTimeBase("2 ns", true);       //  TODO:  Is this right?

    clockIsOn_ = true;
    
    /* ------------- Member variables intialization ------------- */
    configureLinks(params);
    timestamp_              = 0;
    // Figure out interval to check max wait time and associated delay for one shot if we're asleep
    checkMaxWaitInterval_   = cf_.maxWaitTime_ / 4;
    // Doubtful that this corner case will occur but just in case...
    if (cf_.maxWaitTime_ > 0 && checkMaxWaitInterval_ == 0) checkMaxWaitInterval_ = cf_.maxWaitTime_;
    if (cf_.maxWaitTime_ > 0) {
        ostringstream oss;
        oss << checkMaxWaitInterval_;
        string interval = oss.str() + "ns";
        maxWaitWakeupExists_ = false;
        maxWaitSelfLink_ = configureSelfLink("maxWait", interval, new Event::Handler<Cache>(this, &Cache::maxWaitWakeup));
    } else {
        maxWaitWakeupExists_ = true;
    }
    
    /* Register statistics */
    statTotalEventsReceived     = registerStatistic<uint64_t>("TotalEventsReceived");
    statTotalEventsReplayed     = registerStatistic<uint64_t>("TotalEventsReplayed");
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
    statMSHROccupancy           = registerStatistic<uint64_t>("MSHR_occupancy");
    if (!prefetcher.empty()) {
        statPrefetchRequest         = registerStatistic<uint64_t>("Prefetch_requests");
        statPrefetchHit             = registerStatistic<uint64_t>("Prefetch_hits");
        statPrefetchDrop            = registerStatistic<uint64_t>("Prefetch_drops");
    }
    /* --------------- Coherence Controllers --------------- */
    coherenceMgr_ = NULL;
    std::string inclusive = (cf_.type_ == "inclusive") ? "true" : "false";
    std::string protocol = (cf_.protocol_ == CoherenceProtocol::MESI) ? "true" : "false";
    isLL = true;
    lowerIsNoninclusive = false;

    Params coherenceParams;
    coherenceParams.insert("debug_level", params.find<std::string>("debug_level", "1"));
    coherenceParams.insert("debug", params.find<std::string>("debug", "0"));
    coherenceParams.insert("access_latency_cycles", std::to_string(accessLatency_));
    coherenceParams.insert("mshr_latency_cycles", std::to_string(mshrLatency_));
    coherenceParams.insert("tag_access_latency_cycles", std::to_string(tagLatency_));
    coherenceParams.insert("cache_line_size", std::to_string(cf_.lineSize_));
    coherenceParams.insert("protocol", protocol);   // Not used by all managers
    coherenceParams.insert("inclusive", inclusive); // Not used by all managers
    coherenceParams.insert("snoop_l1_invalidations", params.find<std::string>("snoop_l1_invalidations", "false")); // Not used by all managers
    coherenceParams.insert("request_link_width", params.find<std::string>("request_link_width", "0B"));
    coherenceParams.insert("response_link_width", params.find<std::string>("response_link_width", "0B"));
    coherenceParams.insert("min_packet_size", params.find<std::string>("min_packet_size", "8B"));

    if (!cf_.L1_) {
        if (cf_.protocol_ != CoherenceProtocol::NONE) {
            if (cf_.type_ != "noninclusive_with_directory") {
                coherenceMgr_ = dynamic_cast<CoherenceController*>( loadSubComponent("memHierarchy.MESICoherenceController", this, coherenceParams));
            } else {
                coherenceMgr_ = dynamic_cast<CoherenceController*>( loadSubComponent("memHierarchy.MESICacheDirectoryCoherenceController", this, coherenceParams));
            }
        } else {
            coherenceMgr_ = dynamic_cast<CoherenceController*>( loadSubComponent("memHierarchy.IncoherentController", this, coherenceParams));
        }
    } else {
        if (cf_.protocol_ != CoherenceProtocol::NONE) {
            coherenceMgr_ = dynamic_cast<CoherenceController*>( loadSubComponent("memHierarchy.L1CoherenceController", this, coherenceParams));
        } else {
            coherenceMgr_ = dynamic_cast<CoherenceController*>( loadSubComponent("memHierarchy.L1IncoherentController", this, coherenceParams));
        }
    }
    if (coherenceMgr_ == NULL) {
        d_->fatal(CALL_INFO, -1, "%s, Failed to load CoherenceController.\n", this->Component::getName().c_str());
    }

    coherenceMgr_->setLinks(lowNetPort_, highNetPort_, bottomNetworkLink_, topNetworkLink_);
    coherenceMgr_->setMSHR(mshr_);
    coherenceMgr_->setCacheListener(listener_);
    coherenceMgr_->setDebug(DEBUG_ALL, DEBUG_ADDR);

}


/*
 *  Configure links to components above (closer to CPU) and below (closer to memory)
 *  Check for connected ports to determine which links to use
 *  Valid port combos:
 *      high_network_0 & low_network_%d : connected to core/cache/bus above and cache/bus below
 *      high_network_0 & cache          : connected to core/cache/bus above and network talking to a cache below
 *      high_network_0 & directory      : connected to core/cache/bus above and network talking to a directory below
 *      directory                       : connected to a network talking to a cache above and a directory below (single network connection)
 */
void Cache::configureLinks(Params &params) {
    bool highNetExists  = false;    // high_network_0 is connected
    bool lowCacheExists = false;    // cache is connected
    bool lowDirExists   = false;    // directory is connected
    bool lowNetExists   = false;    // low_network_%d port(s) are connected

    highNetExists   = isPortConnected("high_network_0");
    lowCacheExists  = isPortConnected("cache");
    lowDirExists    = isPortConnected("directory");
    lowNetExists    = isPortConnected("low_network_0");

    /* Check for valid port combos */
    if (highNetExists) {
        if (!lowCacheExists && !lowDirExists && !lowNetExists)
            d_->fatal(CALL_INFO,-1,"%s, Error: no connected low ports detected. Please connect one of 'cache' or 'directory' or connect N components to 'low_network_n' where n is in the range 0 to N-1\n",
                    getName().c_str());
        if ((lowCacheExists && (lowDirExists || lowNetExists)) || (lowDirExists && lowNetExists))  
            d_->fatal(CALL_INFO,-1,"%s, Error: multiple connected low port types detected. Please only connect one of 'cache', 'directory', or connect N components to 'low_network_n' where n is in the range 0 to N-1\n",
                    getName().c_str());
        if (isPortConnected("high_network_1"))
            d_->fatal(CALL_INFO,-1,"%s, Error: multiple connected high ports detected. Use the 'Bus' component to connect multiple entities to port 'high_network_0' (e.g., connect 2 L1s to a bus and connect the bus to the L2)\n",
                    getName().c_str());
    } else {
        if (!lowDirExists) 
            d_->fatal(CALL_INFO,-1,"%s, Error: no connected ports detected. Valid ports are high_network_0, cache, directory, and low_network_n\n",
                    getName().c_str());
        if (lowCacheExists || lowNetExists)
            d_->fatal(CALL_INFO,-1,"%s, Error: no connected high ports detected. Please connect a bus/cache/core on port 'high_network_0'\n",
                    getName().c_str());
    }


    /* Finally configure the links */
    if (highNetExists && lowNetExists) {
        
        d_->debug(_INFO_,"Configuring cache with a direct link above and one or more direct links below\n");
        
        // Configure low links
        string linkName = "low_network_0";
        uint32_t id = 0;
        SST::Link * link = configureLink(linkName, "50ps", new Event::Handler<Cache>(this, &Cache::processIncomingEvent));
        d_->debug(_INFO_, "Low Network Link ID: %u\n", (uint)link->getId());
        lowNetPort_ = link;
        linkName = "low_network_" + std::to_string(id);
        bottomNetworkLink_ = NULL;
    
        // Configure high link
        link = configureLink("high_network_0", "50ps", new Event::Handler<Cache>(this, &Cache::processIncomingEvent));
        d_->debug(_INFO_, "High Network Link ID: %u\n", (uint)link->getId());
        highNetPort_ = link;
        topNetworkLink_ = NULL;
    
    } else if (highNetExists && lowCacheExists) {
            
        d_->debug(_INFO_,"Configuring cache with a direct link above and a network link to a cache below\n");
        
        // Configure low link
        MemNIC::ComponentInfo myInfo;
        myInfo.link_port = "cache";
        myInfo.link_bandwidth = params.find<std::string>("network_bw", "80GiB/s");
	myInfo.num_vcs = 1;
        myInfo.name = getName();
        myInfo.network_addr = params.find<int>("network_address");
        myInfo.type = MemNIC::TypeCacheToCache; 
        myInfo.link_inbuf_size = params.find<std::string>("network_input_buffer_size", "1KiB");
        myInfo.link_outbuf_size = params.find<std::string>("network_output_buffer_size", "1KiB");

        MemNIC::ComponentTypeInfo typeInfo;
        typeInfo.blocksize = cf_.lineSize_;
        typeInfo.coherenceProtocol = cf_.protocol_;
        typeInfo.cacheType = cf_.type_;

        bottomNetworkLink_ = new MemNIC(this, d_, DEBUG_ADDR, myInfo, new Event::Handler<Cache>(this, &Cache::processIncomingEvent));
        bottomNetworkLink_->addTypeInfo(typeInfo);
        UnitAlgebra packet = UnitAlgebra(params.find<std::string>("min_packet_size", "8B"));
        if (!packet.hasUnits("B")) d_->fatal(CALL_INFO, -1, "%s, Invalid param: min_packet_size - must have units of bytes (B). Ex: '8B'. SI units are ok. You specified '%s'\n", this->Component::getName().c_str(), packet.toString().c_str());
        bottomNetworkLink_->setMinPacketSize(packet.getRoundedValue());
        
        // Configure high link
        SST::Link * link = configureLink("high_network_0", "50ps", new Event::Handler<Cache>(this, &Cache::processIncomingEvent));
        d_->debug(_INFO_, "High Network Link ID: %u\n", (uint)link->getId());
        highNetPort_ = link;
        topNetworkLink_ = NULL;

    } else if (highNetExists && lowDirExists) {
            
        d_->debug(_INFO_,"Configuring cache with a direct link above and a network link to a directory below\n");
        
        // Configure low link
        MemNIC::ComponentInfo myInfo;
        myInfo.link_port = "directory";
        myInfo.link_bandwidth = params.find<std::string>("network_bw", "80GiB/s");
	myInfo.num_vcs = 1;
        myInfo.name = getName();
        myInfo.network_addr = params.find<int>("network_address");
        myInfo.type = MemNIC::TypeCache; 
        myInfo.link_inbuf_size = params.find<std::string>("network_input_buffer_size", "1KiB");
        myInfo.link_outbuf_size = params.find<std::string>("network_output_buffer_size", "1KiB");

        MemNIC::ComponentTypeInfo typeInfo;
        typeInfo.blocksize = cf_.lineSize_;
        typeInfo.coherenceProtocol = cf_.protocol_;
        typeInfo.cacheType = cf_.type_;

        bottomNetworkLink_ = new MemNIC(this, d_, DEBUG_ADDR, myInfo, new Event::Handler<Cache>(this, &Cache::processIncomingEvent));
        bottomNetworkLink_->addTypeInfo(typeInfo);
        UnitAlgebra packet = UnitAlgebra(params.find<std::string>("min_packet_size", "8B"));
        if (!packet.hasUnits("B")) d_->fatal(CALL_INFO, -1, "%s, Invalid param: min_packet_size - must have units of bytes (B). Ex: '8B'. SI units are ok. You specified '%s'\n", this->Component::getName().c_str(), packet.toString().c_str());
        bottomNetworkLink_->setMinPacketSize(packet.getRoundedValue());

        // Configure high link
        SST::Link * link = configureLink("high_network_0", "50ps", new Event::Handler<Cache>(this, &Cache::processIncomingEvent));
        d_->debug(_INFO_, "High Network Link ID: %u\n", (uint)link->getId());
        highNetPort_ = link;
        topNetworkLink_ = NULL;

    } else {    // lowDirExists
        
        d_->debug(_INFO_, "Configuring cache with a single network link to talk to a cache above and a directory below\n");

        // Configure low link
        // This NIC may need to account for cache slices. Check params.
        int cacheSliceCount         = params.find<int>("num_cache_slices", 1);
        int sliceID                 = params.find<int>("slice_id", 0);
        string sliceAllocPolicy     = params.find<std::string>("slice_allocation_policy", "rr");
        if (cacheSliceCount == 1) sliceID = 0;
        else if (cacheSliceCount > 1) {
            if (sliceID >= cacheSliceCount) d_->fatal(CALL_INFO,-1, "%s, Invalid param: slice_id - should be between 0 and num_cache_slices-1. You specified %d.\n",
                    getName().c_str(), sliceID);
            if (sliceAllocPolicy != "rr") d_->fatal(CALL_INFO,-1, "%s, Invalid param: slice_allocation_policy - supported policy is 'rr' (round-robin). You specified '%s'.\n",
                    getName().c_str(), sliceAllocPolicy.c_str());
        } else {
            d2_->fatal(CALL_INFO, -1, "%s, Invalid param: num_cache_slices - should be 1 or greater. You specified %d.\n", 
                    getName().c_str(), cacheSliceCount);
        }

        MemNIC::ComponentInfo myInfo;
        myInfo.link_port = "directory";
        myInfo.link_bandwidth = params.find<std::string>("network_bw", "80GiB/s");
	myInfo.num_vcs = 1;
        myInfo.name = getName();
        myInfo.network_addr = params.find<int>("network_address");
        myInfo.type = MemNIC::TypeNetworkCache; 
        myInfo.link_inbuf_size = params.find<std::string>("network_input_buffer_size", "1KiB");
        myInfo.link_outbuf_size = params.find<std::string>("network_output_buffer_size", "1KiB");
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
            cf_.cacheArray_->setSliceAware(cacheSliceCount);
        }
        typeInfo.rangeStart     = addrRangeStart;
        typeInfo.rangeEnd       = addrRangeEnd;
        typeInfo.interleaveSize = interleaveSize;
        typeInfo.interleaveStep = interleaveStep;
        typeInfo.blocksize      = cf_.lineSize_;
        typeInfo.coherenceProtocol = cf_.protocol_;
        typeInfo.cacheType = cf_.type_;
        
        bottomNetworkLink_ = new MemNIC(this, d_, DEBUG_ADDR, myInfo, new Event::Handler<Cache>(this, &Cache::processIncomingEvent));
        bottomNetworkLink_->addTypeInfo(typeInfo);
        UnitAlgebra packet = UnitAlgebra(params.find<std::string>("min_packet_size", "8B"));
        if (!packet.hasUnits("B")) d_->fatal(CALL_INFO, -1, "%s, Invalid param: min_packet_size - must have units of bytes (B). Ex: '8B'. SI units are ok. You specified '%s'\n", this->Component::getName().c_str(), packet.toString().c_str());
        bottomNetworkLink_->setMinPacketSize(packet.getRoundedValue());

        // Configure high link
        topNetworkLink_ = bottomNetworkLink_;
    
    }

    // Configure self link for prefetch/listener events
    prefetchLink_ = configureSelfLink("Self", "50ps", new Event::Handler<Cache>(this, &Cache::processPrefetchEvent));
}



void Cache::intrapolateMSHRLatency() {
    uint64 N = 200; // max cache latency supported by the intrapolation method
    int y[N];

    if (cf_.L1_) {
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
        d_->fatal(CALL_INFO, -1, "%s, Error: cannot intrapolate MSHR latency if cache latency > 200. Set 'mshr_latency_cycles' or reduce cache latency. Cache latency: %" PRIu64 "\n", getName().c_str(), accessLatency_);
    }
    mshrLatency_ = y[accessLatency_];

    Output out("", 1, 0, Output::STDOUT);
    out.verbose(CALL_INFO, 1, 0, "%s: No MSHR lookup latency provided (mshr_latency_cycles)...intrapolated to %" PRIu64 " cycles.\n", getName().c_str(), mshrLatency_);
}

}}
