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

#include <sst_config.h>
#include <sst/core/stringize.h>
#include <sst/core/params.h>
#include <sst/core/timeLord.h>

#include "hash.h"
#include "cacheController.h"
#include "util.h"
#include "cacheListener.h"
#include "mshr.h"
#include "memLinkBase.h"

using namespace SST::MemHierarchy;
using namespace std;

/* Main constructor for Cache */
Cache::Cache(ComponentId_t id, Params &params) : Component(id) {

    /* --------------- Output Class --------------- */
    out_ = new Output();
    out_->init("", params.find<int>("verbose", 1), 0, Output::STDOUT);

    dbg_ = new Output();
    dbg_->init("", params.find<int>("debug_level", 1), 0,(Output::output_location_t)params.find<int>("debug", 0));

    /* Debug filtering */
    std::vector<Addr> addrArr;
    params.find_array<Addr>("debug_addr", addrArr);
    for (std::vector<Addr>::iterator it = addrArr.begin(); it != addrArr.end(); it++)
        DEBUG_ADDR.insert(*it);

    bool found;

    /* Warn about deprecated parameters */
    checkDeprecatedParams(params);

    /* Pull out parameters that the cache keeps - the rest will be pulled as needed */
    lineSize_ = params.find<uint64_t>("cache_line_size", 64);

    /* Construct cache structures */
    createCacheArray(params);

    /* Banks */
    uint64_t banks = params.find<uint64_t>("banks", 0);
    bankStatus_.resize(banks, false);
    banked_ = banks;

    /* Create clock, deadlock timeout, etc. */
    createClock(params);


    allNoncacheableRequests_    = params.find<bool>("force_noncacheable_reqs", false);
    maxRequestsPerCycle_        = params.find<int>("max_requests_per_cycle",-1);
    string packetSize           = params.find<std::string>("min_packet_size", "8B");

    UnitAlgebra packetSize_ua(packetSize);
    if (!packetSize_ua.hasUnits("B")) {
        out_->fatal(CALL_INFO, -1, "%s, Invalid param: min_packet_size - must have units of bytes (B). Ex: '8B'. SI units are ok. You specified '%s'\n", this->Component::getName().c_str(), packetSize.c_str());
    }

    if (maxRequestsPerCycle_ == 0) {
        maxRequestsPerCycle_ = -1;  // Simplify compare
    }
    requestsThisCycle_ = 0;

    /* Configure links */
    configureLinks(params);

    createCoherenceManager(params);

    /* Register statistics */
    registerStatistics();

}


