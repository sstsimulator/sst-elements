// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_BASIC_ROCC
#define _H_VANADIS_BASIC_ROCC

#include <sst/core/output.h>
#include <sst/core/subcomponent.h>

#include "rocc/vroccinterface.h"
#include "inst/vrocc.h"

#include <cinttypes>
#include <cstdint>
#include <vector>
#include <queue>
#include <limits>

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisRoCCBasic : public SST::Vanadis::VanadisRoCCInterface {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(VanadisRoCCBasic, "vanadis", "VanadisRoCCBasic",
                                          SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                          "Implements a basic RoCC accelerator",
                                          SST::Vanadis::VanadisRoCCInterface)

    SST_ELI_DOCUMENT_PARAMS(
            { "verbose", "Set the verbosity of output for the RoCC", "0" }, 
            { "max_instructions", "Set the maximum number of RoCC instructions permitted in the queue", "8" },
            {"clock",              "Clock frequency for component TimeConverter. MMIOTile is Unclocked but subcomponents use the TimeConverter", "1Ghz"}
        )

    SST_ELI_DOCUMENT_STATISTICS({ "roccs_issued", "Count number of rocc instructions that are issued", "operations", 1 })

    VanadisRoCCBasic(ComponentId_t id, Params& params) : VanadisRoCCInterface(id, params),
        max_instructions(params.find<size_t>("max_instructions", 8)) {
        
        stat_roccs_issued = registerStatistic<uint64_t>("roccs_issued", "1");

        UnitAlgebra clock = params.find<UnitAlgebra>("clock", "1GHz");

        busy = 0;
    }

    virtual ~VanadisRoCCBasic() {
        for(auto cmd_q_itr = cmd_q.begin(); cmd_q_itr != cmd_q.end(); ) {
            delete (*cmd_q_itr);
            cmd_q_itr = cmd_q.erase(cmd_q_itr);
        }
    }

    bool RoCCFull() override { return cmd_q.size() >= max_instructions; }

    size_t roccQueueSize() override { return cmd_q.size(); }

    void push(VanadisRoCCInstruction* rocc_me) override {
        stat_roccs_issued->addData(1);
        cmd_q.push_back( rocc_me );
    }

    // initialize subcomponents and parameterizable data structures
    void init(unsigned int phase) override {}

    void tick(uint64_t cycle) override {
        output->verbose(CALL_INFO, 16, 0, "-> tick RoCC at cycle %" PRIu64 "\n", cycle);
        if(0 == cmd_q.size()) {
            output->verbose(CALL_INFO, 16, 0, "--> nothing to do in RoCC\n");
            return;
        }
        output->verbose(CALL_INFO, 16, 0, "busy? %d\n", busy);

        if (!busy) { // are we already processing something? If not:
            busy = true; // start processing something
            curr_inst = cmd_q.front(); // grab the command to process
            VanadisRegisterFile* hw_thr_reg = registerFiles->at(curr_inst->getHWThread()); // grab that instruction's register file
            uint64_t rs1 = -1; // initialize variables to hold the src reg values 
            uint64_t rs2 = -1;
            curr_inst->getRegisterValues(output, hw_thr_reg, &rs1, &rs2); // grab the src reg values
            curr_roccCmd = new RoCCCommand(curr_inst, rs1, rs2); // build the RoCC command with the instruction and reg src values

            output->verbose(CALL_INFO, 16, 0, "decoding func7 of RoCC inst\n");
            switch (curr_inst->func7) { 
                case 0x0:
                {
                    output->verbose(CALL_INFO, 16, 0, "performing RoCC ADD\n");
                    performADD();
                } break;
                case 0x1:
                {
                    output->verbose(CALL_INFO, 16, 0, "performing RoCC SRAI\n");
                    performSRAI();
                } break;
                default: 
                {
                    output->verbose(CALL_INFO, 16, 0, "ERROR: unrecognized RoCC func7\n");
                } break;
            }
        }
    }

    // adds rs1 and rs2 and writes it to rd
    void performADD() {
        uint64_t src_1 = registerFiles->at(curr_inst->getHWThread())->getIntReg<int64_t>(curr_inst->getPhysIntRegIn(0));
        uint64_t src_2 = registerFiles->at(curr_inst->getHWThread())->getIntReg<int64_t>(curr_inst->getPhysIntRegIn(1));
        uint64_t result = src_1 + src_2;
        output->verbose(CALL_INFO, 9, 0, "EXECUTE RoCC ADD w/ rs1: %llx, rs2: %llx, result: %llx + %llx", src_1, src_2, result);
        registerFiles->at(curr_inst->getHWThread())->setIntReg<uint64_t>(curr_inst->getPhysIntRegOut(0), result);
        completeRoCC(new RoCCResponse(curr_inst->getPhysIntRegOut(0), result));
        return;
    }

    // writes value of rs1 into rs2
    void performSRAI() {
        uint64_t src_1 = registerFiles->at(curr_inst->getHWThread())->getIntReg<int64_t>(curr_inst->getPhysIntRegIn(0));
        uint64_t shamt = curr_inst->getPhysIntRegIn(1);
        uint64_t result = src_1 >> shamt;
        output->verbose(CALL_INFO, 9, 0, "EXECUTE RoCC SRAI w/ rs1: %llx, shamt: %llx, result: %llx + %llx", src_1, shamt, result);
        registerFiles->at(curr_inst->getHWThread())->setIntReg<uint64_t>(curr_inst->getPhysIntRegOut(0), result);
        completeRoCC(new RoCCResponse(curr_inst->getPhysIntRegOut(0), result));
        return;
    }

    // finalizes the execution of a RoCC instruction by:
    // writing any status data to rd
    // mark the instruction executed so main CPU pipeline knows it is finished
    // pop it out of the command queue so it will no longer be processed 
    // turn off busy bit so host knows this accelerator is no longer busy
    void completeRoCC(RoCCResponse* roccRsp) {
        registerFiles->at(curr_inst->getHWThread())->setIntReg<int64_t>(roccRsp->rd, roccRsp->rd_val); 

        output->verbose(CALL_INFO, 16, 0, "Finalize RoCC command w/ rd %" PRIu16 ", rd_val %" PRIu64 " \n", roccRsp->rd, roccRsp->rd_val);
        curr_inst->markExecuted();
        cmd_q.pop_front();
        busy = false;
    }

    std::deque<VanadisRoCCInstruction*> cmd_q; // queue of RoCC commands issued from CPU
    bool busy; // busy line that keeps accelerator from doing more than one thing at a time
    VanadisRoCCInstruction* curr_inst;
    RoCCCommand* curr_roccCmd;

    int max_instructions;

    Statistic<uint64_t>* stat_roccs_issued;
};

} // namespace Vanadis
} // namespace SST

#endif
