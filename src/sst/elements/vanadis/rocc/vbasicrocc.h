// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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

#include <cassert>
#include <cstddef>
#include <deque>
#include <sst/core/output.h>
#include <sst/core/subcomponent.h>

#include "rocc/vroccinterface.h"

#include <cinttypes>
#include <cstdint>
#include <limits>
#include <queue>
#include <vector>

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisRoCCBasic : public SST::Vanadis::VanadisRoCCInterface {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(VanadisRoCCBasic, "vanadis", "VanadisRoCCBasic",
                                          SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                          "Implements a basic example of a RoCC accelerator",
                                          SST::Vanadis::VanadisRoCCInterface)

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS({ "memory_interface", "Set the interface to memory",
                                            "SST::Interfaces::StandardMem" })

    SST_ELI_DOCUMENT_PARAMS(
            { "max_instructions", "Set the maximum number of RoCC instructions permitted in the queue", "8" },
            { "clock",            "Clock frequency for component TimeConverter. MMIOTile is Unclocked but subcomponents use the TimeConverter", "1Ghz"}
        )

    VanadisRoCCBasic(ComponentId_t id, Params& params) : VanadisRoCCInterface(id, params),
        max_instructions(params.find<size_t>("max_instructions", 8)) {

        try {
            UnitAlgebra clock = params.find<UnitAlgebra>("clock", "1GHz");

            if (!(clock.hasUnits("Hz") || clock.hasUnits("s")) ||
                clock.getRoundedValue() <= 0) {
                output->fatal(CALL_INFO, -1,
                    "%s, Error - Invalid param: clock.\n"
                    "Must have units of Hz or s and be > 0.\n"
                    "SI prefixes ok. You specified '%s'\n",
                    getName().c_str(), clock.toString().c_str());
            }
        } catch (const UnitAlgebra::UnitAlgebraException& exc) {
            output->fatal(CALL_INFO, -1,
                "%s, Invalid param: Exception while parsing 'clock'.\n"
                "'%s'\n",
                getName().c_str(), exc.what());
        }

        std_mem_handlers = new StandardMemHandlers(this, output);

        busy = false;
        curr_resp = NULL;

        memInterface = loadUserSubComponent<Interfaces::StandardMem>(
            "memory_interface", ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, getTimeConverter("1ps"),
            new StandardMem::Handler2<SST::Vanadis::VanadisRoCCBasic,&VanadisRoCCBasic::processIncomingDataCacheEvent>(this));

        if ( nullptr == memInterface ) {
            output->fatal(
                CALL_INFO, -1,
                "Error: unable to load memory interface subcomponent for VanadisRoCCBasic.\n");
        }

    }

    virtual ~VanadisRoCCBasic() {
        for(auto roccCmd_q_itr = roccCmd_q.begin(); roccCmd_q_itr != roccCmd_q.end(); ) {
            delete (*roccCmd_q_itr);
            roccCmd_q_itr = roccCmd_q.erase(roccCmd_q_itr);
        }
    }

    bool RoCCFull() override { return roccCmd_q.size() >= max_instructions; }

    bool isBusy() override { return busy; }

    size_t roccQueueSize() override { return roccCmd_q.size(); }

    void push(RoCCCommand* rocc_me) override {
        stat_rocc_issued->addData(1);
        roccCmd_q.push_back( rocc_me );
    }

    RoCCResponse* respond() override {
        RoCCResponse* temp = curr_resp;
        curr_resp = NULL;
        return temp;
    }

    // initialize subcomponents and parameterizable data structures
    void init(unsigned int phase) override {
        memInterface->setMemoryMappedAddressRegion(0, 64);
        memInterface->init(phase);
    }

    void tick(uint64_t cycle) override {
        output->verbose(CALL_INFO, 16, 0, "-> tick RoCC at cycle %" PRIu64 "\n", cycle);
        if(0 == roccCmd_q.size()) {
            output->verbose(CALL_INFO, 16, 0, "--> nothing to do in RoCC\n");
            return;
        }
        output->verbose(CALL_INFO, 16, 0, "busy? %d\n", busy);

        if (!busy) { // are we already processing something? If not:
            busy = true; // start processing something
            curr_cmd = roccCmd_q.front(); // grab the command to process

            output->verbose(CALL_INFO, 9, 0, "decoding func7 of RoCC inst\n");
            switch (curr_cmd->inst->func7) {
                case 0x0:
                {
                    output->verbose(CALL_INFO, 9, 0, "performing RoCC ADD\n");
                    performADD();
                } break;
                case 0x1:
                {
                    output->verbose(CALL_INFO, 9, 0, "performing RoCC SRAI\n");
                    performSRAI();
                } break;
                case 0x2: // basic load
                {
                    output->verbose(CALL_INFO, 9, 0, "issuing load\n");
                    issueLoad();
                } break;
                case 0x3: // basic store
                {
                    output->verbose(CALL_INFO, 9, 0, "issuing store\n");
                    issueStore();
                } break;
                default:
                {
                    output->verbose(CALL_INFO, 9, 0, "ERROR: unrecognized RoCC func7\n");
                } break;
            }
        }
    }

    // adds rs1 and rs2 and writes it to rd
    void performADD() {
        uint64_t rs1 = curr_cmd->rs1;
        uint64_t rs2 = curr_cmd->rs2;
        output->verbose(CALL_INFO, 9, 0, "EXECUTE RoCC ADD w/ rs1: %" PRIx64 ", rs2: %" PRIx64 ", result: %" PRIx64 "\n", rs1, rs2, rs1 + rs2);
        completeRoCC(rs1 + rs2);
        return;
    }

    // writes value of rs1 into rs2
    void performSRAI() {
        uint64_t src_1 = curr_cmd->rs1;
        uint64_t shamt = curr_cmd->rs2;
        uint64_t result = src_1 >> shamt;
        output->verbose(CALL_INFO, 9, 0, "EXECUTE RoCC SRAI w/ rs1: %" PRIx64 ", shamt: %" PRIx64 ", result: %" PRIx64 "\n", src_1, shamt, result);
        completeRoCC(result);
        return;
    }

    // just a generic load request based on instruction's register values
    void issueLoad() {
        StandardMem::Request* load_req = nullptr;

        load_req = new StandardMem::Read(curr_cmd->rs1, 4, 0);
        output->verbose(CALL_INFO, 9, 0, "----> Read Req: physAddr: %" PRIx64 ", size: %" PRIx64 ", vAddr: %" PRIx64 ", inst ptr: %" PRIx64 ", tid: %" PRIx64 "\n", curr_cmd->rs1, uint64_t{4}, curr_cmd->rs1, uint64_t{0}, uint64_t{0});

        assert(load_req != nullptr);

        memInterface->send(load_req);
    }

    // just a generic store request based on instruction's register values
    void issueStore() {
        StandardMem::Request* store_req = nullptr;
        std::vector<uint8_t> payload(4);
        for (int i = 0; i < 4; ++i) {
            payload[i] = (rocc_internal_register >> (i * 8)) & 0xFF;
        }

        store_req = new StandardMem::Write(curr_cmd->rs1, 4, payload,
            false, 0, curr_cmd->rs1, 0, 0);
        memInterface->send(store_req);
    }

    // finalizes the execution of a RoCC instruction by:
    // pop it out of the command queue so it will no longer be processed
    // turn off busy bit so host knows this accelerator is no longer busy
    // send RoCC response back to host
    void completeRoCC(uint64_t rd_val) {
        output->verbose(CALL_INFO, 9, 0, "Finalize RoCC command w/ rd %" PRIu16 ", rd_val %" PRIu64 " \n", curr_cmd->inst->rd, rd_val);
        roccCmd_q.pop_front();
        busy = false;
        curr_resp = new RoCCResponse(curr_cmd->inst->rd, rd_val);
    }

    class StandardMemHandlers : public Interfaces::StandardMem::RequestHandler {
    public:
        friend class VanadisRoCCBasic;

        StandardMemHandlers(VanadisRoCCBasic* rocc, SST::Output* output) :
                Interfaces::StandardMem::RequestHandler(output), rocc(rocc) {output = output;}

        virtual ~StandardMemHandlers() {}

        virtual void handle(StandardMem::ReadResp* ev) {
            out->verbose(CALL_INFO, 9, 0, "-> handle read-response (virt-addr: 0x%" PRIx64 ")\n", ev->vAddr);
            RoCCCommand* rocc_cmd = rocc->curr_cmd; // need to grab the instruction that generated the read request
            // so that we know where to store the read response results

            if ( ev->getFail() ) {
                out->verbose(CALL_INFO, 9, 0, "RoCC load failed, sending error code 1\n");
                rocc->completeRoCC(1);
            }

            uint64_t reg_offset  = 0;
            uint64_t addr_offset = 0;
            uint64_t reg_width = 8;

            std::vector<uint8_t> register_value(reg_width); // initialize placeholder for read response data this way because it comes in 8-bit  chunks
            // copy entire register here
            for (auto i = 0; i < reg_width; ++i) { // fill data placeholder with read response data
                register_value.at(reg_offset + addr_offset + i) = ev->data[i];
            }

            switch (ev->getAllFlags()) { // treat read response data differently based on who issued it
            case 0x0:
                {
                    rocc->rocc_internal_register = rocc->dataToInt(&register_value);
                    rocc->output->verbose(CALL_INFO, 9, 0, "RoCC loaded value: %d\n", rocc->rocc_internal_register);
                    rocc->completeRoCC(0); // complete the load command
                } break;

            default:
                {
                    rocc->output->verbose(CALL_INFO, 9, 0, "ERROR: unrecognized read response flag\n");
                    rocc->completeRoCC(1); // complete the load command
                } break;
            }

            delete ev;
        }

        virtual void handle(StandardMem::WriteResp* ev) {
            // write is much simpler because we aren't handling any reponse data
            // just need to make sure it went through properly
            out->verbose(CALL_INFO, 9, 0, "-> handle write-response (virt-addr: 0x%" PRIx64 ")\n", ev->vAddr);
            if ( ev->getFail() ) {
                out->verbose(CALL_INFO, 9, 0, "RoCC store failed, responding with error code 1\n");
                rocc->completeRoCC(1);
            }

            delete ev;
            rocc->completeRoCC(0);
        }

        VanadisRoCCBasic* rocc;
        SST::Output* output;
    };

    void processIncomingDataCacheEvent(StandardMem::Request* ev) {
        output->verbose(CALL_INFO, 9, 0, "received incoming data cache request -> processIncomingDataCacheEvent()\n");

        assert(ev != nullptr);
        assert(std_mem_handlers != nullptr);

        ev->handle(std_mem_handlers);
        output->verbose(CALL_INFO, 9, 0, "completed pass off to incoming handlers\n");
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

    std::deque<RoCCCommand*> roccCmd_q; // queue of RoCC commands issued from CPU
    bool busy; // busy line from RoCC accelerator that keeps this interface from sending more commands
    RoCCCommand* curr_cmd;
    RoCCResponse* curr_resp;

    StandardMemHandlers* std_mem_handlers;
    StandardMem* memInterface;

    int max_instructions;
    int rocc_internal_register;
};

} // namespace Vanadis
} // namespace SST

#endif
