// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
 * File:   sieveFactory.cc
 */


#include <sst_config.h>
#include <sst/core/params.h>
#include "../util.h"
#include "../hash.h"
#include "sieveController.h"


namespace SST{ namespace MemHierarchy{
    using namespace SST::MemHierarchy;
    using namespace std;

Sieve::Sieve(ComponentId_t id, Params &params) : Component(id) {

    /* --------------- Output Class --------------- */
    output_ = new Output();
    int debugLevel = params.find<int>("debug_level", 0);

    output_->init("--->  ", debugLevel, 0,(Output::output_location_t)params.find<int>("debug", 0));
    if(debugLevel < 0 || debugLevel > 10)     output_->fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");

    /* --------------- Get Parameters --------------- */
    // LRU - default replacement policy
    int associativity   = params.find<int>("associativity", -1);
    string sizeStr      = params.find<std::string>("cache_size", "");              //Bytes
    lineSize_           = params.find<uint64_t>("cache_line_size", -1);            //Bytes

    /* Check user specified all required fields */
    if(-1 >= associativity)         output_->fatal(CALL_INFO, -1, "Param not specified: associativity\n");
    if(sizeStr.empty())             output_->fatal(CALL_INFO, -1, "Param not specified: cache_size\n");
    if(-1 == lineSize_)             output_->fatal(CALL_INFO, -1, "Param not specified: cache_line_size - number of bytes in a cacheline (block size)\n");

    fixByteUnits(sizeStr);
    UnitAlgebra ua(sizeStr);
    if (!ua.hasUnits("B")) {
        output_->fatal(CALL_INFO, -1, "Invalid param: cache_size - must have units of bytes (e.g., B, KB,etc.)\n");
    }
    uint64_t cacheSize = ua.getRoundedValue();
    uint64_t numLines = cacheSize/lineSize_;

    /* ---------------- Initialization ----------------- */
    HashFunction* ht = loadAnonymousSubComponent<HashFunction>("memHierarchy.hash.none", "hash", 0, ComponentInfo::SHARE_NONE, params);
    ReplacementPolicy* replManager = loadUserSubComponent<ReplacementPolicy>("replacement", ComponentInfo::SHARE_NONE, numLines, associativity);
    if (!replManager)
        replManager = loadAnonymousSubComponent<ReplacementPolicy>("memHierarchy.replacement.lru", "replacement", 0, ComponentInfo::SHARE_NONE, params, numLines, associativity);
    cacheArray_ = new CacheArray<SharedCacheLine>(output_, numLines, associativity, lineSize_, replManager, ht);

    output_->debug(_INFO_,"--------------------------- Initializing [Sieve]: %s... \n", this->Component::getName().c_str());

    /* file output */
    outFileName  = params.find<std::string>("output_file");
    if (outFileName.empty()) {
      outFileName = "sieveMallocRank";
    }
    outCount = 0;

    resetStatsOnOutput = params.find<bool>("reset_stats_at_buoy", 0) != 0;

    // optional link for allocation / free tracking
    configureLinks();

    /* Load profiler, if any */
    createProfiler(params);

    /* Register statistics */
    statReadHits    = registerStatistic<uint64_t>("ReadHits");
    statReadMisses  = registerStatistic<uint64_t>("ReadMisses");
    statWriteHits   = registerStatistic<uint64_t>("WriteHits");
    statWriteMisses = registerStatistic<uint64_t>("WriteMisses");
    statUnassocReadMisses   = registerStatistic<uint64_t>("UnassociatedReadMisses");
    statUnassocWriteMisses  = registerStatistic<uint64_t>("UnassociatedWriteMisses");
}

void Sieve::configureLinks() {
    SST::Link* link;
    cpuLinkCount_ = 0;
    while (true) {
        std::ostringstream linkName;
        linkName << "cpu_link_" << cpuLinkCount_;
        std::string ln = linkName.str();
        link = configureLink(ln, "100 ps", new Event::Handler<Sieve, int>(this, &Sieve::processEvent, cpuLinkCount_));
        if (link) {
            cpuLinks_.push_back(link);
            output_->output(CALL_INFO, "Port %d = Link %d\n", cpuLinks_[cpuLinkCount_]->getId(), cpuLinkCount_);
            cpuLinkCount_++;
        } else {
            break;
        }
    }
    if (cpuLinkCount_ < 1) output_->fatal(CALL_INFO, -1,"Did not find any connected links on ports cpu_link_n\n");

    allocLinks_.resize(cpuLinkCount_);
    for (int i = 0; i < cpuLinkCount_; i++) {
        std::ostringstream linkName;
        linkName << "alloc_link_" << i;
        std::string ln = linkName.str();
        allocLinks_[i] = configureLink(ln, "50ps", new Event::Handler<Sieve>(this, &Sieve::processAllocEvent));
    }
}

void Sieve::createProfiler(const Params &params) {
    listener_ = loadUserSubComponent<CacheListener>("profiler");
    if (listener_) return;

    string profiler = params.find<std::string>("profiler", "");
    if (profiler.empty()) {
	listener_ = 0;
    } else {
	Params profilerParams = params.get_scoped_params("profiler" );
        listener_ = loadAnonymousSubComponent<CacheListener>(profiler, "profiler", 0, ComponentInfo::INSERT_STATS, profilerParams);
    }

}


    }} // end namespaces