void Cache::createCoherenceManager(Params &params) {
    bool found = false;

    // Latency
    uint64_t accessLatency = params.find<uint64_t>("access_latency_cycles", 0, found);
    if (!found) out_->fatal(CALL_INFO, -1, "%s, Param not specified: access_latency_cycles - access time for cache.\n", getName().c_str());

    if (accessLatency < 1) out_->fatal(CALL_INFO,-1, "%s, Invalid param: access_latency_cycles - must be at least 1. You specified %" PRIu64 "\n",
            this->Component::getName().c_str(), accessLatency);
    uint64_t tagLatency = params.find<uint64_t>("tag_access_latency_cycles", accessLatency);

    // Protocol
    std::string protStr = params.find<std::string>("coherence_protocol", "mesi");
    to_lower(protStr);
    CoherenceProtocol protocol = CoherenceProtocol::NONE;
    if (protStr == "mesi") protocol = CoherenceProtocol::MESI;
    else if (protStr == "msi") protocol = CoherenceProtocol::MSI;
    else if (protStr == "none") protocol = CoherenceProtocol::NONE;
    else out_->fatal(CALL_INFO,-1, "%s, Invalid param: coherence_protocol - must be 'msi', 'mesi', or 'none'.\n", getName().c_str());

    // L1
    bool L1 = params.find<bool>("L1", false);

    // Type
    std::string itype = params.find<std::string>("cache_type", "inclusive");
    to_lower(itype);
    if (itype != "inclusive" && itype != "noninclusive" && itype != "noninclusive_with_directory")
        out_->fatal(CALL_INFO, -1, "%s, Invalid param: cache_type - valid options are 'inclusive' or 'noninclusive' or 'noninclusive_with_directory'. You specified '%s'.\n", getName().c_str(), itype.c_str());


    if (L1 && itype != "inclusive") {
        out_->fatal(CALL_INFO, -1, "%s, Invalid param: cache_type - must be 'inclusive' for an L1. You specified '%s'.\n", getName().c_str(), itype.c_str());
    } else if (!L1 && protocol == CoherenceProtocol::NONE && itype != "noninclusive") {
        out_->fatal(CALL_INFO, -1, "%s, Invalid param combo: cache_type and coherence_protocol - non-coherent caches are noninclusive. You specified: cache_type = '%s', coherence_protocol = '%s'\n",
                getName().c_str(), itype.c_str(), protStr.c_str());
    }

    /* Create MSHR */
    uint64_t mshrLatency = createMSHR(params, accessLatency, L1);

    /* Load prefetcher, listeners, if any : Requires MSHR since drop levels depend on MSHR size*/
    createListeners(params);

    coherenceMgr_ = NULL;
    std::string inclusive = (itype == "inclusive") ? "true" : "false";
    std::string mesi = (protocol == CoherenceProtocol::MESI) ? "true" : "false";
    Params coherenceParams;
    coherenceParams.insert("debug_level", params.find<std::string>("debug_level", "1"));
    coherenceParams.insert("debug", params.find<std::string>("debug", "0"));
    coherenceParams.insert("access_latency_cycles", std::to_string(accessLatency));
    coherenceParams.insert("mshr_latency_cycles", std::to_string(mshrLatency));
    coherenceParams.insert("tag_access_latency_cycles", std::to_string(tagLatency));
    coherenceParams.insert("cache_line_size", params.find<std::string>("cache_line_size", "64"));
    coherenceParams.insert("protocol", mesi);   // Not used by all managers
    coherenceParams.insert("inclusive", inclusive); // Not used by all managers
    coherenceParams.insert("snoop_l1_invalidations", params.find<std::string>("snoop_l1_invalidations", "false")); // Not used by all managers
    coherenceParams.insert("request_link_width", params.find<std::string>("request_link_width", "0B"));
    coherenceParams.insert("response_link_width", params.find<std::string>("response_link_width", "0B"));
    coherenceParams.insert("min_packet_size", params.find<std::string>("min_packet_size", "8B"));
    coherenceParams.insert("banks", params.find<std::string>("banks", "0"));
    coherenceParams.insert("associativity", params.find<std::string>("associativity", "-1"));
    coherenceParams.insert("lines", params.find<std::string>("lines", "0"));
    coherenceParams.insert("replacement_policy", params.find<std::string>("replacement_policy", "lru"));
    coherenceParams.insert("dlines", params.find<std::string>("noninclusive_directory_entries", "0"));
    coherenceParams.insert("dassoc", params.find<std::string>("noninclusive_directory_associativity", "0"));
    coherenceParams.insert("drpolicy", params.find<std::string>("noninclusive_directory_repl", "lru"));

    bool prefetch = (statPrefetchRequest != nullptr);

    if (!L1) {
        if (protocol != CoherenceProtocol::NONE) {
            if (itype == "inclusive") {
                coherenceMgr_ = loadAnonymousSubComponent<CoherenceController>("memHierarchy.coherence.mesi_inclusive", "coherence", 0,
                        ComponentInfo::INSERT_STATS, coherenceParams, coherenceParams, prefetch);
            } else if (itype == "noninclusive") {
                coherenceMgr_ = loadAnonymousSubComponent<CoherenceController>("memHierarchy.coherence.mesi_private_noninclusive", "coherence", 0,
                        ComponentInfo::INSERT_STATS, coherenceParams, coherenceParams, prefetch);
            } else {
                coherenceMgr_ = loadAnonymousSubComponent<CoherenceController>("memHierarchy.coherence.mesi_shared_noninclusive", "coherence", 0,
                        ComponentInfo::INSERT_STATS, coherenceParams, coherenceParams, prefetch);
            }
        } else {
            coherenceMgr_ = loadAnonymousSubComponent<CoherenceController>("memHierarchy.coherence.incoherent", "coherence", 0,
                    ComponentInfo::INSERT_STATS, coherenceParams, coherenceParams, prefetch);
        }
    } else {
        if (protocol != CoherenceProtocol::NONE) {
            coherenceMgr_ = loadAnonymousSubComponent<CoherenceController>("memHierarchy.coherence.mesi_l1", "coherence", 0,
                    ComponentInfo::INSERT_STATS, coherenceParams, coherenceParams, prefetch);
        } else {
            coherenceMgr_ = loadAnonymousSubComponent<CoherenceController>("memHierarchy.coherence.incoherent_l1", "coherence", 0,
                    ComponentInfo::INSERT_STATS, coherenceParams, coherenceParams, prefetch);
        }
    }
    if (coherenceMgr_ == NULL) {
        out_->fatal(CALL_INFO, -1, "%s, Failed to load CoherenceController.\n", this->Component::getName().c_str());
    }

    int mshrSize = mshr_->getMaxSize();
    size_t maxOutstandingPrefetch = params.find<size_t>("max_outstanding_prefetch", mshrSize / 2, found);
    if (!found && mshrSize < 0)
        maxOutstandingPrefetch = (size_t) - 2; // Basically no limit if MSHR size is unlimited as well

    size_t dropPrefetchLevel = params.find<size_t>("drop_prefetch_mshr_level", mshrSize - 2, found);
    if (!found && mshrSize < 0) {          // Unlimited MSHR
        dropPrefetchLevel = (size_t) - 2;
    } else if (!found && mshrSize == 2) {   // MSHR min size is 2
        dropPrefetchLevel = mshrSize - 1;
    } else if (found && dropPrefetchLevel >= mshrSize) { // Specified dropPrefetchLevel is bigger than MSHR size
        dropPrefetchLevel = mshrSize - 1;
    }

    coherenceMgr_->setLinks(linkUp_, linkDown_);
    coherenceMgr_->setMSHR(mshr_);
    coherenceMgr_->setCacheListener(listeners_, dropPrefetchLevel, maxOutstandingPrefetch);
    coherenceMgr_->setDebug(DEBUG_ADDR);
    coherenceMgr_->setSliceAware(region_.interleaveSize, region_.interleaveStep);

}


