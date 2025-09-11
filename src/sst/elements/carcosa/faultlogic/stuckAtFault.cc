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
#ifdef DEBUG
    getSimulationOutput().debug(CALL_INFO_LONG, 1, 0, "\Fault Type: Stuck-At Fault");
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
            getSimulationOutput().fatal(CALL_INFO_LONG, -1, "Masks contain overlapping values. Addr: 0x%zu, " 
                                        "byte: %d\n", addr, byte);
        }
        // check for vector in each map before creating it
        if (stuckAtZeroMask.count(addr) == 1) {
            stuckAtZeroMask.at(addr).push_back(make_pair(byte, zeroMask));
        } else {
            auto maskVec = stuckAtZeroMask.emplace(make_pair(addr, std::vector<std::pair<int, uint8_t>>()));
            if (maskVec.second) {
                maskVec.first->second.push_back(make_pair(byte, zeroMask));
            } else {
                getSimulationOutput().fatal(CALL_INFO_LONG, -1, "Failed to insert mask.\n");
            }
        }
#ifdef DEBUG
        getSimulationOutput().debug(_L10_, "Finished inserting zero-masks for 0x%zu.\n", addr);
#endif
        if (stuckAtOneMask.count(addr) == 1) {
            stuckAtOneMask.at(addr).push_back(make_pair(byte, oneMask));
        } else {
            auto maskVec = stuckAtOneMask.emplace(make_pair(addr, std::vector<std::pair<int, uint8_t>>()));
            if (maskVec.second) {
                maskVec.first->second.push_back(make_pair(byte, oneMask));
            } else {
                getSimulationOutput().fatal(CALL_INFO_LONG, -1, "Failed to insert mask.\n");
            }
        }
#ifdef DEBUG
        getSimulationOutput().debug(_L10_, "Finished inserting one-masks for 0x%zu.\n", addr);
#endif
    }
}

void StuckAtFault::faultLogic(SST::Event*& ev) {
    // Convert to memEvent
    SST::MemHierarchy::MemEvent* mem_ev = this->convertMemEvent(ev);

    Addr addr = mem_ev->getAddr();

    // check for the addr in question in the fault map
    if (stuckAtZeroMask.count(addr) == 1 || stuckAtOneMask.count(addr) == 1) {
#ifdef DEBUG
        getSimulationOutput().debug(_L10_, "Addr 0x%x found in stuck map.\n", addr);
#endif
        // replace data if necessary
        dataVec payload = this->getMemEventPayload(ev);

        uint8_t mask = 0b00000000;
#ifdef DEBUG
        getSimulationOutput().debug(_L10_, "Begin zero mask for address: 0x%zu\n", addr);
#endif
        if (stuckAtZeroMask.count(addr) == 1) {
            for (auto maskPair: stuckAtZeroMask.at(addr)) {
                mask = maskPair.second;
#ifdef DEBUG
                getSimulationOutput().debug(_L10_, "\tbyte %d, value: %d, mask: %d, new value: %d\n",
                                            maskPair.first, (int)payload[maskPair.first], (int) mask, 
                                            (int)(payload[maskPair.first] &= (!mask)));
#endif
                payload[maskPair.first] &= (!mask);
            }
        }
#ifdef DEBUG
        getSimulationOutput().debug(_L10_, "End zero mask for address: 0x%zu\n", addr);
        getSimulationOutput().debug(_L10_, "Begin one mask for address: 0x%zu\n", addr);
#endif
        if (stuckAtOneMask.count(addr) == 1) {
            for (auto maskPair: stuckAtOneMask.at(addr)) {
                mask = maskPair.second;
#ifdef DEBUG
                getSimulationOutput().debug(_L10_, "\tbyte %d, value: %d, mask: %d, new value: %d\n",
                                            maskPair.first, (int)payload[maskPair.first], (int) mask, 
                                            (int)(payload[maskPair.first] |= mask));
#endif
                payload[maskPair.first] |= mask;
            }
#ifdef DEBUG
            getSimulationOutput().debug(_L10_, "End one mask for address: 0x%zu\n", addr);
#endif
        }

        // replace payload
        this->setMemEventPayload(ev, payload);
    }
}

std::vector<StuckAtFault::maskParam_t> StuckAtFault::convertString(std::vector<std::string>& paramVecString) {
    std::vector<maskParam_t> paramVec;

    for (auto param = paramVecString.begin(); param != paramVecString.end(); param++) {
        // disassemble string
        std::stringstream stream;
        Addr addr; int byte; uint8_t zeroMask, oneMask;
        stream.str(*param);
        stream >> addr;
        if (stream.peek() == ',') {
            stream.ignore();
        }
        stream >> byte;
        if (stream.peek() == ',') {
            stream.ignore();
        }
        stream >> zeroMask;
        if (stream.peek() == ',') {
            stream.ignore();
        }
        stream >> oneMask;
        if (stream.peek() == ',') {
            stream.ignore();
        }
        // insert maskParam
        paramVec.push_back({addr, byte, zeroMask, oneMask});
    }

    return paramVec;
}