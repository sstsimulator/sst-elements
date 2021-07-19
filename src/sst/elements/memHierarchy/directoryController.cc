// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright(c) 2013-2021, NTESS
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
#include "directoryController.h"


#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "memNIC.h"

/* Debug macros */
#ifdef __SST_DEBUG_OUTPUT__ /* From sst-core, enable with --enable-debug */
#define is_debug_addr(addr) (DEBUG_ADDR.empty() || DEBUG_ADDR.find(addr) != DEBUG_ADDR.end())
#define is_debug_event(ev) (DEBUG_ADDR.empty() || ev->doDebug(DEBUG_ADDR))
#else
#define is_debug_addr(addr) false
#define is_debug_event(ev) false
#endif

using namespace SST;
using namespace SST::MemHierarchy;
using namespace std;


DirectoryController::DirectoryController(ComponentId_t id, Params &params) :
    Component(id) {
    int debugLevel = params.find<int>("debug_level", 0);
    dlevel = debugLevel;
    cacheLineSize = params.find<uint32_t>("cache_line_size", 64);
    lineSize = cacheLineSize;

    dbg.init("", debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));
    if (debugLevel < 0 || debugLevel > 10)     dbg.fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");


    // Detect deprecated parameters and warn/fatal
    // Currently deprecated - network_num_vc
    bool found;
    out.init("", params.find<int>("verbose", 1), 0, Output::STDOUT);
    params.find<int>("network_num_vc", 0, found);
    if (found) {
        out.output("%s, ** Found deprecated parameter: network_num_vc ** MemHierarchy does not use multiple virtual channels. Remove this parameter from your input deck to eliminate this message.\n", getName().c_str());
    }

    // Debug address
    std::vector<Addr> addrArr;
    params.find_array<Addr>("debug_addr", addrArr);
    for (std::vector<Addr>::iterator it = addrArr.begin(); it != addrArr.end(); it++) {
        DEBUG_ADDR.insert(*it);
    }

    registerTimeBase("1 ns", true); // TODO eliminate this

    std::string net_bw = params.find<std::string>("network_bw", "80GiB/s");

    MemRegion region;
    bool gotRegion = false;
    region.start = params.find<uint64_t>("addr_range_start", 0, found);
    if (!found) region.start = params.find<uint64_t>("memNIC.addr_range_start", 0, found);
    if (!found) region.start = params.find<uint64_t>("memlink.addr_range_start", 0, found);
    gotRegion |= found;

    region.end = params.find<uint64_t>("addr_range_end", (uint64_t) - 1, found);
    if (!found) region.end = params.find<uint64_t>("memNIC.addr_range_end", (uint64_t) - 1, found);
    if (!found) region.end = params.find<uint64_t>("memlink.addr_range_end", (uint64_t) - 1, found);
    gotRegion |= found;

    string ilSize   = params.find<std::string>("interleave_size", "0B", found);
    if (!found) ilSize = params.find<std::string>("memNIC.interleave_size", "0B", found);
    if (!found) ilSize = params.find<std::string>("memlink.interleave_size", "0B", found);
    gotRegion |= found;

    string ilStep   = params.find<std::string>("interleave_step", "0B", found);
    if (!found) ilStep = params.find<std::string>("memNIC.interleave_step", "0B", found);
    if (!found) ilStep = params.find<std::string>("memlink.interleave_step", "0B", found);
    gotRegion |= found;

    if(0 == region.end) region.end = (uint64_t)-1;


    memOffset = params.find<uint64_t>("mem_addr_start", 0);

    UnitAlgebra packetSize = UnitAlgebra(params.find<std::string>("min_packet_size", "8B"));
    if (!packetSize.hasUnits("B")) dbg.fatal(CALL_INFO, -1, "%s, Invalid param: min_packet_size - must have units of bytes (B). SI units are ok. You specified '%s'\n", getName().c_str(), packetSize.toString().c_str());


    /* Check interleaveSize & Step
     * Both must be specified in B (SI units ok)
     * Both must be divisible by the cache line size */
    fixByteUnits(ilSize);
    fixByteUnits(ilStep);
    region.interleaveSize = UnitAlgebra(ilSize).getRoundedValue();
    region.interleaveStep = UnitAlgebra(ilStep).getRoundedValue();
    if (!UnitAlgebra(ilSize).hasUnits("B") || region.interleaveSize % cacheLineSize != 0) {
        dbg.fatal(CALL_INFO, -1, "Invalid param(%s): interleave_size - must be specified in bytes with units (SI units OK) and must also be a multiple of cache_line_size. This definition has CHANGED. Example: If you used to set this to '1', change it to '1KiB'. You specified %s\n",
                getName().c_str(), ilSize.c_str());
    }
    if (!UnitAlgebra(ilStep).hasUnits("B") || region.interleaveStep % cacheLineSize != 0) {
        dbg.fatal(CALL_INFO, -1, "Invalid param(%s): interleave_step - must be specified in bytes with units (SI units OK) and must also be a multiple of cache_line_size. This definition has CHANGED. Example: If you used to set this to '4', change it to '4KiB'. You specified %s\n",
                getName().c_str(), ilStep.c_str());
    }

    /*
     *  *****************************
     *  Regions & memory name
     *  *****************************
     *  In earlier versions of the directoryController there was a 1-1 correspondence between a DC and a memory controller (MC).
     *  In this case, the DC had an associated address region, a name of the MC it controlled, and the DC passed the region to the MC.
     *
     *  Later, the requirement was relaxed so that there could be a many-1 correspondance. In this case, multiple DCs could address a single MC.
     *  To do this, MCs declare their own region but the DC uses it's 'memory name' to only address a single MC. For backward compatibility, the
     *  DC still sends its region to the named MC, the MC can optionally ignore it.
     *
     *  Now, we want to enable many-many. To do this, DCs and MCs MUST declare their own regions and no memory name is needed.
     *  Backward compatiblity is not assured: if you declare a memory name, the DC assumes you want 1-1 or many-1. If you don't, it assumes you gave
     *  the MCs their own region. We cannot error check from the parameters...
     */

    cpuLink = loadUserSubComponent<MemLinkBase>("cpulink");
    memLink = loadUserSubComponent<MemLinkBase>("memlink");
    if (cpuLink || memLink) {
        if (!cpuLink) {
            cpuLink = memLink;
            memLink = nullptr;
        }
        if (gotRegion) {
            cpuLink->setRegion(region);
        } else {
            if (cpuLink->getRegion() != region) {
                out.output(CALL_INFO, "%s, Warning: getting region parameters (addr_range_start/end, interleave_step/size) from link subcomponent."
                        " In the future this will not be supported and region parameters should be declared in the directory's parameters instead.\n", getName().c_str());
            }
            region = cpuLink->getRegion();
        }

        if (memLink)
            memLink->setRegion(region);

        cpuLink->setRecvHandler(new Event::Handler<DirectoryController>(this, &DirectoryController::handlePacket));
        memoryName = "";
        if (!memLink) {
            memoryName  = params.find<std::string>("net_memory_name", "");
            if (memoryName != "")
                dbg.output(CALL_INFO,"%s, Warning: parameter 'net_memory_name' is deprecated in order to support many<->many communication between DCs and MCs.\n"
                        "Instead of using net_memory_name, ensure that the directory and memory controllers all specify their own address regions (address_range_start/end, interleave_step/size).\n"
                        "In the future, the directory controller will NOT automatically force its region parameters on its named memory controller\n", getName().c_str());
        } else {
            memLink->setRecvHandler(new Event::Handler<DirectoryController>(this, &DirectoryController::handlePacket));
        }
    } else {
        /* Set up links/network to cache & memory the old way -> and fixup params accordingly */
        fixupParam(params, "network_bw", "memNIC.network_bw");
        fixupParam(params, "network_input_buffer_size", "memNIC.network_input_buffer_size");
        fixupParam(params, "network_output_buffer_size", "memNIC.network_output_buffer_size");
        fixupParam(params, "addr_range_start", "memNIC.addr_range_start");
        fixupParam(params, "addr_range_end", "memNIC.addr_range_end");
        fixupParam(params, "interleave_size", "memNIC.interleave_size");
        fixupParam(params, "interleave_step", "memNIC.interleave_step");
        fixupParam(params, "min_packet_size", "memNIC.min_packet_size");

        Params nicParams = params.get_scoped_params("memNIC");

        nicParams.insert("group", "3", false);
        int cl = nicParams.find<int>("group");
        nicParams.insert("sources", std::to_string(cl - 1), false);
        nicParams.insert("destinations", std::to_string(cl + 1), false);

        // Determine which ports are connected
        unsigned int portCount = 1;
        if (isPortConnected("network_ack")) portCount++;
        if (isPortConnected("network_fwd")) portCount++;
        if (isPortConnected("network_data")) portCount++;
        if (portCount == 4) {
            nicParams.insert("req.port", "network");
            nicParams.insert("ack.port", "network_ack");
            nicParams.insert("fwd.port", "network_fwd");
            nicParams.insert("data.port", "network_data");
            cpuLink = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemNICFour", "cpulink", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, nicParams);
        } else {
            nicParams.insert("port", "network");
            cpuLink = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemNIC", "cpulink", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, nicParams);
        }

        cpuLink->setRecvHandler(new Event::Handler<DirectoryController>(this, &DirectoryController::handlePacket));

        if (isPortConnected("memory")) {
            Params memParams = params.get_scoped_params("memlink");
            memParams.insert("port", "memory");
            memParams.insert("latency", "1ns");
            memParams.insert("addr_range_start", std::to_string(region.start), false);
            memParams.insert("addr_range_end", std::to_string(region.end), false);
            memParams.insert("interleave_size", ilSize, false);
            memParams.insert("interleave_step", ilStep, false);
            memLink = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemLink", "memlink", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, memParams);
            memLink->setRecvHandler(new Event::Handler<DirectoryController>(this, &DirectoryController::handlePacket));
            if (!memLink) {
                dbg.fatal(CALL_INFO, -1, "%s, Error creating link to memory from directory controller\n", getName().c_str());
            }
            memoryName = "";
        } else {
            memoryName  = params.find<std::string>("net_memory_name", "");
            if (memoryName != "")
                dbg.output(CALL_INFO,"%s, Warning: parameter 'net_memory_name' is deprecated in order to support many<->many communication between DCs and MCs.\n"
                        "Instead of using net_memory_name, ensure that the directory and memory controllers all specify their own address regions (address_range_start/end, interleave_step/size).\n"
                        "In the future, the directory controller will NOT automatically force its region parameters on its named memory controller\n", getName().c_str());
            memLink = nullptr;
        }
    }

    clockHandler = new Clock::Handler<DirectoryController>(this, &DirectoryController::clock);
    defaultTimeBase = registerClock(params.find<std::string>("clock", "1GHz"), clockHandler);
    clockOn = true;
    if (memLink)
        clockMemLink = memLink->isClocked();
    else
        clockMemLink = false;
    clockCpuLink = cpuLink->isClocked();

    // Requests per cycle
    maxRequestsPerCycle = params.find<int>("max_requests_per_cycle", 0);

    // Timestamp - aka cycle count
    timestamp = 0;

    // Coherence protocol configuration
    waitWBAck = false; // Don't expect WB Acks
    sendWBAck = true;

    Statistic<uint64_t>* defStat = registerStatistic<uint64_t>("default_stat");
    for (int i = 0; i < (int)Command::LAST_CMD; i++) {
        stat_eventRecv[i] = defStat;
        stat_noncacheRecv[i] = defStat;
        stat_eventSent[i] = defStat;
    }

    // Register statistics
    stat_replacementRequestLatency  = registerStatistic<uint64_t>("replacement_request_latency");
    stat_getRequestLatency          = registerStatistic<uint64_t>("get_request_latency");
    stat_cacheHits                  = registerStatistic<uint64_t>("directory_cache_hits");
    stat_mshrHits                   = registerStatistic<uint64_t>("mshr_hits");
    stat_eventRecv[(int)Command::GetX] = registerStatistic<uint64_t>("GetX_recv");
    stat_eventRecv[(int)Command::GetS] = registerStatistic<uint64_t>("GetS_recv");
    stat_eventRecv[(int)Command::GetSX] = registerStatistic<uint64_t>("GetSX_recv");
    stat_eventRecv[(int)Command::PutM]  = registerStatistic<uint64_t>("PutM_recv");
    stat_eventRecv[(int)Command::PutX]  = registerStatistic<uint64_t>("PutX_recv");
    stat_eventRecv[(int)Command::PutE]  = registerStatistic<uint64_t>("PutE_recv");
    stat_eventRecv[(int)Command::PutS]  = registerStatistic<uint64_t>("PutS_recv");
    stat_eventRecv[(int)Command::NACK]  = registerStatistic<uint64_t>("NACK_recv");
    stat_eventRecv[(int)Command::FetchResp]     = registerStatistic<uint64_t>("FetchResp_recv");
    stat_eventRecv[(int)Command::FetchXResp]    = registerStatistic<uint64_t>("FetchXResp_recv");
    stat_eventRecv[(int)Command::GetSResp]      = registerStatistic<uint64_t>("GetSResp_recv");
    stat_eventRecv[(int)Command::GetXResp]      = registerStatistic<uint64_t>("GetXResp_recv");
    stat_eventRecv[(int)Command::ForceInv]      = registerStatistic<uint64_t>("ForceInv_recv");
    stat_eventRecv[(int)Command::FetchInv]      = registerStatistic<uint64_t>("FetchInv_recv");
    stat_eventRecv[(int)Command::AckInv]        = registerStatistic<uint64_t>("AckInv_recv");
    stat_eventRecv[(int)Command::FlushLine]     = registerStatistic<uint64_t>("FlushLine_recv");
    stat_eventRecv[(int)Command::FlushLineInv]  = registerStatistic<uint64_t>("FlushLineInv_recv");
    stat_eventRecv[(int)Command::FlushLineResp] = registerStatistic<uint64_t>("FlushLineResp_recv");
    stat_noncacheRecv[(int)Command::GetS]       = registerStatistic<uint64_t>("GetS_uncache_recv");
    stat_noncacheRecv[(int)Command::GetX]       = registerStatistic<uint64_t>("GetX_uncache_recv");
    stat_noncacheRecv[(int)Command::GetSX]      = registerStatistic<uint64_t>("GetSX_uncache_recv");
    stat_noncacheRecv[(int)Command::GetSResp]   = registerStatistic<uint64_t>("GetSResp_uncache_recv");
    stat_noncacheRecv[(int)Command::GetXResp]   = registerStatistic<uint64_t>("GetXResp_uncache_recv");
    stat_noncacheRecv[(int)Command::CustomReq]  = registerStatistic<uint64_t>("CustomReq_uncache_recv");
    stat_noncacheRecv[(int)Command::CustomResp] = registerStatistic<uint64_t>("CustomResp_uncache_recv");
    stat_noncacheRecv[(int)Command::CustomAck]  = registerStatistic<uint64_t>("CustomAck_uncache_recv");
    // Events sent
    stat_eventSent[(int)Command::GetS]          = registerStatistic<uint64_t>("eventSent_GetS");
    stat_eventSent[(int)Command::GetX]          = registerStatistic<uint64_t>("eventSent_GetX");
    stat_eventSent[(int)Command::GetSX]         = registerStatistic<uint64_t>("eventSent_GetSX");
    stat_eventSent[(int)Command::PutM]          = registerStatistic<uint64_t>("eventSent_PutM");
    stat_eventSent[(int)Command::Inv]           = registerStatistic<uint64_t>("eventSent_Inv");
    stat_eventSent[(int)Command::FetchInv]      = registerStatistic<uint64_t>("eventSent_FetchInv");
    stat_eventSent[(int)Command::FetchInvX]     = registerStatistic<uint64_t>("eventSent_FetchInvX");
    stat_eventSent[(int)Command::ForceInv]      = registerStatistic<uint64_t>("eventSent_ForceInv");
    stat_eventSent[(int)Command::NACK]          = registerStatistic<uint64_t>("eventSent_NACK");
    stat_eventSent[(int)Command::GetSResp]      = registerStatistic<uint64_t>("eventSent_GetSResp");
    stat_eventSent[(int)Command::GetXResp]      = registerStatistic<uint64_t>("eventSent_GetXResp");
    stat_eventSent[(int)Command::FetchResp]     = registerStatistic<uint64_t>("eventSent_FetchResp");
    stat_eventSent[(int)Command::AckInv]        = registerStatistic<uint64_t>("eventSent_AckInv");
    stat_eventSent[(int)Command::AckPut]        = registerStatistic<uint64_t>("eventSent_AckPut");
    stat_eventSent[(int)Command::FlushLine]     = registerStatistic<uint64_t>("eventSent_FlushLine");
    stat_eventSent[(int)Command::FlushLineInv]  = registerStatistic<uint64_t>("eventSent_FlushLineInv");
    stat_eventSent[(int)Command::FlushLineResp] = registerStatistic<uint64_t>("eventSent_FlushLineResp");
    // Memory writes from directory
    stat_dirEntryReads              = registerStatistic<uint64_t>("eventSent_read_directory_entry");
    stat_dirEntryWrites             = registerStatistic<uint64_t>("eventSent_write_directory_entry");
    stat_MSHROccupancy              = registerStatistic<uint64_t>("MSHR_occupancy");

    // Coherence part

    if (!memLink)
        memLink = cpuLink;

    // TODO implement the cache properly using the cacheArray
    entryCacheMaxSize = params.find<uint64_t>("entry_cache_size", 32768);
    entryCacheSize = 0;
    entrySize = 4; // Bytes, TODO parameterize

    string protstr  = params.find<std::string>("coherence_protocol", "MESI");
    if (protstr == "mesi" || protstr == "MESI") protocol = CoherenceProtocol::MESI;
    else if (protstr == "msi" || protstr == "MSI") protocol = CoherenceProtocol::MSI;
    else dbg.fatal(CALL_INFO, -1, "Invalid param(%s): coherence_protocol - must be 'MESI' or 'MSI'. You specified: %s\n", getName().c_str(), protstr.c_str());

    int mshrSize    = params.find<int>("mshr_num_entries",-1);
    if (mshrSize == 0) dbg.fatal(CALL_INFO, -1, "Invalid param(%s): mshr_num_entries - must be at least 1 or else negative to indicate an unlimited size MSHR\n", getName().c_str());
    mshr                = new MSHR(&dbg, mshrSize, getName(), DEBUG_ADDR);

    /* Get latencies */
    accessLatency   = params.find<uint64_t>("access_latency_cycles", 0);
    mshrLatency     = params.find<uint64_t>("mshr_latency_cycles", 0);
}



