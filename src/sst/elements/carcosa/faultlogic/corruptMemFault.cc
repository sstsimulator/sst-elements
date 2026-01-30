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

#include "sst/elements/carcosa/faultlogic/corruptMemFault.h"

using namespace SST::Carcosa;

CorruptMemFault::CorruptMemFault(Params& params, FaultInjectorBase* injector) : FaultBase(params, injector) {
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
            getSimulationOutput()->fatal(CALL_INFO_LONG, -1, "Invalid corruption region: [0x%zx, 0x%zx].\n",
                                        region_pair.first, region_pair.second);
        }

        corruptionRegions_.push_back(region_pair);
#ifdef __SST_DEBUG_OUTPUT__
        getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "Inserted corruption region: [0x%zx, 0x%zx]\n",
                                    region_pair.first, region_pair.second);
#endif
    }
}

bool CorruptMemFault::faultLogic(Event*& ev) {
    SST::MemHierarchy::MemEvent* mem_ev = convertMemEvent(ev);

    Addr base_addr = mem_ev->getBaseAddr();
    dataVec original_payload = mem_ev->getPayload();
    dataVec new_payload(original_payload);
    for (int r: regionsToUse_) {
        auto& region = corruptionRegions_[r];
        size_t payload_sz = mem_ev->getPayloadSize();
        int32_t start = computeStartIndex(base_addr, payload_sz, region.first);
        if (start < 0) {
            getSimulationOutput()->fatal(CALL_INFO_LONG, -1, "No valid start index for corruption.\n");
        }
        int32_t end = computeEndIndex(base_addr, payload_sz, region.second);
        if (end < 0) {
            getSimulationOutput()->fatal(CALL_INFO_LONG, -1, "No valid start index for corruption.\n");
        }
        for (int i = start; i < end; i++) {
            new_payload[i] = static_cast<uint8_t>(injector_->randUInt32(0,255));
        }
    }
    setMemEventPayload(ev, new_payload);
    return true;
}

std::pair<uint64_t,uint64_t> CorruptMemFault::convertString(std::string& region) {
    std::stringstream ss(region);
    uint64_t addr0, addr1;

    ss >> std::hex >> addr0;
    if (ss.peek() == ','){
        ss.ignore();
    }
    ss >> std::hex >> addr1;

#ifdef __SST_DEBUG_OUTPUT__
    getSimulationDebug()->debug(CALL_INFO_LONG, 2, 0, "Extracted region pair: [0x%zx, 0x%zx]\n",
                                addr0, addr1);
#endif
    return make_pair(addr0, addr1);
}

int32_t CorruptMemFault::computeStartIndex(Addr base_addr, size_t payload_sz, Addr region_start) {
    // start index is always the first byte of this payload in the corruption region
    int payload_bytes = payload_sz / 8;
    Addr addr = base_addr;
    for (int i = 0; i < payload_bytes; i++, addr+=8) {
        if (addr >= region_start) {
            return addr - base_addr;
        }
    }
    return -1;
}

int32_t CorruptMemFault::computeEndIndex(Addr base_addr, size_t payload_sz, Addr region_end) {
    // end index is either the final addr's final byte, or the region end's addr's final byte
    int payload_bytes = payload_sz / 8;
    Addr addr = base_addr + ((payload_bytes - 1) * 8);
    for (int i = payload_bytes; i >= 0; i--, addr-=8) {
        if (addr <= region_end) {
            return addr - base_addr + 8;
        }
    }
    return -1;
}

std::vector<uint32_t>* CorruptMemFault::checkAddrUsage(Event*& ev) {
    Addr base_addr = convertMemEvent(ev)->getBaseAddr();
    for (int i = 0; i < corruptionRegions_.size(); i++) {
        auto& region = corruptionRegions_[i];
        // check if message contains ANY address in this region
        int payload_bytes = convertMemEvent(ev)->getPayloadSize() / 8;
        Addr addr = base_addr;
        for (int j = 0; j < payload_bytes; addr+=8, j++) {
            if ((addr >= region.first) && (addr <= region.second)) {
                regionsToUse_.push_back(i);
                break;
            }
        }
    }
    return &regionsToUse_;
}