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

    void tick() { cycles_left--; }

    uint32_t getHardwareThread() const { return ins->getHWThread(); }

    VanadisInstruction* getInstruction() { return ins; }

private:
    VanadisInstruction* ins;
    uint16_t cycles_left;
};

class VanadisFunctionalUnit {

public:
    VanadisFunctionalUnit(uint16_t id, VanadisFunctionalUnitType unit_type, uint16_t lat)
        : fu_id(id), fu_type(unit_type), latency(lat) {

        slot_inst = nullptr;
    }

    ~VanadisFunctionalUnit() {
        for (auto q_itr = pending_execute.begin(); q_itr != pending_execute.end(); q_itr++) {
            delete (*q_itr);
        }
    }

    VanadisFunctionalUnitType getType() const { return fu_type; }

    bool isInstructionSlotFree() const { return slot_inst == nullptr; }

    void setSlotInstruction(VanadisInstruction* ins) { slot_inst = ins; }

    uint16_t getUnitID() const { return fu_id; }

    void tick(const uint64_t cycle, SST::Output* output, std::vector<VanadisRegisterFile*>& regFile) {
        if (pending_execute.size() > 0) {
            VanadisFunctionalUnitInsRecord* q_front = pending_execute.front();

            if (q_front->readyToExecute()) {
                // ready to execute, remove from pending queue
                pending_execute.pop_front();

                // Perform execute
                VanadisInstruction* inner_ins = q_front->getInstruction();
                inner_ins->execute(output, regFile[inner_ins->getHWThread()]);

                // Delete the record entry for functional unit
                delete q_front;
            }

            for (auto q_itr = pending_execute.begin(); q_itr != pending_execute.end(); q_itr++) {
                (*q_itr)->tick();
            }
        }

        // Were we given a new instruction this cycle? If yes, then add to the queue
        if (slot_inst != nullptr) {
            pending_execute.push_back(new VanadisFunctionalUnitInsRecord(slot_inst, latency));
            slot_inst = nullptr;
        }
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

        if (slot_inst != nullptr) {
            if (slot_inst->getHWThread() == hw_thr) {
                slot_inst = nullptr;
            }
        }
    }

private:
    std::deque<VanadisFunctionalUnitInsRecord*> pending_execute;

    const uint16_t latency;
    VanadisFunctionalUnitType fu_type;
    VanadisInstruction* slot_inst;
    const uint16_t fu_id;
};

} // namespace Vanadis
} // namespace SST

#endif