DirectoryController::~DirectoryController(){
    for(std::unordered_map<Addr, DirEntry*>::iterator i = directory.begin(); i != directory.end() ; ++i){
        delete i->second;
    }
    directory.clear();
}



void DirectoryController::handlePacket(SST::Event *event){
    MemEventBase *evb = static_cast<MemEventBase*>(event);
    evb->setDeliveryTime(getCurrentSimTimeNano());
    if (!clockOn) {
        turnClockOn();
    }

    /* Forward events that we don't handle */
    if (MemEventTypeArr[(int)evb->getCmd()] != MemEventType::Cache || evb->queryFlag(MemEvent::F_NONCACHEABLE)) {

        if (is_debug_event(evb)) {
            dbg.debug(_L3_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:New     (%s)\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), timestamp, getName().c_str(), evb->getVerboseString().c_str());
        }

        if (BasicCommandClassArr[(int)evb->getCmd()] == BasicCommandClass::Request)
            handleNoncacheableRequest(evb);
        else
            handleNoncacheableResponse(evb);
        return;

    }

    MemEvent * ev = static_cast<MemEvent*>(event);
    if (CommandClassArr[(int)ev->getCmd()] == CommandClass::Request)
        recordStartLatency(ev);
    eventBuffer.push_back(ev);
}

