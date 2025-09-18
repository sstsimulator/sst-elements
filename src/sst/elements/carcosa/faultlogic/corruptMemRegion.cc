// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst/elements/carcosa/faultlogic/corruptMemRegion.h"

using namespace SST::Carcosa;

CorruptMemRegion::CorruptMemRegion(Params& params, FaultInjectorBase* injector) : FaultBase(params, injector) {
#ifdef __SST_DEBUG_OUTPUT__
    getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "Fault type: Corrupt Memory Region\n");
#endif
    // read in data regions
    std::vector<std::string> regionVec;

    // parameter format: {"regions": ["start_addr0, end_addr0", "start_addr1, end_addr1",...]}
    params.find_array<std::string>("regions", regionVec);

    // process entries into region
    for (std::string region: regionVec) {
        std::pair<uint64_t,uint64_t> region_pair = convertString(region);

        // check validity
        if (region_pair.first > region_pair.second) {
            getSimulationOutput()->fatal(CALL_INFO_LONG, 1, 0, "Invalid corruption region: [0x%zx, 0x%zx].\n", 
                                        region_pair.first, region_pair.second);
        }

        corruptionRegions.push_back(region_pair);
#ifdef __SST_DEBUG_OUTPUT__
        getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "Inserted corruption region: [0x%zx, 0x%zx]\n",
                                    region_pair.first, region_pair.second);
#endif
    }
    distribution = std::uniform_int_distribution<uint8_t>(0,255);
}

void CorruptMemRegion::faultLogic(Event*& ev) {
    SST::MemHierarchy::MemEvent* mem_ev = convertMemEvent(ev);

    Addr ev_addr = mem_ev->getAddr();
    for (auto& region: corruptionRegions) {
        if ((ev_addr >= region.first) || (ev_addr <= region.second)) {
            dataVec new_payload(8);
            for (uint8_t& byte: new_payload) {
                byte = distribution(generator);
            }
            setMemEventPayload(ev, new_payload);
            break;
        }
    }
}

std::pair<uint64_t,uint64_t> CorruptMemRegion::convertString(std::string& region) {
    std::stringstream ss(region);
    uint64_t addr0, addr1;

    ss >> std::hex >> addr0;
    if (ss.peek() == ','){
        ss.ignore();
    }
    ss >> std::hex >> addr1;

    #ifdef __SST_DEBUG_OUTPUT__
    getSimulationDebug()->debug(CALL_INFO_LONG, 2, 0, "Extracted region pair: [0x%zu, 0x%zx]\n",
                                addr0, addr1);
    #endif
    return make_pair(addr0, addr1);
}