/*
 *  Configure links to components above (closer to CPU) and below (closer to memory)
 *  Check for connected ports to determine which links to use
 *  Valid port combos:
 *      high_network_0 & low_network_%d : connected to core/cache/bus above and cache/bus below
 *      high_network_0 & cache          : connected to core/cache/bus above and network talking to a cache below
 *      high_network_0 & directory      : connected to core/cache/bus above and network talking to a directory below
 *      directory                       : connected to a network talking to a cache above and a directory below (single network connection)
 *      cache & low_network_0           : connected to network above talking to a cache and core/cache/bus below
 */
void Cache::configureLinks(Params &params) {
    linkUp_ = loadUserSubComponent<MemLinkBase>("cpulink");
    if (linkUp_)
        linkUp_->setRecvHandler(new Event::Handler<Cache>(this, &Cache::handleEvent));

    linkDown_ = loadUserSubComponent<MemLinkBase>("memlink");
    if (linkDown_)
        linkDown_->setRecvHandler(new Event::Handler<Cache>(this, &Cache::handleEvent));

    if (linkUp_ || linkDown_) {
        if (!linkUp_ || !linkDown_)
            out_->verbose(_L3_, "%s, Detected user defined subcomponent for either the cpu or mem link but not both. Assuming this component has just one link.\n", getName().c_str());
        if (!linkUp_)
            linkUp_ = linkDown_;
        if (!linkDown_)
            linkDown_ = linkUp_;

        // Check for cache slices and assign the NIC an appropriate region -> overrides the given one
        uint64_t sliceCount         = params.find<uint64_t>("num_cache_slices", 1);
        uint64_t sliceID            = params.find<uint64_t>("slice_id", 0);
        std::string slicePolicy     = params.find<std::string>("slice_allocation_policy", "rr");
        if (sliceCount == 1)
            sliceID = 0;
        else if (sliceCount > 1) {
            if (sliceID >= sliceCount)
                out_->fatal(CALL_INFO,-1, "%s, Invalid param: slice_id - should be between 0 and num_cache_slices-1. You specified %" PRIu64 ".\n",
                        getName().c_str(), sliceID);
            if (slicePolicy != "rr")
                out_->fatal(CALL_INFO,-1, "%s, Invalid param: slice_allocation_policy - supported policy is 'rr' (round-robin). You specified '%s'.\n",
                        getName().c_str(), slicePolicy.c_str());
        } else {
            out_->fatal(CALL_INFO, -1, "%s, Invalid param: num_cache_slices - should be 1 or greater. You specified %" PRIu64 ".\n",
                    getName().c_str(), sliceCount);
        }

        bool gotRegion = false;
        bool found;
        region_.setDefault();
        region_.start = params.find<uint64_t>("addr_range_start", region_.start, found);
        gotRegion |= found;
        region_.end = params.find<uint64_t>("addr_range_end", region_.end, found);
        gotRegion |= found;
        std::string isize = params.find<std::string>("interleave_size", "0B", found);
        gotRegion |= found;
        std::string istep = params.find<std::string>("interleave_step", "0B", found);
        gotRegion |= found;

        if (!UnitAlgebra(isize).hasUnits("B")) {
            out_->fatal(CALL_INFO, -1, "Invalid param(%s): interleave_size - must be specified in bytes with units (SI units OK). For example, '1KiB'. You specified '%s'\n",
                    getName().c_str(), isize.c_str());
        }
        if (!UnitAlgebra(istep).hasUnits("B")) {
            out_->fatal(CALL_INFO, -1, "Invalid param(%s): interleave_step - must be specified in bytes with units (SI units OK). For example, '1KiB'. You specified '%s'\n",
                    getName().c_str(), istep.c_str());
        }
        region_.interleaveSize = UnitAlgebra(isize).getRoundedValue();
        region_.interleaveStep = UnitAlgebra(istep).getRoundedValue();

        if (!gotRegion && sliceCount > 1) {
            gotRegion = true;
            if (slicePolicy == "rr") {
                region_.start = sliceID*lineSize_;
                region_.end = (uint64_t) - 1;
                region_.interleaveSize = lineSize_;
                region_.interleaveStep = sliceCount*lineSize_;
            }
        }

        // Little bit of error checking
        if (region_.end < region_.start) {
            out_->fatal(CALL_INFO, -1, "Invalid params(%s): addr_range_start and addr_range_end - addr_range_end is less than addr_range start. You specified start = %" PRIu64 " and end = %" PRIu64 ".\n",
                    getName().c_str(), region_.start, region_.end);
        }

        if (region_.interleaveStep < region_.interleaveSize) {
            out_->fatal(CALL_INFO, -1, "Invalid params(%s): interleave_size and interleave_step - interleave_size is larger than interleave_step. \n"
                    "interleave_size should be the granuarity of interleaving, and interleave_step should be the distance between chunks (components to interleave across * interleave_size).\n"
                    "For example, to interleave 64B lines across 3 caches, specify interleave_size=64B and interleave_step=192B. You specified size = %" PRIu64 " and step = %" PRIu64 ".\n",
                    getName().c_str(), region_.interleaveSize, region_.interleaveStep);
        }

        if (gotRegion) {
            linkDown_->setRegion(region_);
            linkUp_->setRegion(region_);
        } else {
            region_ = linkDown_->getRegion();
            linkUp_->setRegion(region_);
        }

        clockUpLink_ = linkUp_->isClocked();
        clockDownLink_ = linkDown_->isClocked();

        return;
    }


    bool highNetExists  = false;    // high_network_0 is connected -> direct link toward CPU (to bus or directly to other component)
    bool lowCacheExists = false;    // cache is connected -> direct link towards memory to cache
    bool lowDirExists   = false;    // directory is connected -> network link towards memory to directory
    bool lowNetExists   = false;    // low_network_%d port(s) are connected -> direct link towards memory (to bus or other component)

    highNetExists   = isPortConnected("high_network_0");
    lowCacheExists  = isPortConnected("cache");
    lowDirExists    = isPortConnected("directory");
    lowNetExists    = isPortConnected("low_network_0");

    /* Check for valid port combos */
    if (highNetExists) {
        if (!lowCacheExists && !lowDirExists && !lowNetExists)
            out_->fatal(CALL_INFO,-1,"%s, Error: no connected low ports detected. Please connect one of 'cache' or 'directory' or connect N components to 'low_network_n' where n is in the range 0 to N-1\n",
                    getName().c_str());
        if ((lowCacheExists && (lowDirExists || lowNetExists)) || (lowDirExists && lowNetExists))
            out_->fatal(CALL_INFO,-1,"%s, Error: multiple connected low port types detected. Please only connect one of 'cache', 'directory', or connect N components to 'low_network_n' where n is in the range 0 to N-1\n",
                    getName().c_str());
        if (isPortConnected("high_network_1"))
            out_->fatal(CALL_INFO,-1,"%s, Error: multiple connected high ports detected. Use the 'Bus' component to connect multiple entities to port 'high_network_0' (e.g., connect 2 L1s to a bus and connect the bus to the L2)\n",
                    getName().c_str());
    } else {
        if (!lowCacheExists && !lowDirExists)
            out_->fatal(CALL_INFO,-1,"%s, Error: no connected ports detected. Valid ports are high_network_0, cache, directory, and low_network_n\n",
                    getName().c_str());
    }
    region_.start = 0;
    region_.end = (uint64_t) - 1;
    region_.interleaveSize = 0;
    region_.interleaveStep = 0;

    // Fix up parameters for creating NIC - eventually we'll stop doing this
    bool found;
    if (fixupParam(params, "network_bw", "memNIC.network_bw"))
        out_->output(CALL_INFO, "Note (%s): Changed 'network_bw' to 'memNIC.network_bw' in params. Change your input file to remove this notice.\n", getName().c_str());
    if (fixupParam(params, "network_input_buffer_size", "memNIC.network_input_buffer_size"))
        out_->output(CALL_INFO, "Note (%s): Changed 'network_input_buffer_size' to 'memNIC.network_input_buffer_size' in params. Change your input file to remove this notice.\n", getName().c_str());
    if (fixupParam(params, "network_output_buffer_size", "memNIC.network_output_buffer_size"))
        out_->output(CALL_INFO, "Note (%s): Changed 'network_output_buffer_size' to 'memNIC.network_output_buffer_size' in params. Change your input file to remove this notice.\n", getName().c_str());
    if (fixupParam(params, "min_packet_size", "memNIC.min_packet_size"))
        out_->output(CALL_INFO, "Note (%s): Changed 'min_packet_size' to 'memNIC.min_packet_size'. Change your input file to remove this notice.\n", getName().c_str());

    std::string opalNode = params.find<std::string>("node", "0");
    std::string opalShMem = params.find<std::string>("shared_memory", "0");
    std::string opalSize = params.find<std::string>("local_memory_size", "0");

    Params nicParams = params.get_scoped_params("memNIC" );
    nicParams.insert("node", opalNode);
    nicParams.insert("shared_memory", opalShMem);
    nicParams.insert("local_memory_size", opalSize);

    Params memlink = params.get_scoped_params("memlink");
    memlink.insert("port", "low_network_0");
    memlink.insert("node", opalNode);
    memlink.insert("shared_memory", opalShMem);
    memlink.insert("local_memory_size", opalSize);

    Params cpulink = params.get_scoped_params("cpulink");
    cpulink.insert("port", "high_network_0");
    cpulink.insert("node", opalNode);
    cpulink.insert("shared_memory", opalShMem);
    cpulink.insert("local_memory_size", opalSize);

    /* Finally configure the links */
    if (highNetExists && lowNetExists) {

        dbg_->debug(_INFO_,"Configuring cache with a direct link above and below\n");

        linkDown_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemLink", "memlink", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, memlink);
        linkDown_->setRecvHandler(new Event::Handler<Cache>(this, &Cache::handleEvent));


        linkUp_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemLink", "cpulink", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, cpulink);
        linkUp_->setRecvHandler(new Event::Handler<Cache>(this, &Cache::handleEvent));
        clockUpLink_ = clockDownLink_ = false;
        /* Region given to each should be identical so doesn't matter which we pull but force them to be identical */
        region_ = linkDown_->getRegion();
        linkUp_->setRegion(region_);

    } else if (highNetExists && lowCacheExists) {

        dbg_->debug(_INFO_,"Configuring cache with a direct link above and a network link to a cache below\n");

        nicParams.find<std::string>("group", "", found);
        if (!found) nicParams.insert("group", "1");

        if (isPortConnected("cache_ack") && isPortConnected("cache_fwd") && isPortConnected("cache_data")) {
            nicParams.find<std::string>("req.port", "", found);
            if (!found) nicParams.insert("req.port", "cache");
            nicParams.find<std::string>("ack.port", "", found);
            if (!found) nicParams.insert("ack.port", "cache_ack");
            nicParams.find<std::string>("fwd.port", "", found);
            if (!found) nicParams.insert("fwd.port", "cache_fwd");
            nicParams.find<std::string>("data.port", "", found);
            if (!found) nicParams.insert("data.port", "cache_data");
            linkDown_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemNICFour", "memlink", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, nicParams);
        } else {
            nicParams.find<std::string>("port", "", found);
            if (!found) nicParams.insert("port", "cache");
            linkDown_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemNIC", "memlink", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, nicParams);
        }

        linkDown_->setRecvHandler(new Event::Handler<Cache>(this, &Cache::handleEvent));

        // Configure high link
        linkUp_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemLink", "cpulink", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, cpulink);
        linkUp_->setRecvHandler(new Event::Handler<Cache>(this, &Cache::handleEvent));
        clockDownLink_ = true;
        clockUpLink_ = false;

        region_ = linkDown_->getRegion();
        linkUp_->setRegion(region_);

    } else if (lowCacheExists && lowNetExists) { // "lowCache" is really "highCache" now
        dbg_->debug(_INFO_,"Configuring cache with a network link to a cache above and a direct link below\n");

        nicParams.find<std::string>("group", "", found);
        if (!found) nicParams.insert("group", "1");

        if (isPortConnected("cache_ack") && isPortConnected("cache_fwd") && isPortConnected("cache_data")) {
            nicParams.find<std::string>("req.port", "", found);
            if (!found) nicParams.insert("req.port", "cache");
            nicParams.find<std::string>("ack.port", "", found);
            if (!found) nicParams.insert("ack.port", "cache_ack");
            nicParams.find<std::string>("fwd.port", "", found);
            if (!found) nicParams.insert("fwd.port", "cache_fwd");
            nicParams.find<std::string>("data.port", "", found);
            if (!found) nicParams.insert("data.port", "cache_data");
            linkUp_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemNICFour", "cpulink", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, nicParams);
        } else {
            nicParams.find<std::string>("port", "", found);
            if (!found) nicParams.insert("port", "cache");
            linkUp_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemNIC", "cpulink", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, nicParams);
        }

        linkUp_->setRecvHandler(new Event::Handler<Cache>(this, &Cache::handleEvent));

        // Configure high link
        linkDown_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemLink", "memlink", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, memlink);
        linkDown_->setRecvHandler(new Event::Handler<Cache>(this, &Cache::handleEvent));
        clockUpLink_ = true;
        clockDownLink_ = false;

        /* Pull region off network link, really we should have the same region on both and it should be a cache property not link property... */
        region_ = linkUp_->getRegion();
        linkDown_->setRegion(region_);

    } else if (highNetExists && lowDirExists) {

        dbg_->debug(_INFO_,"Configuring cache with a direct link above and a network link to a directory below\n");

        nicParams.find<std::string>("group", "", found);
        if (!found) nicParams.insert("group", "2");

        if (isPortConnected("directory_ack") && isPortConnected("directory_fwd") && isPortConnected("directory_data")) {
            nicParams.find<std::string>("req.port", "", found);
            if (!found) nicParams.insert("req.port", "directory");
            nicParams.find<std::string>("ack.port", "", found);
            if (!found) nicParams.insert("ack.port", "directory_ack");
            nicParams.find<std::string>("fwd.port", "", found);
            if (!found) nicParams.insert("fwd.port", "directory_fwd");
            nicParams.find<std::string>("data.port", "", found);
            if (!found) nicParams.insert("data.port", "directory_data");
            linkDown_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemNICFour", "memlink", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, nicParams);
        } else {
            nicParams.find<std::string>("port", "", found);
            if (!found) nicParams.insert("port", "directory");
            linkDown_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemNIC", "memlink", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, nicParams);
        }
        // Configure low link
        linkDown_->setRecvHandler(new Event::Handler<Cache>(this, &Cache::handleEvent));

        // Configure high link
        linkUp_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemLink", "cpulink", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, cpulink);
        linkUp_->setRecvHandler(new Event::Handler<Cache>(this, &Cache::handleEvent));
        clockDownLink_ = true;
        clockUpLink_ = false;

        region_ = linkDown_->getRegion();
        linkUp_->setRegion(region_);

    } else {    // lowDirExists

        dbg_->debug(_INFO_, "Configuring cache with a network to talk to both a cache above and a directory below\n");

        nicParams.find<std::string>("group", "", found);
        if (!found) nicParams.insert("group", "2");

        nicParams.find<std::string>("port", "", found);
        if (!found) nicParams.insert("port", "directory");

        // Configure low link
        // This NIC may need to account for cache slices. Check params.
        uint64_t cacheSliceCount    = params.find<uint64_t>("num_cache_slices", 1);
        uint64_t sliceID            = params.find<uint64_t>("slice_id", 0);
        string sliceAllocPolicy     = params.find<std::string>("slice_allocation_policy", "rr");
        if (cacheSliceCount == 1) sliceID = 0;
        else if (cacheSliceCount > 1) {
            if (sliceID >= cacheSliceCount) out_->fatal(CALL_INFO,-1, "%s, Invalid param: slice_id - should be between 0 and num_cache_slices-1. You specified %" PRIu64 ".\n",
                    getName().c_str(), sliceID);
            if (sliceAllocPolicy != "rr") out_->fatal(CALL_INFO,-1, "%s, Invalid param: slice_allocation_policy - supported policy is 'rr' (round-robin). You specified '%s'.\n",
                    getName().c_str(), sliceAllocPolicy.c_str());
        } else {
            out_->fatal(CALL_INFO, -1, "%s, Invalid param: num_cache_slices - should be 1 or greater. You specified %" PRIu64 ".\n",
                    getName().c_str(), cacheSliceCount);
        }

        uint64_t addrRangeStart = 0;
        uint64_t addrRangeEnd = (uint64_t) - 1;
        uint64_t interleaveSize = 0;
        uint64_t interleaveStep = 0;

        if (cacheSliceCount > 1) {
            if (sliceAllocPolicy == "rr") {
                addrRangeStart = sliceID*lineSize_;
                interleaveSize = lineSize_;
                interleaveStep = cacheSliceCount*lineSize_;
            }
        }
        // Set region parameters
        nicParams.find<std::string>("addr_range_start", "", found);
        if (!found) nicParams.insert("addr_range_start", std::to_string(addrRangeStart));
        nicParams.find<std::string>("addr_range_end", "", found);
        if (!found) nicParams.insert("addr_range_end", std::to_string(addrRangeEnd));
        nicParams.find<std::string>("interleave_size", "", found);
        if (!found) nicParams.insert("interleave_size", std::to_string(interleaveSize) + "B");
        nicParams.find<std::string>("interleave_step", "", found);
        if (!found) nicParams.insert("interleave_step", std::to_string(interleaveStep) + "B");

        if (isPortConnected("directory_ack") && isPortConnected("directory_fwd") && isPortConnected("directory_data")) {
            nicParams.find<std::string>("req.port", "", found);
            if (!found) nicParams.insert("req.port", "directory");
            nicParams.find<std::string>("ack.port", "", found);
            if (!found) nicParams.insert("ack.port", "directory_ack");
            nicParams.find<std::string>("fwd.port", "", found);
            if (!found) nicParams.insert("fwd.port", "directory_fwd");
            nicParams.find<std::string>("data.port", "", found);
            if (!found) nicParams.insert("data.port", "directory_data");
            linkDown_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemNICFour", "cpulink", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, nicParams);
        } else {
            nicParams.find<std::string>("port", "", found);
            if (!found) nicParams.insert("port", "directory");
            linkDown_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemNIC", "cpulink", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, nicParams);
        }

        linkDown_->setRecvHandler(new Event::Handler<Cache>(this, &Cache::handleEvent));

        // Configure high link
        linkUp_ = linkDown_;
        clockDownLink_ = true;
        clockUpLink_ = false;

        region_ = linkDown_->getRegion();
        linkUp_->setRegion(region_);
    }
}