/**
 *  Called each cycle. Handle any waiting events in the queue.
 */
bool DirectoryController::clock(SST::Cycle_t cycle){
    timestamp = cycle;
    stat_MSHROccupancy->addData(mshr->getSize());

    sendOutgoingEvents();

    bool idle = true;
    if (clockCpuLink)
        idle &= cpuLink->clock();
    if (clockMemLink)
        idle &= memLink->clock();

    int requestsThisCycle = 0;

    addrsThisCycle.clear();

    size_t entries = retryBuffer.size();

    std::list<MemEvent*>::iterator it = retryBuffer.begin();
    while (it != retryBuffer.end()) {
        if (maxRequestsPerCycle != 0 && requestsThisCycle == maxRequestsPerCycle) {
            break;
        }
        dbg.debug(_L3_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Retry   (%s)\n",
                Simulation::getSimulation()->getCurrentSimCycle(), timestamp, getName().c_str(), (*it)->getVerboseString().c_str());
        
        if (processPacket(*it, true)) {
            requestsThisCycle++;
            it = retryBuffer.erase(it);
        } else {
            it++;
        }
    }

    it = eventBuffer.begin();
    while (it != eventBuffer.end()) {
        if (maxRequestsPerCycle != 0 && requestsThisCycle == maxRequestsPerCycle)
            break;

        dbg.debug(_L3_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:New     (%s)\n",
                Simulation::getSimulation()->getCurrentSimCycle(), timestamp, getName().c_str(), (*it)->getVerboseString().c_str());

        if (processPacket(*it, false)) {
            requestsThisCycle++;
            it = eventBuffer.erase(it);
        } else {
            it++;
        }
    }

    idle &= (eventBuffer.empty() && retryBuffer.empty());
    idle &= (cpuMsgQueue.empty() && memMsgQueue.empty());

   if (idle && clockOn) {
        clockOn = false;
        lastActiveClockCycle = timestamp;
        return true;
    }
    return false;
}


