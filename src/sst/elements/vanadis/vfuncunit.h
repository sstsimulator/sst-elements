// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_FUNCTIONAL_UNIT
#define _H_VANADIS_FUNCTIONAL_UNIT

#include <cinttypes>
#include <climits>
#include <cstdint>
#include <deque>

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisFunctionalUnitInsRecord {
public:
    VanadisFunctionalUnitInsRecord(VanadisInstruction* base_ins, uint16_t cycles)
        : ins(base_ins), cycles_left(cycles) {}
    ~VanadisFunctionalUnitInsRecord() {}

    uint16_t getCycles() const { return cycles_left; }

    bool readyToExecute() const { return (0 == cycles_left); }

    void tick() { cycles_left = (cycles_left > 0) ? cycles_left - 1 : 0; }

    uint32_t getHardwareThread() const { return ins->getHWThread(); }

    VanadisInstruction* getInstruction() { return ins; }

private:
    VanadisInstruction* ins;
    uint16_t cycles_left;
};

class VanadisFunctionalUnit {

public:
    VanadisFunctionalUnit(uint16_t id, VanadisFunctionalUnitType unit_type, uint16_t lat)
        : fu_id(id), fu_type(unit_type), latency(lat), accept_this_cycle(true) {
    }

    ~VanadisFunctionalUnit() {
        for (auto q_itr = pending_execute.begin(); q_itr != pending_execute.end(); q_itr++) {
            delete (*q_itr);
        }
    }

    VanadisFunctionalUnitType getType() const { return fu_type; }

    bool isInstructionSlotFree() const { return accept_this_cycle; }

    void insertInstruction(VanadisInstruction* ins) {
        pending_execute.push_back(new VanadisFunctionalUnitInsRecord(ins, latency));
        accept_this_cycle = false;
    }

    uint16_t getUnitID() const { return fu_id; }

    void tick(const uint64_t cycle, SST::Output* output, std::vector<VanadisRegisterFile*>& regFile) {
        for(auto q_itr = pending_execute.begin(); q_itr != pending_execute.end();) {
            VanadisFunctionalUnitInsRecord* q_item = (*q_itr);

            if(q_item->readyToExecute()) {
                VanadisInstruction* inner_ins = q_item->getInstruction();
                inner_ins->execute(output, regFile[inner_ins->getHWThread()]);

                if(LIKELY(inner_ins->completedExecution())) {
                    // Delete the record entry for functional unit if the instruction marked itself executed
                    delete q_item;

                    // ready to execute, remove from pending queue
                    q_itr = pending_execute.erase(q_itr);
                } else {
                    q_itr++;
                }
            } else {
                q_item->tick();
                q_itr++;
            }
        }

        accept_this_cycle = true;
    }

    void clearByHWThreadID(SST::Output* output, const uint16_t hw_thr) {
        output->verbose(CALL_INFO, 16, 0, "-> Function Unit, clearing by hardware thread %" PRIu32 "...\n", hw_thr);

        for (auto q_itr = pending_execute.begin(); q_itr != pending_execute.end();) {
            // if we get a hardware thread match, remove and carry out
            if ((*q_itr)->getHardwareThread() == hw_thr) {
                delete (*q_itr);
                q_itr = pending_execute.erase(q_itr);
            } else {
                q_itr++;
            }
        }
    }

    void print(SST::Output* output) {
        uint16_t index = 0;

        for (auto q_itr = pending_execute.begin(); q_itr != pending_execute.end(); q_itr++) {
            VanadisFunctionalUnitInsRecord* q_front = pending_execute.front();
            output->verbose(CALL_INFO, 16, 0, "----> func-unit: %" PRIu16 " %" PRIu16 " entries / entry: %" PRIu16 " / %s / 0x%llx / cycles: %" PRIu16 " out of %" PRIu16 "\n",
                fu_id, (uint16_t) pending_execute.size(), index++, q_front->getInstruction()->getInstCode(),
                q_front->getInstruction()->getInstructionAddress(), q_front->getCycles(), latency);
        }
    }

private:
    std::deque<VanadisFunctionalUnitInsRecord*> pending_execute;

    const uint16_t latency;
    VanadisFunctionalUnitType fu_type;
    const uint16_t fu_id;
    bool accept_this_cycle;
};

} // namespace Vanadis
} // namespace SST

#endif