/*
 * Listeners can be prefetchers, but could also be for statistic collection, trace generation, monitoring, etc.
 * Prefetchers load into the 'prefetcher slot', listeners into the 'listener' slot
 */
void Cache::createListeners(Params &params) {
    uint64_t mshrSize = mshr_->getMaxSize(); // Either negative (unlimited) or 2+ (limited but can't be 0 or 1)
    /* Configure prefetcher(s) */
    bool found;

    SubComponentSlotInfo * lists = getSubComponentSlotInfo("prefetcher");
    if (lists) {
        int k = 0;
        for (int i = 0; i <= lists->getMaxPopulatedSlotNumber(); i++) {
            if (lists->isPopulated(i)) {
                listeners_.push_back(lists->create<CacheListener>(i, ComponentInfo::SHARE_NONE));
                listeners_[k]->registerResponseCallback(new Event::Handler<Cache>(this, &Cache::handlePrefetchEvent));
                k++;
            }
        }
    } else {
        std::string prefetcher = params.find<std::string>("prefetcher", "");
        Params prefParams;
        if (!prefetcher.empty()) {
            prefParams = params.get_scoped_params("prefetcher");
            listeners_.push_back(loadAnonymousSubComponent<CacheListener>(prefetcher, "prefetcher", 0, ComponentInfo::INSERT_STATS, prefParams));
            listeners_[0]->registerResponseCallback(new Event::Handler<Cache>(this, &Cache::handlePrefetchEvent));
        }
    }
    if (!listeners_.empty()) {
        statPrefetchRequest = registerStatistic<uint64_t>("Prefetch_requests");
        statPrefetchDrop = registerStatistic<uint64_t>("Prefetch_drops");
    } else {
        statPrefetchRequest = nullptr;
        statPrefetchDrop = nullptr;
    }

    if (!listeners_.empty()) { // Have at least one prefetcher
        // Configure self link for prefetch/listener events
        // Delay prefetches by a cycle TODO parameterize - let user specify prefetch delay
        std::string frequency = params.find<std::string>("cache_frequency", "", found);
        prefetchDelay_ = params.find<SimTime_t>("prefetch_delay_cycles", 1);

        prefetchSelfLink_ = configureSelfLink("prefetchlink", frequency, new Event::Handler<Cache>(this, &Cache::processPrefetchEvent));
    }

    /* Configure listener(s) */
    lists = getSubComponentSlotInfo("listener");
    if (lists) {
        for (int i = 0; i < lists->getMaxPopulatedSlotNumber(); i++) {
            if (lists->isPopulated(i))
                listeners_.push_back(lists->create<CacheListener>(i, ComponentInfo::SHARE_NONE));
        }
    } else if (listeners_.empty()) {
        Params emptyParams;
        listeners_.push_back(loadAnonymousSubComponent<CacheListener>("memHierarchy.emptyCacheListener", "listener", 0, ComponentInfo::SHARE_NONE, emptyParams));
    }
}