bool DirectoryController::processPacket(MemEvent * ev, bool replay) {
    bool dbgevent = false;
    if (is_debug_event(ev)) {
        fflush(stdout);
        dbgevent = true;
    }

    if(! isRequestAddressValid(ev->getAddr()) ) {
	dbg.fatal(CALL_INFO, -1, "%s, Error: Request address is not valid. Event: %s. Time = %" PRIu64 "ns\n",
                getName().c_str(), ev->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    Addr addr = ev->getBaseAddr();

    /* Disallow more than one access to a given line per cycle */
    if (!arbitrateAccess(addr)) {
        if (is_debug_addr(addr)) {
            std::stringstream id;
            id << "<" << ev->getID().first << "," << ev->getID().second << ">";
            dbg.debug(_L5_, "A: %-20" PRIu64 " %-20" PRIu64 " %-20s %-13s 0x%-16" PRIx64 " %-15s %-6s %-6s %-10s %-15s\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), timestamp, getName().c_str(), CommandString[(int)ev->getCmd()],
                    addr, id.str().c_str(), "", "", "Stall", "(line conflict)");
        }
        return false;
    }

    bool retval = false;
    Command cmd = ev->getCmd();

    if (!replay) {
        stat_eventRecv[(int)cmd]->addData(1);
    }

    switch (cmd) {
        case Command::GetS:
            retval = handleGetS(ev, replay);
            break;
        case Command::GetSX:
            retval = handleGetSX(ev, replay);
            break;
        case Command::GetX:
            retval = handleGetX(ev, replay);
            break;
        case Command::PutS:
            retval = handlePutS(ev, replay);
            break;
        case Command::PutE:
            retval = handlePutE(ev, replay);
            break;
        case Command::PutX:
            retval = handlePutX(ev, replay);
            break;
        case Command::PutM:
            retval = handlePutM(ev, replay);
            break;
        case Command::FlushLineInv:
            retval = handleFlushLineInv(ev, replay);
            break;
        case Command::FlushLine:
            retval = handleFlushLine(ev, replay);
            break;
        case Command::FetchInv:
            retval = handleFetchInv(ev, replay);
            break;
        case Command::ForceInv:
            retval = handleForceInv(ev, replay);
            break;
        case Command::GetXResp:
            retval = handleGetXResp(ev, replay);
            break;
        case Command::GetSResp:
            retval = handleGetSResp(ev, replay);
            break;
        case Command::FlushLineResp:
            retval = handleFlushLineResp(ev, replay);
            break;
        case Command::AckInv:
            retval = handleAckInv(ev, replay);
            break;
        case Command::AckPut:
            retval = handleAckPut(ev, replay);
            break;
        case Command::FetchResp:
            retval = handleFetchResp(ev, replay);
            break;
        case Command::FetchXResp:
            retval = handleFetchXResp(ev, replay);
            break;
        case Command::NACK:
            retval = handleNACK(ev, replay);
            break;
        default:
            dbg.fatal(CALL_INFO, -1 , "%s, Error: Received unrecognized request: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), ev->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    if (dbgevent)
        printDebugInfo();

    if (retval)
        addrsThisCycle.insert(addr);

    return retval;
}


bool DirectoryController::arbitrateAccess(Addr addr) {
    if (addrsThisCycle.find(addr) == addrsThisCycle.end())
        return true;
    return false;
}

void DirectoryController::handleNoncacheableRequest(MemEventBase * ev) {
    if (!(ev->queryFlag(MemEventBase::F_NORESPONSE))) {
        noncacheMemReqs[ev->getID()] = ev->getSrc();
    }
    stat_noncacheRecv[(int)ev->getCmd()]->addData(1);

    ev->setSrc(getName());
    if (memoryName == "")
        ev->setDst(memLink->findTargetDestination(ev->getRoutingAddress()));
    else
        ev->setDst(memoryName);

    forwardTowardsMem(ev);
}


void DirectoryController::handleNoncacheableResponse(MemEventBase * ev) {
    if (noncacheMemReqs.find(ev->getID()) == noncacheMemReqs.end()) {
        dbg.fatal(CALL_INFO, -1, "%s, Error: Received a noncacheable response that does not match a pending request. Event: %s\n. Time: %" PRIu64 "ns\n",
                getName().c_str(), ev->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    ev->setDst(noncacheMemReqs[ev->getID()]);
    ev->setSrc(getName());

    stat_noncacheRecv[(int)ev->getCmd()]->addData(1);

    noncacheMemReqs.erase(ev->getID());

    forwardTowardsCPU(ev);
}

void DirectoryController::printStatus(Output &statusOut) {
    statusOut.output("MemHierarchy::DirectoryController %s\n", getName().c_str());
    statusOut.output("  Cached entries: %" PRIu64 "\n", entryCacheSize);
    statusOut.output("  Requests waiting to be handled:  %zu\n", eventBuffer.size());
//    for(std::list<std::pair<MemEvent*,bool> >::iterator i = workQueue.begin() ; i != workQueue.end() ; ++i){
//        statusOut.output("    %s, %s\n", i->first->getVerboseString().c_str(), i->second ? "replay" : "new");
//    }

    if (mshr) {
        statusOut.output("  MSHR Status:\n");
        mshr->printStatus(statusOut);
    }

    if (cpuLink) {
        statusOut.output("  NIC Status: ");
        cpuLink->printStatus(statusOut);
    }

    if (memLink != cpuLink) {
        statusOut.output("  Memory Link Status: ");
        memLink->printStatus(statusOut);
    }

    statusOut.output("  Directory entries:\n");
    for (std::unordered_map<Addr, DirEntry*>::iterator it = directory.begin(); it != directory.end(); it++) {
        statusOut.output("    0x%" PRIx64 " %s\n", it->first, it->second->getString().c_str());
    }
    statusOut.output("End MemHierarchy::DirectoryController\n\n");
}


void DirectoryController::emergencyShutdown() {
    if (out.getVerboseLevel() > 1) {
        if (out.getOutputLocation() == Output::STDOUT)
            out.setOutputLocation(Output::STDERR);
        printStatus(out);
        out.output("   Checking for unreceived events on network link:\n");
        cpuLink->emergencyShutdownDebug(out);
    }
}


bool DirectoryController::isRequestAddressValid(Addr addr){
    return cpuLink->isRequestAddressValid(addr);

    if(0 == region.interleaveSize) {
        return (addr >= region.start && addr < region.end);
    } else {
        if (addr < region.start) return false;
        if (addr >= region.end) return false;

        addr        = addr - region.start;
        Addr offset = addr % region.interleaveStep;

        if (offset >= region.interleaveSize) return false;
        return true;
    }
}


void DirectoryController::turnClockOn() {
    clockOn = true;
    timestamp = reregisterClock(defaultTimeBase, clockHandler);
    timestamp--; // reregisterClock returns next cycle clock will be enabled, set timestamp to current cycle
    uint64_t inactiveCycles = timestamp - lastActiveClockCycle;
    for (uint64_t i = 0; i < inactiveCycles; i++) {
        stat_MSHROccupancy->addData(mshr->getSize());
    }
}


void DirectoryController::init(unsigned int phase) {

    cpuLink->init(phase);
    if (cpuLink != memLink) memLink->init(phase);


    // Must happen after network init or merlin croaks
    // InitData: Name, NULLCMD, Endpoint type, inclusive of all upper levels, will send writeback acks, line size
    if (!phase) {
        // Push our region to memory (backward compatibility)
        // Only push if we got a memory name (old config script)
        // or we are directly linked to a memory (always safe to push)
        if (memoryName != "" || cpuLink != memLink) {
            MemEventInitRegion * reg = new MemEventInitRegion(getName(), memLink->getRegion(), true);
            reg->setDst(memoryName);
            memLink->sendInitData(reg);
        }
        if (cpuLink != memLink)
            cpuLink->sendInitData(new MemEventInitCoherence(getName(), Endpoint::Directory, true, true, false, cacheLineSize, true));
        memLink->sendInitData(new MemEventInitCoherence(getName(), Endpoint::Directory, true, true, false, cacheLineSize, true));
    }

    /* Pass data on to memory */
    while(MemEventInit *ev = cpuLink->recvInitData()){
        if (ev->getCmd() == Command::NULLCMD) {
            dbg.debug(_L10_, "I: %-20s   Event:Init      (%s)\n",
                getName().c_str(), ev->getVerboseString().c_str());
            if (ev->getInitCmd() == MemEventInit::InitCommand::Coherence) {
                MemEventInitCoherence * mEv = static_cast<MemEventInitCoherence*>(ev);
                if (mEv->getType() == Endpoint::Scratchpad)
                    waitWBAck = true;
            }
            delete ev;
        } else {

            /* Check that memory name is valid - really only need to do this once, but since it's init, whatever */
            if (cpuLink == memLink && memoryName != "" && !cpuLink->isDest(memoryName)) {
                dbg.fatal(CALL_INFO,-1,"%s, Invalid param: net_memory_name - must name a valid memory component in the system. You specified: %s.\nNOTE this parameter is deprecated anyways in favor of giving memory and directory controllers their own region parameters\n",getName().c_str(), memoryName.c_str());
            }

            dbg.debug(_L10_, "I: %-20s   Event:Init      (%s)\n",
                    getName().c_str(), ev->getVerboseString().c_str());
            if (isRequestAddressValid(ev->getAddr())){
                dbg.debug(_L10_, "I: %-20s   Event:SendInitData    %" PRIx64 "\n",
                        getName().c_str(), ev->getAddr());
                if (memoryName == "")
                    ev->setDst(memLink->findTargetDestination(ev->getRoutingAddress()));
                else
                    ev->setDst(memoryName);
                    memLink->sendInitData(ev);
            } else
                delete ev;

        }

    }

    SST::Event * ev;
    if (cpuLink != memLink) {
        while ((ev = memLink->recvInitData())) {
            MemEventInit * initEv = dynamic_cast<MemEventInit*>(ev);
            if (initEv && initEv->getCmd() == Command::NULLCMD) {
                dbg.debug(_L10_, "I: %-20s   Event:Init      (%s)\n",
                        getName().c_str(), initEv->getVerboseString().c_str());

                if (initEv->getInitCmd() == MemEventInit::InitCommand::Coherence) {
                    MemEventInitCoherence * mEv = static_cast<MemEventInitCoherence*>(initEv);
                    if (mEv->getSendWBAck())
                        waitWBAck = true;
                }
            }
            delete ev;
        }
    }
}



void DirectoryController::finish(void){
    cpuLink->finish();
}



void DirectoryController::setup(void){
    cpuLink->setup();
    //MemLinkBase * mem = memLink ? memLink : network;
    // dircc->configure(getName(), memoryName, sendWBAck, recvWBAck, network, mem);
}



/**************************************************************************************************************************/
/***** The rest will eventually become a coherence manager so that we can use the directory for different protocols *******/
/**************************************************************************************************************************/

bool DirectoryController::handleGetS(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry * entry = getDirEntry(addr);
    State state = entry->getState();
    bool cached = entry->isCached();
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::GetS, false, addr, state);

    if (!cached)
        return retrieveDirEntry(entry, event, inMSHR);

    if (!inMSHR)
        stat_cacheHits->addData(1);

    switch (state) {
        case I:
            if (mshr->hasData(addr)) {
                if (!inMSHR)
                    out.output("ALERT (%s): mshr should NOT have data for 0x%" PRIx64 " but it does...\n", getName().c_str(), addr);
                else {
                    if (protocol == CoherenceProtocol::MESI) {
                        entry->setState(M);
                        entry->setOwner(event->getSrc());
                        sendDataResponse(event, entry, mshr->getData(addr), Command::GetXResp);
                        mshr->clearData(addr);
                    } else {
                        entry->setState(S);
                        entry->addSharer(event->getSrc());
                        sendDataResponse(event, entry, mshr->getData(addr), Command::GetSResp);
                    }
                    if (is_debug_event(event)) {
                        eventDI.reason = "hit";
                        eventDI.action = "Done";
                    }
                    cleanUpAfterRequest(event, inMSHR);
                    break;
                }
            }
            // Miss, get data from memory
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);
            if (status == MemEventStatus::OK) {
                issueMemoryRequest(event, entry);
                entry->setState(IS);
                if (is_debug_event(event))
                    eventDI.reason = "miss";
            }
            break;
        case S:
            if (mshr->hasData(addr)) { // saved from earlier request
                entry->addSharer(event->getSrc());
                sendDataResponse(event, entry, mshr->getData(addr), Command::GetSResp);
                if (is_debug_event(event)) {
                    eventDI.reason = "hit";
                    eventDI.action = "Done";
                }
                cleanUpAfterRequest(event, inMSHR);
                break;
            }
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);
            if (status == MemEventStatus::OK) {
                issueMemoryRequest(event, entry);
                entry->setState(S_D);
            }
            if (is_debug_event(event))
                eventDI.reason = "hit";
            break;
        case M:
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);
            if (is_debug_event(event))
                eventDI.reason = "hit";
            if (status == MemEventStatus::OK) {
                issueFetch(event, entry, Command::FetchInvX);
                entry->setState(M_InvX);
            }
            break;
        default:
            if (!inMSHR)
                status = allocateMSHR(event, false);
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    return true;
}


bool DirectoryController::handleGetSX(MemEvent * event, bool inMSHR) {
    return handleGetX(event, inMSHR);
}


bool DirectoryController::handleGetX(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry * entry = getDirEntry(addr);
    State state = entry->getState();
    bool cached = entry->isCached();
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), event->getCmd(), false, addr, state);

    if (!cached)
        return retrieveDirEntry(entry, event, inMSHR);

    if (!inMSHR)
        stat_cacheHits->addData(1);

    if (mshr->hasData(addr) && mshr->getDataDirty(addr)) // Data was temporarily buffered here due to racing accesses
        writebackDataFromMSHR(addr);

    switch (state) {
        case I:
            if (mshr->hasData(addr)) {
                if (!inMSHR)
                    out.output("ALERT (%s): mshr should NOT have data for 0x%" PRIx64 " but it does...\n", getName().c_str(), addr);
                else {
                    entry->setState(M);
                    entry->setOwner(event->getSrc());
                    sendDataResponse(event, entry, mshr->getData(addr), Command::GetXResp);
                    mshr->clearData(addr);
                    if (is_debug_event(event)) {
                        eventDI.reason = "hit";
                        eventDI.action = "Done";
                    }
                    cleanUpAfterRequest(event, inMSHR);
                    break;
                }
            }
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);
            if (status == MemEventStatus::OK) {
                entry->setState(IM);
                issueMemoryRequest(event, entry);
                if (is_debug_event(event))
                    eventDI.reason = "miss";
            }
            break;
        case S:
            // Cases
            // Upgrade request and no other sharers -> respond & M
            // Upgrade request and other sharers -> invalidate other sharers & S_Inv
            // Otherwise need data & invalidate sharers -> invalidate other sharers, request data from Memory, SM_Inv
            if (entry->isSharer(event->getSrc())) { // Don't need data
                if (entry->getSharerCount() == 1) { // Also don't need to invalidate
                    if (mshr->hasData(addr))
                        mshr->clearData(addr);
                    entry->setState(M);
                    entry->removeSharer(event->getSrc());
                    entry->setOwner(event->getSrc());
                    sendResponse(event);
                    if (is_debug_event(event)) {
                        eventDI.reason = "hit";
                        eventDI.action = "Done";
                    }
                    cleanUpAfterRequest(event, inMSHR);
                    break;
                }
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);
                if (status == MemEventStatus::OK) {
                    if (mshr->hasData(addr))
                        mshr->clearData(addr);
                    entry->setState(S_Inv);
                    issueInvalidations(event, entry, Command::Inv);
                    if (is_debug_event(event)) {
                        eventDI.reason = "miss";
                    }
                }
            } else { // Need data
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);
                if (status == MemEventStatus::OK) {
                    if (mshr->hasData(addr)) {
                        entry->setState(S_Inv);
                    } else {
                        entry->setState(SM_Inv);
                        issueMemoryRequest(event, entry);
                    }
                    issueInvalidations(event, entry, Command::Inv);
                    if (is_debug_event(event)) {
                        eventDI.reason = "miss";
                    }
                }
            }
            break;
        case M:
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);
            if (status == MemEventStatus::OK) {
                entry->setState(M_Inv);
                issueFetch(event, entry, Command::FetchInv);
                if (is_debug_event(event)) {
                    eventDI.reason = "miss";
                }
            }
            break;
        default:
            if (!inMSHR)
                status = allocateMSHR(event, false);
            break;
    }

    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}

