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

#include "sst/elements/carcosa/faultlogic/stuckAtFault.h"

using namespace SST::Carcosa;

/********** StuckAtFault **********/

StuckAtFault::StuckAtFault(Params& params, FaultInjectorBase* injector) : FaultBase(params, injector) 
{
#ifdef __SST_DEBUG_OUTPUT__
    getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "Fault Type: Stuck-At Fault\n");
#endif
    // read in masks
    // parameter format: {masks: ["addr, byte, zeroMask, oneMask"]}
    std::vector<std::string> paramVecStr;
    params.find_array<std::string>("masks", paramVecStr);

    std::vector<maskParam_t> paramVec = convertString(paramVecStr);
    // build maps
    for (auto param = paramVec.begin(); param != paramVec.end(); param++) {
        Addr addr = param->addr;
        int byte = param->byte;
        uint8_t zeroMask = param->zeroMask;
        uint8_t oneMask = param->oneMask;
        if ((int)(zeroMask & oneMask) > 0) {
            getSimulationOutput()->fatal(CALL_INFO_LONG, -1, "Masks contain overlapping values. Addr: 0x%zx, " 
                                        "byte: %d\n", addr, byte);
        }
        // check for vector in each map before creating it
        if (stuckAtZeroMask_.count(addr) == 1) {
            stuckAtZeroMask_.at(addr).push_back(make_pair(byte, zeroMask));
        } else {
            auto addrVecPair = stuckAtZeroMask_.emplace(make_pair(addr, std::vector<std::pair<int, uint8_t>>()));
            if (addrVecPair.second) {
                addrVecPair.first->second.push_back(make_pair(byte, zeroMask));
            } else {
                getSimulationOutput()->fatal(CALL_INFO_LONG, -1, "Failed to insert mask.\n");
            }
        }
#ifdef __SST_DEBUG_OUTPUT__
        getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "Finished inserting zero-masks for 0x%zx.\n", addr);
#endif
        if (stuckAtOneMask_.count(addr) == 1) {
            stuckAtOneMask_.at(addr).push_back(make_pair(byte, oneMask));
        } else {
            auto addrVecPair = stuckAtOneMask_.emplace(make_pair(addr, std::vector<std::pair<int, uint8_t>>()));
            if (addrVecPair.second) {
                addrVecPair.first->second.push_back(make_pair(byte, oneMask));
            } else {
                getSimulationOutput()->fatal(CALL_INFO_LONG, -1, "Failed to insert mask.\n");
            }
        }
#ifdef __SST_DEBUG_OUTPUT__
        getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "Finished inserting one-masks for 0x%zx.\n", addr);
#endif
    }
}

bool StuckAtFault::faultLogic(SST::Event*& ev) {
    // Convert to memEvent
    SST::MemHierarchy::MemEvent* mem_ev = this->convertMemEvent(ev);

    Addr addr = mem_ev->getAddr();

    // check for the addr in question in the fault map
    if (stuckAtZeroMask_.count(addr) == 1 || stuckAtOneMask_.count(addr) == 1) {
#ifdef __SST_DEBUG_OUTPUT__
        getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "Addr 0x%zx found in stuck map.\n", addr);
#endif
        // replace data if necessary
        dataVec payload = this->getMemEventPayload(ev);
        // TODO: review everything in below comment and adjust this function
        // payloads are given a size in the memEvent and it's usually cache line size
        // addr is whatever the core requested
        // base addr is first addr in byte array
        // vanadis riscv is LITTLE ENDIAN so byte order is reversed (byte 0 at Addr A is lowest byte, but Addr0 is still base addr, Addr1 = Addr0+8)
        // confirm that little endian is this trolling

        uint8_t mask = 0b00000000;
#ifdef __SST_DEBUG_OUTPUT__
        getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "Begin zero mask for address: 0x%zx\n", addr);
#endif
        if (stuckAtZeroMask_.count(addr) == 1) {
            for (auto maskPair: stuckAtZeroMask_.at(addr)) {
                mask = maskPair.second;
#ifdef __SST_DEBUG_OUTPUT__
                getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "\tbyte %d, value: %d, mask: %d, new value: %d\n",
                                            maskPair.first, (int)payload[maskPair.first], (int) mask, 
                                            (int)(payload[maskPair.first] &= (!mask)));
#endif
                payload[maskPair.first] &= (!mask);
            }
        }
#ifdef __SST_DEBUG_OUTPUT__
        getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "End zero mask for address: 0x%zx\n", addr);
        getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "Begin one mask for address: 0x%zx\n", addr);
#endif
        if (stuckAtOneMask_.count(addr) == 1) {
            for (auto maskPair: stuckAtOneMask_.at(addr)) {
                mask = maskPair.second;
#ifdef __SST_DEBUG_OUTPUT__
                getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "\tbyte %d, value: %d, mask: %d, new value: %d\n",
                                            maskPair.first, (int)payload[maskPair.first], (int) mask, 
                                            (int)(payload[maskPair.first] |= mask));
#endif
                payload[maskPair.first] |= mask;
            }
#ifdef __SST_DEBUG_OUTPUT__
            getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "End one mask for address: 0x%zx\n", addr);
#endif
        }

        // replace payload
        this->setMemEventPayload(ev, payload);
    }
    return true;
}

std::vector<StuckAtFault::maskParam_t> StuckAtFault::convertString(std::vector<std::string>& paramVecString) {
    std::vector<maskParam_t> paramVec;

    for (auto param = paramVecString.begin(); param != paramVecString.end(); param++) {
        // disassemble string
        std::stringstream stream;
        Addr addr; int byte; std::string zeroMaskStr, oneMaskStr; uint8_t zeroMask, oneMask;
        stream.str(*param);
        stream >> std::hex >> addr;
        if (stream.peek() == ',') {
            stream.ignore();
        }
        stream >> std::dec >> byte;
        if (stream.peek() == ',') {
            stream.ignore();
        }
        stream >> zeroMaskStr;
        zeroMask = static_cast<uint8_t>(std::bitset<8>(zeroMaskStr).to_ulong());
        if (stream.peek() == ',') {
            stream.ignore();
        }
        stream >> oneMaskStr;
        oneMask = static_cast<uint8_t>(std::bitset<8>(oneMaskStr).to_ulong());
        if (stream.peek() == ',') {
            stream.ignore();
        }
#ifdef __SST_DEBUG_OUTPUT__
        getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "Masks for addr 0x%zx, byte %d: %d %d\n", addr, byte, (int)zeroMask, (int)oneMask);
#endif
        // insert maskParam
        paramVec.push_back({addr, byte, zeroMask, oneMask});
    }

    return paramVec;
}