uint64_t Cache::createMSHR(Params &params, uint64_t accessLatency, bool L1) {
    bool found;
    uint64_t defaultMshrLatency = 1;
    int mshrSize = params.find<int>("mshr_num_entries", -1);           //number of entries
    uint64_t mshrLatency = params.find<uint64_t>("mshr_latency_cycles", defaultMshrLatency, found);

    if (mshrSize == 1 || mshrSize == 0)
        out_->fatal(CALL_INFO, -1, "Invalid param: mshr_num_entries - MSHR requires at least 2 entries to avoid deadlock. You specified %d\n", mshrSize);

    mshr_ = new MSHR(dbg_, mshrSize, getName(), DEBUG_ADDR);

    if (mshrLatency > 0 && found)
        return mshrLatency;

    if (L1) {
        mshrLatency = 1;
    } else {
        // Otherwise if mshrLatency isn't set or is 0, intrapolate from cache latency
        uint64_t N = 200; // max cache latency supported by the intrapolation method
        int y[N];

        /* L2 */
        y[0] = 0;
        y[1] = 1;
        for(uint64_t idx = 2;  idx < 12; idx++) y[idx] = 2;
        for(uint64_t idx = 12; idx < 16; idx++) y[idx] = 3;
        for(uint64_t idx = 16; idx < 26; idx++) y[idx] = 5;

        /* L3 */
        for(uint64_t idx = 26; idx < 46; idx++) y[idx] = 19;
        for(uint64_t idx = 46; idx < 68; idx++) y[idx] = 26;
        for(uint64_t idx = 68; idx < N;  idx++) y[idx] = 32;

        if (accessLatency > N) {
            out_->fatal(CALL_INFO, -1, "%s, Error: cannot intrapolate MSHR latency if cache latency > 200. Set 'mshr_latency_cycles' or reduce cache latency. Cache latency: %" PRIu64 "\n",
                    getName().c_str(), accessLatency);
        }
        mshrLatency = y[accessLatency];
    }

    if (mshrLatency != defaultMshrLatency) {
        Output out("", 1, 0, Output::STDOUT);
        out.verbose(CALL_INFO, 1, 0, "%s: No MSHR lookup latency provided (mshr_latency_cycles)...intrapolated to %" PRIu64 " cycles.\n", getName().c_str(), mshrLatency);
    }
    return mshrLatency;
}