bool DirectoryController::handleFlushLine(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry* entry = getDirEntry(addr);
    State state = entry->getState();
    bool cached = entry->isCached();
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::FlushLine, false, addr, state);

    if (!cached)
        return retrieveDirEntry(entry, event, inMSHR);

    if (!inMSHR) {
        stat_cacheHits->addData(1);
        status = allocateMSHR(event, false);
    }

    switch (state) {
        case I:
            if (status == MemEventStatus::OK) {
                issueFlush(event);
            }
            break;
        case S:
            if (status == MemEventStatus::OK) {
                issueFlush(event);
                entry->setState(S_B);
            }
            break;
        case M:
            if (status == MemEventStatus::OK) {
                if (event->getEvict()) {
                    entry->removeOwner();
                    entry->addSharer(event->getSrc());
                    mshr->setData(addr, event->getPayload(), event->getDirty());
                    event->setEvict(false);
                } else if (entry->hasOwner()) {
                    issueFetch(event, entry, Command::FetchInvX);
                    entry->setState(M_InvX);
                    break;
                }
                issueFlush(event);
                entry->setState(S_B);
            }
            break;
        case M_Inv:
            if (event->getEvict()) {
                entry->removeOwner();
                entry->addSharer(event->getSrc());
                mshr->setData(addr, event->getPayload(), event->getDirty());
                event->setEvict(false);
                entry->setState(S_Inv);
            }
            break;
        case M_InvX:
            if (event->getEvict()) {
                entry->removeOwner();
                entry->addSharer(event->getSrc());
                mshr->setData(addr, event->getPayload(), event->getDirty());
                entry->setState(S);
                mshr->decrementAcksNeeded(addr);
                responses.find(addr)->second.erase(event->getSrc());
                if (responses.find(addr)->second.empty()) responses.erase(addr);
                retryBuffer.push_back(static_cast<MemEvent*>(mshr->getFrontEvent(addr)));
            }
            break;
        default:
            break;
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    return true;
}


bool DirectoryController::handleFlushLineInv(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry* entry = getDirEntry(addr);
    State state = entry->getState();
    bool cached = entry->isCached();
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::FlushLineInv, false, addr, state);

    if (!cached)
        return retrieveDirEntry(entry, event, inMSHR);

    if (!inMSHR) {
        stat_cacheHits->addData(1);
        status = allocateMSHR(event, false);
    }

    switch (state) {
        case I:
            if (status == MemEventStatus::OK)
                issueFlush(event);
            break;
        case S:
            if (status == MemEventStatus::OK) {
                if (event->getEvict()) {
                    entry->removeSharer(event->getSrc());
                    event->setEvict(false);
                }

                if (entry->hasSharers()) {
                    entry->setState(S_Inv);
                    issueInvalidations(event, entry, Command::Inv);
                } else {
                    entry->setState(I_B);
                    issueFlush(event);
                }
            }
            break;
        case M:
            if (status == MemEventStatus::OK) {
                if (event->getEvict()) {
                    entry->removeOwner();
                    mshr->setData(addr, event->getPayload(), event->getDirty());
                    event->setEvict(false);
                }

                if (entry->hasOwner()) {
                    entry->setState(M_Inv);
                    issueFetch(event, entry, Command::FetchInv);
                } else {
                    entry->setState(I_B);
                    issueFlush(event);
                }
            }
            break;
        case S_D:
            if (event->getEvict()) {
                entry->removeSharer(event->getSrc());
                event->setEvict(false);
                if (!entry->hasSharers())
                    entry->setState(IS);
            }
            break;
        case S_B:
            if (event->getEvict()) {
                entry->removeSharer(event->getSrc());
                event->setEvict(false);
                if (!entry->hasSharers())
                    entry->setState(I);
            }
            break;
        case M_InvX:
            if (event->getEvict()) {
                entry->removeOwner();
                mshr->setData(addr, event->getPayload(), event->getDirty());
                event->setEvict(false);
                responses.find(addr)->second.erase(event->getSrc());
                if (responses.find(addr)->second.empty()) responses.erase(addr);

                if (mshr->decrementAcksNeeded(addr)) {
                    entry->setState(I);
                    retryBuffer.push_back(static_cast<MemEvent*>(mshr->getFrontEvent(addr)));
                }
            }
            break;
        case SD_Inv:
            if (event->getEvict()) {
                entry->removeSharer(event->getSrc());
                event->setEvict(false);
                responses.find(addr)->second.erase(event->getSrc());
                if (responses.find(addr)->second.empty()) responses.erase(addr);
                if (mshr->decrementAcksNeeded(addr)) {
                    entry->hasSharers() ? entry->setState(S_D) : entry->setState(IS);
                    retryBuffer.push_back(static_cast<MemEvent*>(mshr->getFrontEvent(addr)));
                }
            }
            break;
        case SM_Inv:
            if (event->getEvict()) {
                entry->removeSharer(event->getSrc());
                event->setEvict(false);
                responses.find(addr)->second.erase(event->getSrc());
                if (responses.find(addr)->second.empty()) responses.erase(addr);
                if (mshr->decrementAcksNeeded(addr)) {
                    entry->setState(IM);
                }
            }
            break;
        case S_Inv:
            if (event->getEvict()) {
                entry->removeSharer(event->getSrc());
                event->setEvict(false);
                responses.find(addr)->second.erase(event->getSrc());
                if (responses.find(addr)->second.empty()) responses.erase(addr);
                if (mshr->decrementAcksNeeded(addr)) {
                    entry->hasSharers() ? entry->setState(S) : entry->setState(I);
                    retryBuffer.push_back(static_cast<MemEvent*>(mshr->getFrontEvent(addr)));
                }
            }
            break;
        case M_Inv:
            if (event->getEvict()) {
                entry->removeSharer(event->getSrc());
                event->setEvict(false);
                responses.find(addr)->second.erase(event->getSrc());
                if (responses.find(addr)->second.empty()) responses.erase(addr);
                if (mshr->decrementAcksNeeded(addr)) {
                    entry->setState(I);
                    retryBuffer.push_back(static_cast<MemEvent*>(mshr->getFrontEvent(addr)));
                }
            }
            break;
        default:
            break;
    }


    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    return true;
}

bool DirectoryController::handlePutS(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry* entry = getDirEntry(addr);
    State state = entry->getState();
    bool cached = entry->isCached();

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::PutS, false, addr, state);

    if (!cached)
        return retrieveDirEntry(entry, event, inMSHR);

    if (!inMSHR)
        stat_cacheHits->addData(1);

    entry->removeSharer(event->getSrc());
    sendAckPut(event);

    if (responses.find(addr) != responses.end() && responses.find(addr)->second.find(event->getSrc()) != responses.find(addr)->second.end()) {
        responses.find(addr)->second.erase(event->getSrc());
        if (responses.find(addr)->second.empty()) responses.erase(addr);
    }

    bool update = false;
    switch (state) {
        case S:
            if (!entry->hasSharers()) {
                entry->setState(I);
            }
            update = true;
            break;
        case S_B:
            if (!entry->hasSharers()) {
                entry->setState(I);
            }
            break;
        case S_D:
            if (!entry->hasSharers()) {
                entry->setState(IS);
            }
            break;
        case S_Inv:
            if (mshr->decrementAcksNeeded(addr)) {
                entry->hasSharers() ? entry->setState(S) : entry->setState(I);
            }
            break;
        case SD_Inv:
            if (mshr->decrementAcksNeeded(addr)) {
                entry->hasSharers() ? entry->setState(S_D) : entry-> setState(IS);
            }
            break;
        case SM_Inv:
            if (mshr->decrementAcksNeeded(addr))
                entry->setState(IM);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "%s, Error: Directory received PutS but state is %s. Event = %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    cleanUpAfterRequest(event, inMSHR);

    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    if (update)
        updateCache(entry);


    return true;
}

bool DirectoryController::handlePutX(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry* entry = getDirEntry(addr);
    State state = entry->getState();
    bool cached = entry->isCached();

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::PutX, false, addr, state);

    if (!cached)
        return retrieveDirEntry(entry, event, inMSHR);

    if (!inMSHR)
        stat_cacheHits->addData(1);

    entry->removeOwner();
    entry->addSharer(event->getSrc());

    sendAckPut(event);

    bool update = false;
    switch (state) {
        case M:
            if (event->getDirty()) {
                writebackData(event);
            }
            entry->setState(S);
            update = true;
            break;
        case M_Inv:
            mshr->setData(addr, event->getPayload(), event->getDirty());
            entry->setState(S_Inv);
            break;
        case M_InvX:
            mshr->decrementAcksNeeded(addr);
            responses.find(addr)->second.erase(event->getSrc());
            if (responses.find(addr)->second.empty()) responses.erase(addr);
            mshr->setData(addr, event->getPayload(), event->getDirty());
            entry->setState(S);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "%s, Error: Directory received PutX but state is %s. Event = %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    cleanUpAfterRequest(event, inMSHR);
    if (update) updateCache(entry);

    return true;
}

bool DirectoryController::handlePutE(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry* entry = getDirEntry(addr);
    State state = entry->getState();
    bool cached = entry->isCached();

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::PutE, false, addr, state);

    if (!cached)
        return retrieveDirEntry(entry, event, inMSHR);

    if (!inMSHR)
        stat_cacheHits->addData(1);

    entry->removeOwner();

    sendAckPut(event);

    bool update = false;
    switch (state) {
        case M:
            entry->setState(I);
            update = true;
            break;
        case M_Inv:
        case M_InvX:
            mshr->decrementAcksNeeded(addr);
            responses.find(addr)->second.erase(event->getSrc());
            if (responses.find(addr)->second.empty()) responses.erase(addr);
            mshr->setData(addr, event->getPayload(), event->getDirty());
            entry->setState(I);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "%s, Error: Directory received PutE but state is %s. Event = %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    cleanUpAfterRequest(event, inMSHR);
    if (update) updateCache(entry);

    return true;
}

