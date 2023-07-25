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
#include <sst/core/interfaces/stdMem.h>

#include "rocc/vroccinterface.h"
#include "inst/vrocc.h"

#include <cinttypes>
#include <cstdint>
#include <vector>
#include <queue>
#include <limits>

#define VANADIS_RISCV_FUNC7_MASK  0xFE000000

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisRoCCBasic : public SST::Vanadis::VanadisRoCCInterface {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(VanadisRoCCBasic, "vanadis", "VanadisRoCCBasic",
                                          SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                          "Implements a RoCC accelerator interface for the analog core",
                                          SST::Vanadis::VanadisRoCCInterface)

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS({ "memory_interface", "Set the interface to memory", "SST::Interfaces::StandardMem" })

    SST_ELI_DOCUMENT_PORTS({ "dcache_link", "Connects the RoCC frontend to the data cache", {} })

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

        std_mem_handlers = new VanadisRoCCBasic::StandardMemHandlers(this, output);

        busy = 0;

        memInterface = loadUserSubComponent<Interfaces::StandardMem>(
            "memory_interface", ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, getTimeConverter("1ps"),
            new StandardMem::Handler<SST::Vanadis::VanadisRoCCBasic>(
                this, &VanadisRoCCBasic::processIncomingDataCacheEvent));

    }

    virtual ~VanadisRoCCBasic() {
        for(auto analogCmd_q_itr = analogCmd_q.begin(); analogCmd_q_itr != analogCmd_q.end(); ) {
            delete (*analogCmd_q_itr);
            analogCmd_q_itr = analogCmd_q.erase(analogCmd_q_itr);
        }

        delete std_mem_handlers;
    }

    bool RoCCFull() override { return analogCmd_q.size() >= max_instructions; }

    size_t roccQueueSize() override { return analogCmd_q.size(); }

    void push(VanadisRoCCInstruction* rocc_me) override {
        stat_roccs_issued->addData(1);
        analogCmd_q.push_back( rocc_me );
    }

    // initialize subcomponents and parameterizable data structures
    void init(unsigned int phase) override { 
        memInterface->init(phase); 
    }

    void tick(uint64_t cycle) override {
        output->verbose(CALL_INFO, 16, 0, "-> tick RoCC at cycle %" PRIu64 "\n", cycle);
        if(0 == analogCmd_q.size()) {
            output->verbose(CALL_INFO, 16, 0, "--> nothing to do in RoCC\n");
            return;
        }
        output->verbose(CALL_INFO, 16, 0, "busy? %d\n", busy);

        if (!busy) { // are we already processing something? If not:
            busy = true; // start processing something
            curr_inst = analogCmd_q.front(); // grab the command to process
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
                case 0x1: // basic load
                {
                    output->verbose(CALL_INFO, 16, 0, "issuing load\n");
                    issueLoad();
                } break;
                case 0x2: // basic store
                {
                    output->verbose(CALL_INFO, 16, 0, "issuing store\n");
                    issueStore();
                } break;
                default: 
                {
                    output->verbose(CALL_INFO, 16, 0, "ERROR: unrecognized RoCC func7\n");
                } break;
            }
        }
    }

    // writes value of rs1 into rs2
    void performADD() {
        uint64_t src_1 = registerFiles->at(curr_inst->getHWThread())->getIntReg<int64_t>(curr_inst->getPhysIntRegIn(0));
        uint64_t src_2 = registerFiles->at(curr_inst->getHWThread())->getIntReg<int64_t>(curr_inst->getPhysIntRegIn(1));
        output->verbose(CALL_INFO, 9, 0, "EXECUTE ADD w/ rs1: %llx, rs2: %llx, result: %llx + %llx", src_1, src_2, src_1 + src_2);
        registerFiles->at(curr_inst->getHWThread())->setIntReg<uint64_t>(curr_inst->getPhysIntRegOut(0), src_1 + src_2);
        completeRoCC(new RoCCResponse(curr_inst->getPhysIntRegOut(0), 4));
        return;
    }

    // just a generic load request based on instruction's register values
    void issueLoad() {
        StandardMem::Request* load_req = nullptr;

        load_req = new StandardMem::Read(curr_roccCmd->rs1, 4, 0, 
                            curr_roccCmd->rs1, curr_roccCmd->inst->getInstructionAddress(), curr_roccCmd->inst->getHWThread());
        output->verbose(CALL_INFO, 9, 0, "----> Read Req: physAddr: %llx, size: %llx, vAddr: %llx, inst ptr: %llx, tid: %llx\n", 
                        curr_roccCmd->rs1, 4, curr_roccCmd->rs1, curr_roccCmd->inst->getInstructionAddress(), curr_roccCmd->inst->getHWThread());
        // if the instruction does not trap an error we will continue to process it
        if(LIKELY(! curr_roccCmd->inst->trapsError())) {
            output->verbose(CALL_INFO, 16, VANADIS_DBG_LSQ_LOAD_FLG, "-----> ins: 0x%llx / thr: %" PRIu32 " processed and requests sent to memory system.\n",
                curr_roccCmd->inst->getInstructionAddress(), curr_roccCmd->inst->getHWThread());

            assert(load_req != nullptr);

            memInterface->send(load_req);
        }
    }

    // just a generic store request based on instruction's register values
    void issueStore() {
        StandardMem::Request* store_req = nullptr;
        std::vector<uint8_t> payload(4);
        
        store_req = new StandardMem::Write(curr_roccCmd->rs1, 4, payload, 
            false, 0, curr_roccCmd->rs1, curr_roccCmd->inst->getInstructionAddress(), curr_roccCmd->inst->getHWThread());
        memInterface->send(store_req);

        return;
    }

    // finalizes the execution of a RoCC instruction by:
    // writing any status data to rd
    // mark the instruction executed so main CPU pipeline knows it is finished
    // pop it out of the command queue so it will no longer be processed 
    // reset state of this analog RoCC frontend in case all-in-one just finished
    // turn off busy bit so host knows this accelerator is no longer busy
    void completeRoCC(RoCCResponse* roccRsp) {
        registerFiles->at(curr_inst->getHWThread())->setIntReg<int64_t>(roccRsp->rd, roccRsp->rd_val); 

        output->verbose(CALL_INFO, 16, 0, "Finalize RoCC command w/ rd %" PRIu16 ", rd_val %" PRIu64 " \n", roccRsp->rd, roccRsp->rd_val);
        curr_inst->markExecuted();
        analogCmd_q.pop_front();
        busy = false;
    }

    class StandardMemHandlers : public Interfaces::StandardMem::RequestHandler {
    public:
        friend class VanadisRoCCBasic;

        StandardMemHandlers(VanadisRoCCBasic* rocc, SST::Output* output) :
                Interfaces::StandardMem::RequestHandler(output), rocc(rocc) {output = output;}
        
        virtual ~StandardMemHandlers() {}

        virtual void handle(StandardMem::ReadResp* ev) {
            //std::cout << "flags: " << ev->getAllFlags() << std::endl; // flags identify origin of read request
            out->verbose(CALL_INFO, 16, 0, "-> handle read-response (virt-addr: 0x%llx)\n", ev->vAddr);
            VanadisRoCCInstruction* load_ins = rocc->analogCmd_q.front(); // need to grab the instrution that generated the read request
            // so that we know where to store the read response results

            const uint32_t hw_thr = load_ins->getHWThread(); // hardware thread gives us register file info

            if ( ev->getFail() ) { 
                out->verbose(CALL_INFO, 16, 0, "RoCC load failed\n");
                load_ins->flagError();
            }
            
            uint16_t target_isa_reg = load_ins->getISAIntRegOut(0); // get architectural output register 
            uint16_t target_reg = load_ins->getPhysIntRegOut(0); // get physical output register
            uint64_t reg_offset  = 0;
            uint64_t addr_offset = 0;

            uint32_t reg_width = rocc->registerFiles->at(hw_thr)->getIntRegWidth(); // get size of output register
            std::vector<uint8_t> register_value(reg_width); // initialize placeholder for read response data this way because it comes in 8bit (I think?) chunks
            // copy entire register here
            rocc->registerFiles->at(hw_thr)->copyFromIntRegister(target_reg, 0, &register_value[0], reg_width);
            std::cout << "size of read response: " << ev->size << std::endl;
            std::cout << "size of target register: " << reg_width << std::endl;
            for (auto i = 0; i < reg_width; ++i) { // fill data placeholder with read response data
                register_value.at(reg_offset + addr_offset + i) = ev->data[i];
            }

            switch (ev->getAllFlags()) { // treat read response data differently based on who issued it (flags)
                default:
                {
                    rocc->output->verbose(CALL_INFO, 16, 0, "ERROR: unrecognized read response flag\n");
                } break;
            }

            //rocc->registerFiles->at(hw_thr)->print(out);
            rocc->registerFiles->at(hw_thr)->copyToIntRegister(target_reg, 0, &register_value[0], register_value.size()); // write the read response data into the destination register of the instruciton
            //rocc->registerFiles->at(hw_thr)->print(out);
            if (rocc->curr_inst->func7 == 0x1) { // if this is just an individual load command:
                rocc->completeRoCC(new RoCCResponse(rocc->curr_inst->getPhysIntRegOut(0), 1)); // complete the load command
            }
        } 
        
        virtual void handle(StandardMem::WriteResp* ev) {
            // write is much simpler because we aren't handling any reponse data
            // just need to make sure it went through properly
            out->verbose(CALL_INFO, 16, 0, "-> handle write-response (virt-addr: 0x%llx)\n", ev->vAddr);
            if ( ev->getFail() ) { 
                out->verbose(CALL_INFO, 16, 0, "RoCC store failed\n");
                rocc->analogCmd_q.front()->flagError();
            }
            
            delete ev;
            rocc->completeRoCC(new RoCCResponse(rocc->curr_inst->getPhysIntRegOut(0), 1));
        }
    
        VanadisRoCCBasic* rocc;
        SST::Output* output;
    };

    void processIncomingDataCacheEvent(StandardMem::Request* ev) {
        output->verbose(CALL_INFO, 16, 0, "received incoming data cache request -> processIncomingDataCacheEvent()\n");

        assert(ev != nullptr);
        assert(std_mem_handlers != nullptr);

        ev->handle(std_mem_handlers);
        output->verbose(CALL_INFO, 16, 0, "completed pass off to incoming handlers\n");
    }

    // takes the vector of 8bit chunks and converts it into single uint64_t value
    inline uint64_t dataToInt(std::vector<uint8_t>* data) {
        uint64_t retval = 0;
        assert (data->size() <= 8);

        for (int i = data->size(); i > 0; i--) {
            retval <<= 8;
            retval |= (*data)[i-1];
        }
        return retval;
    }

    // grabs the func7 field of RoCC instruction
    uint32_t extract_func7(const uint32_t ins) const { return ((ins & VANADIS_RISCV_FUNC7_MASK) >> 25); }

    std::deque<VanadisRoCCInstruction*> analogCmd_q; // queue of RoCC commands issued from CPU
    bool busy; // busy line from analog accelerator that keeps this interface from sending more commands
    VanadisRoCCInstruction* curr_inst;
    RoCCCommand* curr_roccCmd;

    StandardMemHandlers* std_mem_handlers;
    StandardMem* memInterface;

    int max_instructions;

    Statistic<uint64_t>* stat_roccs_issued;
};

} // namespace Vanadis
} // namespace SST

#endif