/* Create the cache array */
void Cache::createCacheArray(Params &params) {
    /* Get parameters and error check */
    bool found;
    std::string sizeStr = params.find<std::string>("cache_size", "", found);
    if (!found) out_->fatal(CALL_INFO, -1, "%s, Param not specified: cache_size\n", getName().c_str());

    uint64_t assoc = params.find<uint64_t>("associativity", -1, found); // uint64_t to match cache size in case we have a fully associative cache
    if (!found) out_->fatal(CALL_INFO, -1, "%s, Param not specified: associativity\n", getName().c_str());


    uint64_t dEntries = params.find<uint64_t>("noninclusive_directory_entries", 0);
    uint64_t dAssoc = params.find<uint64_t>("noninclusive_directory_associativity", 1);


    /* Error check parameters and compute derived parameters */
    /* Fix up parameters */
    fixByteUnits(sizeStr);

    UnitAlgebra ua(sizeStr);
    if (!ua.hasUnits("B")) {
        out_->fatal(CALL_INFO, -1, "%s, Invalid param: cache_size - must have units of bytes(B). Ex: '32KiB'. SI units are ok. You specified '%s'.", getName().c_str(), sizeStr.c_str());
    }

    uint64_t cacheSize = ua.getRoundedValue();

    if (lineSize_ > cacheSize)
        out_->fatal(CALL_INFO, -1, "%s, Invalid param combo: cache_line_size cannot be greater than cache_size. You specified: cache_size = '%s', cache_line_size = '%" PRIu64 "'\n",
                getName().c_str(), sizeStr.c_str(), lineSize_);
    if (!isPowerOfTwo(lineSize_)) out_->fatal(CALL_INFO, -1, "%s, cache_line_size - must be a power of 2. You specified '%" PRIu64 "'.\n", getName().c_str(), lineSize_);

    uint64_t lines = cacheSize / lineSize_;
    params.insert("lines", std::to_string(lines));
    return;
}