bool DirectoryController::handlePutM(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry* entry = getDirEntry(addr);
    State state = entry->getState();
    bool cached = entry->isCached();

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::PutM, false, addr, state);

    if (!cached)
        return retrieveDirEntry(entry, event, inMSHR);

    if (!inMSHR)
        stat_cacheHits->addData(1);

    entry->removeOwner();

    sendAckPut(event);

    bool update = false;
    switch (state) {
        case M:
            writebackData(event);
            entry->setState(I);
            update = true;
            break;
        case M_Inv:
        case M_InvX:
            mshr->decrementAcksNeeded(addr);
            responses.find(addr)->second.erase(event->getSrc());
            if (responses.find(addr)->second.empty()) responses.erase(addr);
            mshr->setData(addr, event->getPayload(), event->getDirty());
            entry->setState(I);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "%s, Error: Directory received PutM but state is %s. Event = %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    cleanUpAfterRequest(event, inMSHR);
    if (update) updateCache(entry);

    return true;
}

/* Sent by a memory controller or scratchpad doing a shootdown */
bool DirectoryController::handleFetchInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry* entry = getDirEntry(addr);
    State state = entry->getState();
    bool cached = entry->isCached();
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::FetchInv, false, addr, state);

    if (!cached)
        return retrieveDirEntry(entry, event, inMSHR);

    if (!inMSHR)
        stat_cacheHits->addData(1);

    switch (state) {
        case I:
            if (!(mshr->pendingWriteback(addr) || (mshr->exists(addr) && mshr->getFrontEvent(addr)->getCmd() == Command::FlushLineInv))) {
                if (mshr->hasData(addr) && mshr->getDataDirty(addr))
                    sendFetchResponse(event);
                else
                   sendAckInv(event);
            } else {
                sendAckInv(event);
            }
            cleanUpAfterRequest(event, inMSHR);
            break;
        case S:
            if (!inMSHR)
                status = allocateMSHR(event, true);
            if (status == MemEventStatus::OK) {
                issueInvalidations(event, entry, Command::Inv);
                entry->setState(S_Inv);
            }
            break;
        case M:
            if (!inMSHR)
                status = allocateMSHR(event, true);
            if (status == MemEventStatus::OK) {
                issueFetch(event, entry, Command::FetchInv);
                entry->setState(M_Inv);
            }
            break;
        case IS:
            if (!mshr->pendingWriteback(addr))
                sendAckInv(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case IM:
            if (!mshr->pendingWriteback(addr))
                sendAckInv(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case I_B:
            sendAckInv(event);
            entry->setState(I);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case S_B:
            if (!inMSHR)
                status = allocateMSHR(event, true, 0); // Put in front of flush
            if (status == MemEventStatus::OK) {
                issueInvalidations(event, entry, Command::Inv);
                entry->setState(SB_Inv);
            }
            break;
        case S_D:
            if (!inMSHR)
                status = allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                issueInvalidations(event, entry, Command::Inv);
                entry->setState(SD_Inv);
            }
            break;
        case S_Inv:
            if (!inMSHR)
                status = allocateMSHR(event, true, 1);
            break;
        case SM_Inv:
            if (!inMSHR)
                status = allocateMSHR(event, true, 0);
            break;
        case M_Inv:
            if (!inMSHR)
                status = allocateMSHR(event, true, 1);
            break;
        case M_InvX:
            if (!inMSHR)
                status = allocateMSHR(event, true, 1);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "%s, Error: Directory received PutM but state is %s. Event = %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}

/* Sent by a shootdown */
bool DirectoryController::handleForceInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry* entry = getDirEntry(addr);
    State state = entry->getState();
    bool cached = entry->isCached();
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::ForceInv, false, addr, state);

    if (!cached)
        return retrieveDirEntry(entry, event, inMSHR);

    if (!inMSHR)
        stat_cacheHits->addData(1);

    switch (state) {
        case I:
            sendAckInv(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case S:
            if (!inMSHR)
                status = allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                issueInvalidations(event, entry, Command::ForceInv);
                entry->setState(S_Inv);
            }
            break;
        case M:
            if (!inMSHR)
                status = allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                issueInvalidation(entry->getOwner(), event, entry, Command::ForceInv);
                entry->setState(M_Inv);
            }
            break;
        case IS:
            if (!(mshr->pendingWriteback(addr)))
                sendAckInv(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case IM:
            if (!(mshr->pendingWriteback(addr)))
                sendAckInv(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case I_B:
            if (!(mshr->pendingWriteback(addr)))
                sendAckInv(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case S_B:
            if (!inMSHR)
                status = allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                issueInvalidations(event, entry, Command::ForceInv);
                entry->setState(SB_Inv);
            }
            break;
        case S_D:
            if (!inMSHR)
                status = allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                issueInvalidations(event, entry, Command::ForceInv);
                entry->setState(SD_Inv);
            }
            break;
        case S_Inv:
            if (!inMSHR)
                status = allocateMSHR(event, true, 1);
            break;
        case SM_Inv:
            if (!inMSHR)
                status = allocateMSHR(event, true, 0);
            break;
        case M_Inv:
            if (!inMSHR)
                status = allocateMSHR(event, true, 1);
            break;
        case M_InvX:
            if (!inMSHR)
                status = allocateMSHR(event, true, 1);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "%s, Error: Directory received PutM but state is %s. Event = %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);
    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    return true;
}

bool DirectoryController::handleGetSResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry* entry = getDirEntry(addr);
    State state = entry->getState();
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::GetSResp, false, addr, state);

    // Entry must be cached since we can't evict non-stable-state entries

    MemEvent * reqEv = static_cast<MemEvent*>(mshr->getFrontEvent(addr));

    if (state != IS && state != S_D) {
        out.fatal(CALL_INFO, -1, "%s, Error: Received GetSResp in unhandled state '%s'. Event: %s. Time: %" PRIu64 "ns\n",
                getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    entry->setState(S);
    entry->addSharer(reqEv->getSrc());

    sendDataResponse(reqEv, entry, event->getPayload(), Command::GetSResp);
    mshr->setData(addr, event->getPayload(), false); // Save data for a subsequent GetS
    cleanUpAfterResponse(event, inMSHR);

    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    return true;
}

bool DirectoryController::handleGetXResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry* entry = getDirEntry(addr);
    State state = entry ? entry->getState() : NP;

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::GetXResp, false, addr, state);

    MemEvent * reqEv = static_cast<MemEvent*>(mshr->getFrontEvent(addr));

    switch (state) {
        case IS:
            if (protocol == CoherenceProtocol::MESI) {
                entry->setState(M);
                entry->setOwner(reqEv->getSrc());
                sendDataResponse(reqEv, entry, event->getPayload(), Command::GetXResp);
                break;
            }
        case S_D:
            entry->setState(S);
            entry->addSharer(reqEv->getSrc());
            sendDataResponse(reqEv, entry, event->getPayload(), Command::GetSResp);
            mshr->setData(addr, event->getPayload(), false); // So subsequent GetS can get data
            break;
        case IM:
            entry->setState(M);
            entry->setOwner(reqEv->getSrc());
            sendDataResponse(reqEv, entry, event->getPayload(), Command::GetXResp);
            break;
        case SM_Inv:
            entry->setState(S_Inv);
            mshr->setData(addr, event->getPayload(), false); // Save data for when the invalidations finish
            if (is_debug_addr(addr)) {
                eventDI.newst = entry->getState();
                eventDI.verboseline = entry->getString();
            }
            delete event;
            return true;
            break;
        default:
            out.fatal(CALL_INFO, -1, "%s, Error: Received GetXResp in unhandled state '%s'. Event: %s. Time: %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    cleanUpAfterResponse(event, inMSHR);
    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    return true;
}

bool DirectoryController::handleFlushLineResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry* entry = getDirEntry(addr);
    State state = entry->getState();

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::FlushLineResp, false, addr, state);

    MemEvent * reqEv = static_cast<MemEvent*>(mshr->getFrontEvent(addr));
    mshr->clearData(addr);

    switch (state) {
        case I:
            break;
        case I_B:
            entry->setState(I);
            break;
        case S_B:
            entry->setState(S);
            break;
        default:
            out.fatal(CALL_INFO, -1, "%s, Error: Received FlushLineResp in unhandled state '%s'. Event: %s. Time: %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    sendResponse(reqEv, event->getFlags(), event->getMemFlags());
    cleanUpAfterResponse(event, inMSHR);
    return true;
}

bool DirectoryController::handleAckPut(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry* entry = getDirEntry(addr);
    State state = entry->getState();

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::AckPut, false, addr, state);

    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    cleanUpAfterResponse(event, inMSHR);

    return true;
}

bool DirectoryController::handleAckInv(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry* entry = getDirEntry(addr);
    State state = entry->getState();

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::AckInv, false, addr, state);

    if (entry->isSharer(event->getSrc()))
        entry->removeSharer(event->getSrc());
    else
        entry->removeOwner();

    bool done = mshr->decrementAcksNeeded(addr);
    responses.find(addr)->second.erase(event->getSrc());
    if (responses.find(addr)->second.empty()) responses.erase(addr);

    if (!done) {
        delete event;
        return true;
    }

    switch (state) {
        case M_Inv: // ForceInv or GetX
            entry->setState(I);
            retryBuffer.push_back(static_cast<MemEvent*>(mshr->getFrontEvent(addr)));
            break;
        case S_Inv:
            entry->hasSharers() ? entry->setState(S) : entry->setState(I);
            retryBuffer.push_back(static_cast<MemEvent*>(mshr->getFrontEvent(addr)));
            break;
        case SB_Inv:
            entry->hasSharers() ? entry->setState(S_B) : entry->setState(I);
            retryBuffer.push_back(static_cast<MemEvent*>(mshr->getFrontEvent(addr)));
            break;
        case SD_Inv:
            entry->hasSharers() ? entry->setState(S_D) : entry->setState(IS);
            retryBuffer.push_back(static_cast<MemEvent*>(mshr->getFrontEvent(addr)));
            break;
        case SM_Inv:
            entry->setState(IM);
            break;
        default:
            out.fatal(CALL_INFO, -1, "%s, Error: Received AckInv in unhandled state '%s'. Event: %s. Time: %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    delete event;

    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    return true;
}

bool DirectoryController::handleFetchXResp(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry* entry = getDirEntry(addr);
    State state = entry->getState();

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::FetchInvX, false, addr, state);

    if (state != M_InvX)
        out.fatal(CALL_INFO, -1, "%s, Error: Received FetchXResp in unhandled state '%s'. Event: %s. Time: %" PRIu64 "ns\n",
                getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());

    mshr->decrementAcksNeeded(addr);
    responses.find(addr)->second.erase(event->getSrc());
    if (responses.find(addr)->second.empty()) responses.erase(addr);

    mshr->setData(addr, event->getPayload(), event->getDirty());       // Save data for retry

    entry->removeOwner();
    entry->addSharer(event->getSrc());
    entry->setState(S);
    retryBuffer.push_back(static_cast<MemEvent*>(mshr->getFrontEvent(addr)));

    delete event;

    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    return true;
}

bool DirectoryController::handleFetchResp(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry* entry = getDirEntry(addr);
    State state = entry->getState();

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::FetchResp, false, addr, state);

    if (state != S_Inv && state != M_Inv)
        out.fatal(CALL_INFO, -1, "%s, Error: Received FetchResp in unhandled state '%s'. Event: %s. Time: %" PRIu64 "ns\n",
                getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());

    MemEvent * reqEv = static_cast<MemEvent*>(mshr->getFrontEvent(addr));

    mshr->decrementAcksNeeded(addr);
    responses.find(addr)->second.erase(event->getSrc());
    if (responses.find(addr)->second.empty())
        responses.erase(addr);
    mshr->setData(addr, event->getPayload(), event->getDirty());       // Save data for retry

    entry->setState(I);

    retryBuffer.push_back(static_cast<MemEvent*>(mshr->getFrontEvent(addr)));

    if (event->getDirty())
        writebackDataFromMSHR(addr);

    delete event;

    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    return true;
}

bool DirectoryController::handleNACK(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirEntry* entry = getDirEntry(addr);
    State state = entry->getState();

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::NACK, false, addr, state);

    MemEvent * nackedEvent = event->getNACKedEvent();
    delete event;

    switch (nackedEvent->getCmd()) {
        case Command::GetS:
        case Command::GetX:
        case Command::GetSX:
        case Command::PutM:
        case Command::FlushLine:
        case Command::FlushLineInv:
            // Always retry
            break;
        case Command::FetchInv:
        case Command::FetchInvX:
        case Command::Inv:
        case Command::ForceInv:
            // Only retry if we still need the response)
            if (responses.find(addr) != responses.end()
                    && responses.find(addr)->second.find(nackedEvent->getDst()) != responses.find(addr)->second.end()
                    && responses.find(addr)->second.find(nackedEvent->getDst())->second == nackedEvent->getID())
                break;
            delete nackedEvent;
            return true;
        default:
            out.fatal(CALL_INFO, -1, "%s, Error: Received NACK in unhandled state '%s'. Event: %s. Time: %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], nackedEvent->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    // Resend nack'd event
    cpuMsgQueue.insert(std::make_pair(timestamp + mshrLatency, nackedEvent)); // Resend after MSHR lookup (assuming we store info about responses there)

    if (is_debug_addr(addr)) {
        eventDI.newst = entry->getState();
        eventDI.verboseline = entry->getString();
    }

    return true;
}

/****************************
 * Manage data structures
 ****************************/
DirectoryController::DirEntry* DirectoryController::getDirEntry(Addr addr) {
    std::unordered_map<Addr,DirEntry*>::iterator i = directory.find(addr);

    if (directory.end() == i) {
        directory[addr] = new DirEntry(addr);
        i = directory.find(addr);
        i->second->cacheIter = entryCache.end();
        i->second->setCached(true);

    }
    return i->second;
}

bool DirectoryController::retrieveDirEntry(DirEntry* entry, MemEvent* event, bool inMSHR) {
    MemEventStatus status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);
    if (status == MemEventStatus::Reject)
        return false;
    else if (status == MemEventStatus::Stall)
        return true;

    State state = entry->getState();
    switch (state) {
        case I:
            entry->setState(I_d);
            break;
        case S:
            entry->setState(S_d);
            break;
        case M:
            entry->setState(M_d);
            break;
        case I_d:
        case S_d:
        case M_d:
            return true;
        default:
            dbg.fatal(CALL_INFO, 01, "%s, Error: Attempting to retrieve a directory entry from memory in state '%s'. Address: 0x%" PRIx64 ". Time: %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], entry->getBaseAddr(), getCurrentSimTimeNano());
    }

    MemEvent* me = new MemEvent(getName(), 0, 0, Command::GetS, lineSize);
    me->setAddrGlobal(false);
    me->setSize(entrySize);

    uint64_t deliveryTime = timestamp + accessLatency;

    memMsgQueue.insert(std::make_pair(deliveryTime, MemMsg(me, true)));

    return true;
}

MemEventStatus DirectoryController::allocateMSHR(MemEvent* event, bool fwdReq, int pos) {
    int end_pos = mshr->insertEvent(event->getBaseAddr(), event, pos, fwdReq, false);
    if (end_pos == -1) {
        if (is_debug_event(event)) {
            eventDI.action = "Reject";
            eventDI.reason = "MSHR full";
        }
        return MemEventStatus::Reject; // MSHR is full
    } else if (end_pos != 0) {
        if (is_debug_event(event)) {
            eventDI.action = "Stall";
            eventDI.reason = "MSHR conflict";
        }
        return MemEventStatus::Stall; // Another event in front of this one
    } else
        return MemEventStatus::OK;
}

void DirectoryController::cleanUpAfterRequest(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();

    if (inMSHR) {
        if (mshr->getFrontType(addr) == MSHREntryType::Event)
            mshr->removeFront(addr);
        else
            mshr->removeEntry(addr, 1); // Sometimes we've inserted a writeback in response to a request and we need to skip the writeback to remove the request
    }

    delete event;

    if (mshr->exists(addr) && mshr->getFrontType(addr) == MSHREntryType::Event) {
        if (!mshr->getInProgress(addr) && mshr->getAcksNeeded(addr) == 0) {
            retryBuffer.push_back(static_cast<MemEvent*>(mshr->getFrontEvent(addr)));
        }
    }
}

void DirectoryController::cleanUpAfterResponse(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();

    MemEvent * req = nullptr;
    if (mshr->getFrontType(addr) == MSHREntryType::Event) {
        req = static_cast<MemEvent*>(mshr->getFrontEvent(addr));
    }

    mshr->removeFront(addr);
    delete event;
    if (req)
        delete req;

    if (mshr->exists(addr) && mshr->getFrontType(addr) == MSHREntryType::Event) {
        if (!mshr->getInProgress(addr) && mshr->getAcksNeeded(addr) == 0) {
            retryBuffer.push_back(static_cast<MemEvent*>(mshr->getFrontEvent(addr)));
        }
    }
}

void DirectoryController::updateCache(DirEntry * entry) { // TODO replace with a proper cache!
    if (0 == entryCacheMaxSize) {
        sendEntryToMemory(entry);
    } else {
        if (entry->cacheIter != entryCache.end()) {
            entryCache.erase(entry->cacheIter);
            --entryCacheSize;
            entry->cacheIter = entryCache.end();
        }

        if (entry->getState() == I) {
            directory.erase(entry->getBaseAddr());
            delete entry;
            return;
        } else  {
            entryCache.push_front(entry);
            entry->cacheIter = entryCache.begin();
            ++entryCacheSize;

            while (entryCacheSize > entryCacheMaxSize) {
                DirEntry * oldEntry = entryCache.back();
                if (mshr->exists(oldEntry->getBaseAddr()))
                    break;

                entryCache.pop_back();
                --entryCacheSize;
                oldEntry->cacheIter = entryCache.end();
                oldEntry->setCached(false);
                sendEntryToMemory(oldEntry);
            }
        }
    }
}

void DirectoryController::sendEntryToMemory(DirEntry *entry) {
    Addr entryAddr = 0;
    MemEvent * me = new MemEvent(getName(), entryAddr, entryAddr, Command::PutE, lineSize);
    me->setSize(entrySize);

    uint64_t deliveryTime = timestamp + accessLatency;
    if (memoryName == "")
        me->setDst(memLink->findTargetDestination(0));
    else
        me->setDst(memoryName);
    memMsgQueue.insert(std::make_pair(deliveryTime, MemMsg(me, true)));
}

/****************************
 * Send events
 ****************************/

void DirectoryController::issueMemoryRequest(MemEvent* event, DirEntry* entry) {
    MemEvent* reqEvent = new MemEvent(*event);
    reqEvent->setSrc(getName());
    if (memoryName == "")
        reqEvent->setDst(memLink->findTargetDestination(reqEvent->getRoutingAddress()));
    else
        reqEvent->setDst(memoryName);
    memReqs[reqEvent->getID()] = event->getBaseAddr();
    uint64_t deliveryTime = timestamp + accessLatency;

    memMsgQueue.insert(std::make_pair(deliveryTime, MemMsg(reqEvent, false)));

    mshr->setInProgress(entry->getBaseAddr());
}

void DirectoryController::issueFlush(MemEvent* event) {
    Addr addr = event->getBaseAddr();
    MemEvent * flush = new MemEvent(*event);
    flush->setSrc(getName());
    if (memoryName == "")
        flush->setDst(memLink->findTargetDestination(event->getRoutingAddress()));
    else
        flush->setDst(memoryName);
    memReqs[flush->getID()] = addr;

    if (mshr->hasData(addr) && mshr->getDataDirty(addr)) { // also writeback dirty data
        flush->setEvict(true);
        flush->setPayload(mshr->getData(addr));
        flush->setDirty(true);
        mshr->clearData(addr); // Don't retain data
    } else {
        flush->setPayload(0, nullptr);
    }

    mshr->setInProgress(addr);

    uint64_t deliveryTime = timestamp + accessLatency;
    memMsgQueue.insert(std::make_pair(deliveryTime, MemMsg(flush, false)));
}

void DirectoryController::issueFetch(MemEvent* event, DirEntry* entry, Command cmd) {
    Addr addr = event->getBaseAddr();
    MemEvent * fetch = new MemEvent(getName(), event->getAddr(), addr, cmd, lineSize);
    fetch->setDst(entry->getOwner());

    if (responses.find(addr) == responses.end()) {
        std::map<std::string,MemEvent::id_type> resp;
        resp.insert(std::make_pair(entry->getOwner(), fetch->getID()));
        responses.insert(std::make_pair(addr, resp));
    } else {
        responses.find(addr)->second.insert(std::make_pair(entry->getOwner(), fetch->getID()));
    }

    mshr->incrementAcksNeeded(addr);

    cpuMsgQueue.insert(std::make_pair(timestamp+accessLatency, fetch));
}

void DirectoryController::issueInvalidations(MemEvent* event, DirEntry* entry, Command cmd) {
    std::string rqstr = (event->getSrc());

    for (set<std::string>::iterator it = entry->getSharers()->begin(); it != entry->getSharers()->end(); it++) {
        if (*it == rqstr) continue;
        issueInvalidation(*it, event, entry, cmd);
    }
}

void DirectoryController::issueInvalidation(std::string dst, MemEvent* event, DirEntry* entry, Command cmd) {
    Addr addr = entry->getBaseAddr();
    MemEvent* inv = new MemEvent(getName(), addr, addr, cmd, lineSize);
    if (event) {
        inv->copyMetadata(event);
        inv->setRqstr(event->getRqstr());
    } else {
        inv->setRqstr(getName());
    }
    inv->setDst(dst);

    mshr->incrementAcksNeeded(addr);

    if (responses.find(addr) == responses.end()) {
        std::map<std::string,MemEvent::id_type> resp;
        resp.insert(std::make_pair(entry->getOwner(), inv->getID()));
        responses.insert(std::make_pair(addr, resp));
    } else {
        responses.find(addr)->second.insert(std::make_pair(entry->getOwner(), inv->getID()));
    }

    uint64_t deliveryTime = timestamp + accessLatency;
    cpuMsgQueue.insert(std::make_pair(deliveryTime, inv));
}

void DirectoryController::sendDataResponse(MemEvent* event, DirEntry* entry, std::vector<uint8_t>& data, Command cmd, uint32_t flags) {
    MemEvent * respEv = event->makeResponse(cmd);
    respEv->setSize(lineSize);
    respEv->setPayload(data);
    respEv->setMemFlags(flags);
    cpuMsgQueue.insert(std::make_pair(timestamp+mshrLatency, respEv));
}

void DirectoryController::sendResponse(MemEvent* event, uint32_t flags, uint32_t memflags) {
    MemEvent * respEv = event->makeResponse();
    respEv->setSize(lineSize);
    respEv->setMemFlags(memflags);
    respEv->setFlags(flags);
    cpuMsgQueue.insert(std::make_pair(timestamp + mshrLatency, respEv));
}

void DirectoryController::writebackData(MemEvent* event) {
    MemEvent * wb = new MemEvent(getName(), event->getBaseAddr(), event->getBaseAddr(), Command::PutM, lineSize);
    wb->copyMetadata(event);
    wb->setRqstr(event->getRqstr());
    if (memoryName == "")
        wb->setDst(memLink->findTargetDestination(wb->getRoutingAddress()));
    else
        wb->setDst(memoryName);

    if (waitWBAck)
        mshr->insertWriteback(event->getBaseAddr(), false);

    uint64_t deliveryTime = timestamp + accessLatency;
    memMsgQueue.insert(std::make_pair(deliveryTime, MemMsg(wb, false)));
}

void DirectoryController::writebackDataFromMSHR(Addr addr) {
    MemEvent * wb = new MemEvent(getName(), addr, addr, Command::PutM, lineSize);
    if (memoryName == "")
        wb->setDst(memLink->findTargetDestination(wb->getRoutingAddress()));
    else
        wb->setDst(memoryName);

    mshr->setDataDirty(addr, false);

    if (waitWBAck)
        mshr->insertWriteback(addr, false);

    uint64_t deliveryTime = timestamp + mshrLatency;
    memMsgQueue.insert(std::make_pair(deliveryTime, MemMsg(wb, false)));
}

void DirectoryController::sendFetchResponse(MemEvent * event) {
    Addr addr = event->getBaseAddr();
    MemEvent * ack = event->makeResponse();
    if (memoryName == "")
        ack->setDst(memLink->findTargetDestination(ack->getRoutingAddress()));
    else
        ack->setDst(memoryName);

    ack->setPayload(mshr->getData(addr));
    ack->setDirty(mshr->getDataDirty(addr));

    mshr->clearData(addr);

    memMsgQueue.insert(std::make_pair(timestamp + accessLatency, MemMsg(ack, false)));
}

void DirectoryController::sendAckInv(MemEvent * event) {
    Addr addr = event->getBaseAddr();
    MemEvent * ack = event->makeResponse(Command::AckInv);
    if (memoryName == "")
        ack->setDst(memLink->findTargetDestination(ack->getRoutingAddress()));
    else
        ack->setDst(memoryName);

    if (mshr->hasData(addr))
        mshr->clearData(addr);

    memMsgQueue.insert(std::make_pair(timestamp + accessLatency, MemMsg(ack, false)));
}

void DirectoryController::sendAckPut(MemEvent * event) {
    Addr addr = event->getBaseAddr();
    MemEvent * ack = event->makeResponse(Command::AckPut);

    cpuMsgQueue.insert(std::make_pair(timestamp + accessLatency, ack));
}

void DirectoryController::sendNACK(MemEvent * event) {
    MemEvent * nack = event->makeNACKResponse(event);

    uint64_t deliveryTime = timestamp + accessLatency;

    cpuMsgQueue.insert(std::make_pair(deliveryTime, nack));
}


// Return whether there are pending outgoing events
void DirectoryController::sendOutgoingEvents() {

    bool debugLine = false;
    while (!cpuMsgQueue.empty() && cpuMsgQueue.begin()->first <= timestamp) {
        MemEventBase * ev = cpuMsgQueue.begin()->second;

        if (is_debug_event(ev)) {
            dbg.debug(_L4_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Send    (%s)\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), timestamp, getName().c_str(), ev->getBriefString().c_str());
        }
        if (startTimes.find(ev->getResponseToID()) != startTimes.end()) {
            if (CommandClassArr[(int)ev->getCmd()] == CommandClass::Data)
                stat_getRequestLatency->addData(timestamp - startTimes.find(ev->getResponseToID())->second); // GetS, GetX, GetSX
            else
                stat_replacementRequestLatency->addData(timestamp - startTimes.find(ev->getResponseToID())->second); // Put*, FlushLine*
            startTimes.erase(ev->getResponseToID());
        }
        stat_eventSent[(int)ev->getCmd()]->addData(1);
        cpuLink->send(ev);
        cpuMsgQueue.erase(cpuMsgQueue.begin());
    }

    while (!memMsgQueue.empty() && memMsgQueue.begin()->first <= timestamp) {
        MemEventBase * ev = memMsgQueue.begin()->second.event;

        if (is_debug_event(ev)) {
            dbg.debug(_L4_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Send    (%s)\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), timestamp, getName().c_str(), ev->getBriefString().c_str());
        }

        if (memMsgQueue.begin()->second.dirAccess) {
            if (ev->getCmd() == Command::GetS)
                stat_dirEntryReads->addData(1);
            else
                stat_dirEntryWrites->addData(1);
        } else {
            stat_eventSent[(int)ev->getCmd()]->addData(1);
        }
        memLink->send(ev);
        memMsgQueue.erase(memMsgQueue.begin());
    }

}