void Cache::createClock(Params &params) {
    /* Create clock */
    bool found;
    std::string frequency = params.find<std::string>("cache_frequency", "", found);
    if (!found)
        out_->fatal(CALL_INFO, -1, "%s, Param not specified: frequency - cache frequency.\n", getName().c_str());

    clockHandler_       = new Clock::Handler<Cache>(this, &Cache::clockTick);
    defaultTimeBase_    = registerClock(frequency, clockHandler_);

    clockIsOn_ = true;
    timestamp_ = 0;
    lastActiveClockCycle_ = 0;

    // Deadlock timeout
    timeout_ = params.find<SimTime_t>("maxRequestDelay", 0);
    if (timeout_ > 0) {
        SimTime_t checkInterval = timeout_ / 2; // An event might go nearly 1.5X the maxNano before detection but not more than that. Checking isn't cheap and failing is unlikely -> don't do it often
        ostringstream oss;
        oss << checkInterval;
        string interval = oss.str() + "ns";
        timeoutSelfLink_ = configureSelfLink("timeout", interval, new Event::Handler<Cache>(this, &Cache::timeoutWakeup));
    }
}

/* Check for deprecated parameters and warn/fatal */
void Cache::checkDeprecatedParams(Params &params) {
    Output out("", 1, 0, Output::STDOUT);
    bool found;

    /* Standard error messages */
    std::string defError = "This parameter is no longer neccessary.";
    std::string autoDetectError = "The value of this parameter is now auto-detected.";

    std::map<std::string,std::string> depMap;

    /* Deprecated parameters */
    depMap["network_address"] = autoDetectError;

    for (std::map<std::string,std::string>::iterator it = depMap.begin(); it != depMap.end(); it++) {
        params.find<std::string>(it->first, "", found);
        if (found) {
            out.output("%s, ** Found deprecated parameter: %s ** %s Remove this parameter from your input deck to eliminate this message.\n", getName().c_str(), it->first.c_str(), it->second.c_str());
        }
    }
}

void Cache::registerStatistics() {
    Statistic<uint64_t>* def_stat = registerStatistic<uint64_t>("default_stat");
    for (int i = 0; i < (int)Command::LAST_CMD; i++) {
        statCacheRecv[i] = def_stat;
        statUncacheRecv[i] = def_stat;
    }

    statRecvEvents  = registerStatistic<uint64_t>("TotalEventsReceived");
    statRetryEvents = registerStatistic<uint64_t>("TotalEventsReplayed");

    statUncacheRecv[(int)Command::Put]      = registerStatistic<uint64_t>("Put_uncache_recv");
    statUncacheRecv[(int)Command::Get]      = registerStatistic<uint64_t>("Get_uncache_recv");
    statUncacheRecv[(int)Command::AckMove]  = registerStatistic<uint64_t>("AckMove_uncache_recv");
    statUncacheRecv[(int)Command::GetS]     = registerStatistic<uint64_t>("GetS_uncache_recv");
    statUncacheRecv[(int)Command::GetX]     = registerStatistic<uint64_t>("GetX_uncache_recv");
    statUncacheRecv[(int)Command::GetSX]    = registerStatistic<uint64_t>("GetSX_uncache_recv");
    statUncacheRecv[(int)Command::GetSResp] = registerStatistic<uint64_t>("GetSResp_uncache_recv");
    statUncacheRecv[(int)Command::GetXResp] = registerStatistic<uint64_t>("GetXResp_uncache_recv");
    statUncacheRecv[(int)Command::CustomReq]  = registerStatistic<uint64_t>("CustomReq_uncache_recv");
    statUncacheRecv[(int)Command::CustomResp] = registerStatistic<uint64_t>("CustomResp_uncache_recv");
    statUncacheRecv[(int)Command::CustomAck]  = registerStatistic<uint64_t>("CustomAck_uncache_recv");

    // Valid cache commands depend on coherence manager
    std::set<Command> validrecv = coherenceMgr_->getValidReceiveEvents();

    for (std::set<Command>::iterator it = validrecv.begin(); it != validrecv.end(); it++) {
        std::string stat = CommandString[(int)(*it)];
        stat.append("_recv");
        statCacheRecv[(int)(*it)] = registerStatistic<uint64_t>(stat);
    }

    statMSHROccupancy               = registerStatistic<uint64_t>("MSHR_occupancy");
    statBankConflicts               = registerStatistic<uint64_t>("Bank_conflicts");
}