void DirectoryController::forwardTowardsMem(MemEventBase* ev) {
    memMsgQueue.insert(std::make_pair(timestamp+1, MemMsg(ev, false)));
}

void DirectoryController::forwardTowardsCPU(MemEventBase* ev) {
    cpuMsgQueue.insert(std::make_pair(timestamp+1, ev));
}

void DirectoryController::recordStartLatency(MemEventBase* ev) {
    startTimes.insert(std::make_pair(ev->getID(), timestamp));
}

void DirectoryController::printDebugInfo() {
    if (dlevel < 5)
            return;

    std::string cmd = CommandString[(int)eventDI.cmd];
    if (eventDI.prefetch)
        cmd += "-pref";

    std::stringstream id;
    id << "<" << eventDI.id.first << "," << eventDI.id.second << ">";

    stringstream reas;
    reas << "(" << eventDI.reason << ")";

    dbg.debug(_L5_, "C: %-20" PRIu64 " %-20" PRIu64 " %-20s %-13s 0x%-16" PRIx64 " %-15s %-6s %-6s %-10s %-15s",
            Simulation::getSimulation()->getCurrentSimCycle(), timestamp, getName().c_str(), cmd.c_str(), eventDI.addr,
            id.str().c_str(), StateString[eventDI.oldst], StateString[eventDI.newst], eventDI.action.c_str(), reas.str().c_str());

    dbg.debug(_L6_, " %s", eventDI.verboseline.c_str());
    dbg.debug(_L5_, "\n");
}
