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

#ifndef _H_VANADIS_LSQ_SEQUENTIAL
#define _H_VANADIS_LSQ_SEQUENTIAL

#include <deque>
#include <unordered_map>
#include <unordered_set>

#include "sst/core/interfaces/stdMem.h"

#include "lsq/vlsq.h"
#include "lsq/vmemwriterec.h"

#include "inst/vinst.h"
#include "inst/vstore.h"

namespace SST {
namespace Vanadis {

class VanadisSequentialLoadStoreSCRecord {
public:
    VanadisSequentialLoadStoreSCRecord(VanadisStoreInstruction* instr, StandardMem::Request::id_t req) {

        ins = instr;
        req_id = req;
    }

    StandardMem::Request::id_t getRequestID() const { return req_id; }
    VanadisStoreInstruction* getInstruction() const { return ins; }

protected:
    VanadisStoreInstruction* ins;
    StandardMem::Request::id_t req_id;
};

class VanadisSequentialLoadStoreRecord {
public:
    VanadisSequentialLoadStoreRecord(VanadisInstruction* instr) {
        ins = instr;
        issued = false;
        req_id = 0;
        opAddr = 0;
    }

    bool isOperationIssued() { return issued; }

    void markOperationIssued() { issued = true; }

    VanadisInstruction* getInstruction() { return ins; }

    StandardMem::Request::id_t getRequestID() { return req_id; }

    void setRequestID(StandardMem::Request::id_t new_id) { req_id = new_id; }

    bool isStore() { return ins->getInstFuncType() == INST_STORE; }

    bool isLoad() { return ins->getInstFuncType() == INST_LOAD; }

    uint64_t getOperationAddress() const { return opAddr; }

    void setOperationAddress(const uint64_t newAddr) { opAddr = newAddr; }

    void print(SST::Output* output) {
        output->verbose(CALL_INFO, 16, 0, "-> type: %s / ins: 0x%llx / issued: %c\n", isStore() ? "STORE" : "LOAD ",
                        ins->getInstructionAddress(), issued ? 'Y' : 'N');
    }

protected:
    StandardMem::Request::id_t req_id;
    VanadisInstruction* ins;
    bool issued;
    uint64_t opAddr;
};

class VanadisSequentialLoadStoreQueue : public SST::Vanadis::VanadisLoadStoreQueue {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(VanadisSequentialLoadStoreQueue, "vanadis", "VanadisSequentialLoadStoreQueue",
                                          SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                          "Implements a sequential load-store for use with the SST memInterface",
                                          SST::Vanadis::VanadisLoadStoreQueue)

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS({ "memory_interface", "Set the interface to memory",
                                          "SST::Interfaces::StandardMem" })

    SST_ELI_DOCUMENT_PORTS({ "dcache_link", "Connects the LSQ to the data cache", {} })

    SST_ELI_DOCUMENT_PARAMS({ "load_store_entries", "Set the number of load/stores that can be pending", "8" },
                            { "check_memory_loads",
                              "Check memory loads come from memory locations that have been written", "0" }, )

    VanadisSequentialLoadStoreQueue(ComponentId_t id, Params& params) : VanadisLoadStoreQueue(id, params) {

        memInterface = loadUserSubComponent<Interfaces::StandardMem>(
            "memory_interface", ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, getTimeConverter("1ps"),
            new StandardMem::Handler<SST::Vanadis::VanadisSequentialLoadStoreQueue>(
                this, &VanadisSequentialLoadStoreQueue::processIncomingDataCacheEvent));

        if (nullptr == memInterface) {
            output->fatal(CALL_INFO, -1, "Error - unable to load \"memory_interface\"\n");
        }

        max_q_size = params.find<size_t>("load_store_entries", 8);

        output->verbose(CALL_INFO, 2, 0, "LSQ Load/Store Queue entry count:     %" PRIu32 "\n", (uint32_t)max_q_size);

        std::string trace_file_path = params.find<std::string>("address_trace", "");

        if ("" != trace_file_path) {
            address_trace_file = fopen(trace_file_path.c_str(), "wt");
        } else {
            address_trace_file = nullptr;
        }

        allow_speculated_operations = params.find<bool>("allow_speculated_operations", true);
        fault_on_memory_not_written = params.find<bool>("check_memory_loads", false);

        // Check for up to 4GB
        if (fault_on_memory_not_written) {
            uint64_t mem_gb = 4;
            uint64_t mem_bytes = mem_gb * 1024 * 1024 * 1024;
            flag_non_written_loads_count = params.find<uint64_t>("fault_non_written_loads_after", 0);

            memory_check_table = new VanadisMemoryWrittenRecord(16, mem_bytes);
        } else {
            memory_check_table = nullptr;
        }
        std_mem_handlers = new StandardMemHandlers(this, output);
    }

    virtual ~VanadisSequentialLoadStoreQueue() {
        if (address_trace_file != nullptr) {
            fclose(address_trace_file);
        }

        delete memInterface;
        delete memory_check_table;
        delete std_mem_handlers;
    }

    virtual bool storeFull() { return op_q.size() >= max_q_size; }

    virtual bool loadFull() { return op_q.size() >= max_q_size; }

    virtual bool storeEmpty() { return op_q.size() == 0; }

    virtual bool loadEmpty() { return op_q.size() == 0; }

    virtual size_t storeSize() { return op_q.size(); }

    virtual size_t loadSize() { return op_q.size(); }

    virtual void printStatus(SST::Output& output) {
        int next_index = 0;

        for (auto q_itr = op_q.begin(); q_itr != op_q.end(); q_itr++) {
            if ((*q_itr)->isLoad()) {
                output.verbose(CALL_INFO, 0, 0,
                               "---> lsq[%5d]: ins: 0x%llx / LOAD  / 0x%llx / "
                               "rob-issued: %c / lsq-issued: %c\n",
                               next_index++, (*q_itr)->getInstruction()->getInstructionAddress(),
                               (*q_itr)->getOperationAddress(),
                               (*q_itr)->getInstruction()->completedIssue() ? 'y' : 'n',
                               (*q_itr)->isOperationIssued() ? 'y' : 'n');
            } else {
                output.verbose(CALL_INFO, 0, 0,
                               "---> lsq[%5d]: ins: 0x%llx / STORE / rob-issued: %c / "
                               "lsq-issued: %c\n",
                               next_index++, (*q_itr)->getInstruction()->getInstructionAddress(),
                               (*q_itr)->getInstruction()->completedIssue() ? 'y' : 'n',
                               (*q_itr)->isOperationIssued() ? 'y' : 'n');
            }
        }
    }

    virtual void push(VanadisStoreInstruction* store_me) {
        if (op_q.size() < max_q_size) {
            output->verbose(CALL_INFO, 8, 0, "enqueue store ins-addr: 0x%llx\n", store_me->getInstructionAddress());
            op_q.push_back(new VanadisSequentialLoadStoreRecord(store_me));
        } else {
            output->fatal(CALL_INFO, -1,
                          "Error - attempted to enqueue but no room (max: %" PRIu32 ", size: %" PRIu32 ")\n",
                          (uint32_t)max_q_size, (uint32_t)op_q.size());
        }
    }

    virtual void push(VanadisLoadInstruction* load_me) {
        if (op_q.size() < max_q_size) {
            output->verbose(CALL_INFO, 8, 0, "enqueue load ins-addr: 0x%llx\n", load_me->getInstructionAddress());
            op_q.push_back(new VanadisSequentialLoadStoreRecord(load_me));
        } else {
            output->fatal(CALL_INFO, -1,
                          "Error - attempted to enqueue but no room (max: %" PRIu32 ", size: %" PRIu32 ")\n",
                          (uint32_t)max_q_size, (uint32_t)op_q.size());
        }
    }

    void printLSQ() {
        output->verbose(CALL_INFO, 16, 0, "-- LSQ Seq / Size: %" PRIu64 " ----------------\n", (uint64_t)op_q.size());

        for (auto op_q_itr = op_q.begin(); op_q_itr != op_q.end(); op_q_itr++) {
            (*op_q_itr)->print(output);
        }
    }

    virtual void tick(uint64_t cycle) {
        output->verbose(CALL_INFO, 16, 0, "ticking load/store queue at cycle %" PRIu64 " lsq size: %" PRIu64 "\n",
                        (uint64_t)cycle, (uint64_t)op_q.size());

        if (output->getVerboseLevel() >= 16) {
            printLSQ();
        }

        if (sc_inflight.size() > 0) {
            output->verbose(CALL_INFO, 16, 0,
                            "-> LLSC_STORE operation in flight, stalling this cycle "
                            "until returns\n");
        }

        VanadisSequentialLoadStoreRecord* next_item = nullptr;
        auto op_q_itr = op_q.begin();

        for (; op_q_itr != op_q.end(); op_q_itr++) {
            if (!(*op_q_itr)->isOperationIssued()) {
                next_item = (*op_q_itr);
                break;
            }
        }

        if (nullptr != next_item) {

            VanadisRegisterFile* reg_file = registerFiles->at(next_item->getInstruction()->getHWThread());

            if (next_item->isLoad()) {
                VanadisLoadInstruction* load_ins = dynamic_cast<VanadisLoadInstruction*>(next_item->getInstruction());

                // we are either allowed to issue the load because speculated loads are
                // OK or we don't permit speculated loads but now we are at the front of
                // the ROB
                if (allow_speculated_operations || load_ins->checkFrontOfROB()) {
                    uint64_t load_addr = 0;
                    uint16_t load_width = 0;
                    std::string load_type = "";

                    load_ins->computeLoadAddress(output, reg_file, &load_addr, &load_width);

                    output->verbose(CALL_INFO, 8, 0, "--> issue load for 0x%llx width: %" PRIu16 " bytes.\n", load_addr,
                                    load_width);
                    load_addr = load_addr & address_mask;

                    StandardMem::Request* load_req;

                    switch (load_ins->getTransactionType()) {
                    case MEM_TRANSACTION_LLSC_LOAD: {
                        load_req = new StandardMem::LoadLink(load_addr, load_width, 0, load_addr,
                            load_ins->getInstructionAddress());
                        load_type = "LLSC";
                        output->verbose(CALL_INFO, 16, 0, "----> generated as LoadLink\n");
                    } break;
                    case MEM_TRANSACTION_LOCK: {
                        load_req = new StandardMem::ReadLock(load_addr, load_width, 0, load_addr,
                            load_ins->getInstructionAddress());
                        load_type = "LOCK";
                        output->verbose(CALL_INFO, 16, 0, "----> generated as ReadLock\n");
                    } break;
                    case MEM_TRANSACTION_NONE: {
                        load_req = new StandardMem::Read(load_addr, load_width, 0, load_addr,
                            load_ins->getInstructionAddress());
                    } break;
                    case MEM_TRANSACTION_LLSC_STORE: {
                        output->fatal(CALL_INFO, -1,
                                      "Error - found a LLSC_STORE transaction while "
                                      "processing a LOAD\n");
                    }
                    }

                    if (load_addr < 4096) {
                        output->verbose(CALL_INFO, 16, 0,
                                        "[fault] address for load 0x%llx is less than 4096, indicates "
                                        "segmentation-fault, mark load error (load-ins: 0x%llx)\n",
                                        load_addr, load_ins->getInstructionAddress());
                        load_ins->flagError();
                    } else {
                        if ((!load_ins->isPartialLoad()) && ((load_addr % load_width) > 0)) {
                            output->verbose(CALL_INFO, 16, 0,
                                            "[fault] load is not partial and 0x%llx is not "
                                            "aligned to load width (%" PRIu16 ")\n",
                                            load_addr, load_width);
                            load_ins->flagError();
                        } else {
                            writeTrace(load_ins, "LOAD", load_type, load_addr, load_width);

                            memInterface->send(load_req);
                            stat_load_issued->addData(1);
                            stat_data_bytes_read->addData(load_width);

                            next_item->setRequestID(load_req->getID());
                            next_item->setOperationAddress(load_addr);
                        }
                    }

                    next_item->markOperationIssued();
                }
            } else if (next_item->isStore()) {
                VanadisStoreInstruction* store_ins
                    = dynamic_cast<VanadisStoreInstruction*>(next_item->getInstruction());

                // Stores must be at the front of the ROB to be executed or else we will
                // potentially violate correct execution in OoO pipelines
                if (store_ins->checkFrontOfROB()) {
                    uint64_t store_addr = 0;
                    uint16_t store_width = 0;
                    uint16_t value_reg = 0; // store_ins->getPhysIntRegIn(1); // reg-0 =
                                            // addr, reg-1 = value
                    uint16_t reg_offset = store_ins->getRegisterOffset();
                    std::string store_type = "";

                    std::vector<uint8_t> payload;
                    char* reg_ptr = nullptr;

                    switch (store_ins->getValueRegisterType()) {
                    case STORE_INT_REGISTER: {
                        output->verbose(CALL_INFO, 16, 0, "--> store is integer register\n");
                        value_reg = store_ins->getPhysIntRegIn(1);
                        reg_ptr = reg_file->getIntReg(value_reg);
                    } break;
                    case STORE_FP_REGISTER: {
                        output->verbose(CALL_INFO, 16, 0, "--> store is floating-point register\n");
                        value_reg = store_ins->getPhysFPRegIn(0);
                        reg_ptr = reg_file->getFPReg(value_reg);
                    } break;
                    }

                    store_ins->computeStoreAddress(output, reg_file, &store_addr, &store_width);
                    store_addr = store_addr & address_mask;

                    for (uint16_t i = 0; i < store_width; ++i) {
                        payload.push_back(reg_ptr[reg_offset + i]);
                    }

                    output->verbose(CALL_INFO, 8, 0,
                                    "--> issue store at 0x%llx width: %" PRIu16 " bytes, value-reg: %" PRIu16
                                    " / partial: %s / offset: %" PRIu16 "\n",
                                    store_addr, store_width, value_reg, store_ins->isPartialStore() ? "yes" : "no",
                                    reg_offset);

                    if (store_addr < 4096) {
                        output->verbose(CALL_INFO, 16, 0,
                                        "[fault] - address 0x%llx is less than 4096, indicates a "
                                        "segmentation fault (store-ins: 0x%llx)\n",
                                        store_addr, store_ins->getInstructionAddress());
                        store_ins->flagError();
                        store_ins->markExecuted();
                    } else {
                        StandardMem::Request* store_req;

                        switch (store_ins->getTransactionType()) {
                        case MEM_TRANSACTION_LLSC_STORE: {
                            store_req = new StandardMem::StoreConditional(store_addr, store_width, payload, 0,
                                store_addr, store_ins->getInstructionAddress());
                            sc_inflight.insert(new VanadisSequentialLoadStoreSCRecord(store_ins, store_req->getID()));
                            store_type = "LLSC";
                            output->verbose(CALL_INFO, 16, 0, "----> generated as StoreConditional\n");
                        } break;
                        case MEM_TRANSACTION_LOCK: {
                            store_req = new StandardMem::WriteUnlock(store_addr, store_width, payload, 0,
                                store_addr, store_ins->getInstructionAddress());
                            store_type = "LOCK";
                            output->verbose(CALL_INFO, 16, 0, "----> generated as WriteUnlock\n");
                            store_ins->markExecuted();
                        } break;
                        case MEM_TRANSACTION_NONE: {
                            store_req = new StandardMem::Write(store_addr, store_width, payload, false, 0,
                                store_addr, store_ins->getInstructionAddress());
                            store_ins->markExecuted();
                        } break;
                        case MEM_TRANSACTION_LLSC_LOAD: {
                            output->fatal(CALL_INFO, -1,
                                          "Executing an LLSC LOAD operation for a STORE "
                                          "instruction - logical error.\n");
                        } break;
                        }

                        writeTrace(store_ins, "STORE", store_type, store_addr, store_width);
                        memInterface->send(store_req);

                        stat_store_issued->addData(1);
                        stat_data_bytes_written->addData(store_width);

                        if (fault_on_memory_not_written) {
                            for (uint64_t i = store_addr; i < (store_addr + store_width); ++i) {
                                memory_check_table->markByte(i);
                            }
                        }
                    }

                    op_q.erase(op_q_itr);
                }
            } else {
                output->fatal(CALL_INFO, 8, 0, "Unknown type of item in LSQ, neither load nor store?\n");
            }
        }
    }

    class StandardMemHandlers : public StandardMem::RequestHandler {
    public:
        friend class VanadisSequentialLoadStoreQueue;

        StandardMemHandlers(VanadisSequentialLoadStoreQueue* lsq, SST::Output* output) :
            StandardMem::RequestHandler(output), lsq(lsq) {}

        virtual ~StandardMemHandlers() {}

        virtual void handle(StandardMem::ReadResp* ev) override {
            out->verbose(CALL_INFO, 16, 0, "recv incoming d-cache event, addr: 0x%llx, size: %" PRIu32 "\n", ev->vAddr,
                            (uint32_t)ev->data.size());

            bool processed = false;
            for (auto op_q_itr = lsq->op_q.begin(); op_q_itr != lsq->op_q.end();) {
                // is the operation issued to the memory system and do we match ID
                if ((*op_q_itr)->isOperationIssued() && (ev->getID() == (*op_q_itr)->getRequestID())) {

                    out->verbose(CALL_INFO, 8, 0, "matched a load record (addr: 0x%llx)\n",
                                    (*op_q_itr)->getInstruction()->getInstructionAddress());

                    if (out->getVerboseLevel() >= 16) {
                        char* payload_print = new char[256];
								char* payload_print_inner = new char[256];

								payload_print[0] = '\0';
								payload_print_inner[0] = '\0';

								for( int s = 0; s < std::min((int)ev->data.size(), 8); ++s) {
									std::strncpy(payload_print_inner, payload_print, 256);
	                        snprintf(payload_print, 256, "%s 0x%02x", payload_print_inner, ev->data[s]);
								}

                        out->verbose(CALL_INFO, 16, 0, "-> payload (first %d bytes): %s\n",
									std::min((int)ev->data.size(), 8), payload_print);

                        delete[] payload_print;
								delete[] payload_print_inner;
                    }

                    if ((*op_q_itr)->isLoad()) { // TODO it better be since this is a ReadResp
                        VanadisLoadInstruction* load_ins
                            = dynamic_cast<VanadisLoadInstruction*>((*op_q_itr)->getInstruction());
                        VanadisRegisterFile* reg_file = lsq->registerFiles->at(load_ins->getHWThread());

                        uint64_t load_addr = 0;
                        uint16_t load_width = 0;
                        uint16_t target_reg = 0; // load_ins->getPhysIntRegOut(0);
                        uint16_t target_isa = 0;
                        uint16_t reg_offset = 0;

                        if (lsq->fault_on_memory_not_written) {
                            uint64_t load_unwritten_address = 0;

                            for (uint64_t i = ev->vAddr; i < (ev->vAddr + ev->size); ++i) {
                                // if the address is not marked as written, potential error
                                // condition
                                if (!lsq->memory_check_table->isMarked(i)) {
                                    load_unwritten_address = i;
                                    break;
                                }
                            }

                            // We have an unwritten value from memory, so flag an error
                            if (load_unwritten_address > 0) {
                                if (lsq->flag_non_written_loads_count > 0) {
                                    lsq->flag_non_written_loads_count--;
                                } else {
                                    out->verbose(CALL_INFO, 8, 0,
                                                "--> [load-unwritten-memory-fault]: load-ins: 0x%llx -> "
                                                "memory is not written at addr 0x%llx\n",
                                                load_ins->getInstructionAddress(), load_unwritten_address);
                                    load_ins->flagError();
                                }
                            }
                        }

                        switch (load_ins->getValueRegisterType()) {
                        case LOAD_INT_REGISTER: {
                            out->verbose(CALL_INFO, 8, 0, "--> load is integer register\n");
                            target_reg = load_ins->getPhysIntRegOut(0);
                            target_isa = load_ins->getISAIntRegOut(0);
                        } break;
                        case LOAD_FP_REGISTER: {
                            out->verbose(CALL_INFO, 8, 0, "--> load is floating point register\n");
                            target_reg = load_ins->getPhysFPRegOut(0);
                            target_isa = load_ins->getISAFPRegOut(0);
                        } break;
                        }

                        load_ins->computeLoadAddress(out, reg_file, &load_addr, &load_width);
                        reg_offset = load_ins->getRegisterOffset();

                        out->verbose(
                            CALL_INFO, 8, 0,
                            "--> load info: addr: 0x%llx / width: %" PRIu16 " / isa: %" PRIu16 " = target-reg: %" PRIu16
                            " / partial: %s / reg-offset: %" PRIu16 " / sgn-exd: %s\n",
                            load_addr, load_width, target_isa, target_reg, load_ins->isPartialLoad() ? "yes" : "no",
                            reg_offset, load_ins->performSignExtension() ? "yes" : "no");

                        switch (load_ins->getValueRegisterType()) {
                        case LOAD_INT_REGISTER: {
                            if (target_reg != load_ins->getISAOptions()->getRegisterIgnoreWrites()) {
                                // if we are doing a partial copy and we aren't starting at
                                // zero then we need to copy bytewise, if not, we can do a
                                // single copy
                                if (load_ins->isPartialLoad()) {
                                    char* reg_ptr = reg_file->getIntReg(target_reg);

                                    for (uint16_t i = 0; i < load_width; ++i) {
                                        reg_ptr[reg_offset + i] = ev->data[i];
                                    }

                                    out->verbose(CALL_INFO, 8, 0,
                                                "----> partial-load addr: 0x%llx / reg-offset: %" PRIu16
                                                " / load-width: %" PRIu16 " / full-width: %" PRIu16 "\n",
                                                load_addr, reg_offset, load_width, load_ins->getLoadWidth());

                                    if ((reg_offset + load_width) >= load_ins->getLoadWidth()) {
                                        if ((reg_ptr[reg_offset + load_width - 1] & 0x80) != 0) {
                                            // We need to perform sign extension, fill every byte with
                                            // all 1s
                                            for (uint16_t i = reg_offset + load_width; i < 8; ++i) {
                                                reg_ptr[i] = 0xFF;
                                            }
                                        }
                                    }
                                } else {
                                    if (load_ins->performSignExtension()) {
                                        switch (load_width) {
                                        case 1: {
                                            int8_t* v_8 = (int8_t*)&ev->data[0];
                                            reg_file->setIntReg<int8_t>(target_reg, *v_8, true);
                                        } break;
                                        case 2: {
                                            int16_t* v_16 = (int16_t*)&ev->data[0];
                                            reg_file->setIntReg<int16_t>(target_reg, *v_16, true);
                                        } break;
                                        case 4: {
                                            int32_t* v_32 = (int32_t*)&ev->data[0];
                                            reg_file->setIntReg<int32_t>(target_reg, *v_32, true);
                                        } break;
                                        case 8: {
                                            int64_t* v_64 = (int64_t*)&ev->data[0];
                                            reg_file->setIntReg<int64_t>(target_reg, *v_64, true);
                                        } break;
                                        }
                                    } else {
                                        switch (load_width) {
                                        case 1: {
                                            uint8_t* v_8 = (uint8_t*)&ev->data[0];
                                            reg_file->setIntReg<uint8_t>(target_reg, *v_8, false);
                                        } break;
                                        case 2: {
                                            uint16_t* v_16 = (uint16_t*)&ev->data[0];
                                            reg_file->setIntReg<uint16_t>(target_reg, *v_16, false);
                                        } break;
                                        case 4: {
                                            uint32_t* v_32 = (uint32_t*)&ev->data[0];
                                            reg_file->setIntReg<uint32_t>(target_reg, *v_32, false);
                                        } break;
                                        case 8: {
                                            uint64_t* v_64 = (uint64_t*)&ev->data[0];
                                            reg_file->setIntReg<uint64_t>(target_reg, *v_64, false);
                                        } break;
                                        }
                                    }
                                }
                            }
                        } break;

                        case LOAD_FP_REGISTER: {
                            if (load_ins->isPartialLoad()) {
                                out->fatal(CALL_INFO, -1,
                                          "Error - does not support partial load of a "
                                          "floating point register.\n");
                            } else {
                                char* reg_ptr = reg_file->getFPReg(target_reg);

                                switch (load_width) {
                                case 4: {
                                    float* v_f = (float*)&ev->data[0];
                                    reg_file->setFPReg(target_reg, (*v_f));
                                } break;
                                case 8: {
                                    double* v_d = (double*)&ev->data[0];
                                    reg_file->setFPReg(target_reg, (*v_d));
                                } break;
                                default: {
                                    out->fatal(CALL_INFO, -1,
                                              "Error - load to floating point register in not "
                                              "supported size (%" PRIu16 ")\n",
                                              load_width);
                                } break;
                                }
                            }
                        } break;
                        }

                        // mark instruction as executed
                        load_ins->markExecuted();
                    }

                    // Clean up
                    delete (*op_q_itr);
                    lsq->op_q.erase(op_q_itr);
                    processed = true;

                    break;
                } else {
                    op_q_itr++;
                }
            }

            if (!processed) {
                out->verbose(CALL_INFO, 16, 0, "did not match any request.\n");
            }

            delete ev;
        }

        virtual void handle(StandardMem::WriteResp* ev) override {
            out->verbose(CALL_INFO, 16, 0, "recv incoming d-cache event, addr: 0x%llx, size: %" PRIu32 "\n", ev->vAddr,
                        (uint32_t)ev->size);

            bool processed = false;
            for (auto op_q_itr = lsq->op_q.begin(); op_q_itr != lsq->op_q.end();) {
                // is the operation issued to the memory system and do we match ID
                if ((*op_q_itr)->isOperationIssued() && (ev->getID() == (*op_q_itr)->getRequestID())) {

                    out->verbose(CALL_INFO, 8, 0, "matched a load record (addr: 0x%llx)\n",
                                    (*op_q_itr)->getInstruction()->getInstructionAddress());

                    // TODO assert (!(*op_q_itr)->isLoad()) ?

                    // Clean up
                    delete (*op_q_itr);
                    lsq->op_q.erase(op_q_itr);
                    processed = true;

                    break;
                } else {
                    op_q_itr++;
                }
            }

            for (auto sc_itr = lsq->sc_inflight.begin(); sc_itr != lsq->sc_inflight.end();) {
                if (ev->getID() == (*sc_itr)->getRequestID()) {
                    out->verbose(CALL_INFO, 16, 0, "matched an inflight LLSC_STORE operation\n");
                    lsq->printEventFlags(ev);

                    VanadisStoreInstruction* store_ins = (*sc_itr)->getInstruction();

                    switch (store_ins->getValueRegisterType()) {
                    case STORE_INT_REGISTER: {
                        const uint16_t value_reg = store_ins->getPhysIntRegOut(0);

                        if (ev->getSuccess()) {
                            out->verbose(CALL_INFO, 16, 0,
                                        "---> LSQ LLSC-STORE ins: 0x%llx / addr: 0x%llx / rt: %" PRIu16
                                        " <- 1 (success)\n",
                                        store_ins->getInstructionAddress(), ev->vAddr, value_reg);
                            lsq->registerFiles->at(store_ins->getHWThread())->setIntReg<uint64_t>(value_reg, (uint64_t)1);
                        } else {
                            out->verbose(CALL_INFO, 16, 0,
                                        "---> LSQ LLSC-STORE ins: 0x%llx / addr: 0x%llx rt: %" PRIu16
                                        " <- 0 (failed)\n",
                                        store_ins->getInstructionAddress(), ev->vAddr, value_reg);
                            lsq->registerFiles->at(store_ins->getHWThread())->setIntReg<uint64_t>(value_reg, (uint64_t)0);
                        }
                    } break;
                    case STORE_FP_REGISTER: {
                        const uint16_t value_reg = store_ins->getPhysFPRegOut(0);

                        if (ev->getSuccess()) {
                            out->verbose(CALL_INFO, 16, 0,
                                        "---> LSQ LLSC-STOREFP ins: 0x%llx / addr: 0x%llx "
                                        "/ rt: %" PRIu16 " <- 1.0 (success)\n",
                                        store_ins->getInstructionAddress(), ev->vAddr, value_reg);
                            lsq->registerFiles->at(store_ins->getHWThread())->setFPReg(value_reg, 1.0f);
                        } else {
                            out->verbose(CALL_INFO, 16, 0,
                                        "---> LSQ LLSC-STOREFP ins: 0x%llx / addr: 0x%llx rt: %" PRIu16
                                        " <- 0.0 (failed)\n",
                                        store_ins->getInstructionAddress(), ev->vAddr, value_reg);
                            lsq->registerFiles->at(store_ins->getHWThread())->setFPReg(value_reg, 0.0f);
                        }
                    } break;
                    }

                    // Mark store instruction as executed and ensure we delete it
                    (*sc_itr)->getInstruction()->markExecuted();
                    delete (*sc_itr);

                    lsq->sc_inflight.erase(sc_itr);
                    processed = true;

                    break;
                } else {
                    sc_itr++;
                }
            }

            if (!processed) {
                out->verbose(CALL_INFO, 16, 0, "did not match any request.\n");
            }

            delete ev;
        }

            VanadisSequentialLoadStoreQueue* lsq;
    };

    void processIncomingDataCacheEvent(StandardMem::Request* ev) {
        ev->handle(std_mem_handlers);
    }

    virtual void clearLSQByThreadID(const uint32_t thread) {
        output->verbose(CALL_INFO, 8, 0, "clear for thread %" PRIu32 ", size: %" PRIu32 "\n", thread,
                        (uint32_t)op_q.size());

        for (auto op_q_itr = op_q.begin(); op_q_itr != op_q.end();) {
            if ((*op_q_itr)->getInstruction()->getHWThread() == thread) {
                delete (*op_q_itr);
                op_q_itr = op_q.erase(op_q_itr);
            } else {
                op_q_itr++;
            }
        }

        output->verbose(CALL_INFO, 8, 0, "clear complete, size: %" PRIu32 "\n", (uint32_t)op_q.size());
    }

    virtual void init(unsigned int phase) {
        output->verbose(CALL_INFO, 2, 0, "LSQ Memory Interface Init, Phase %u\n", phase);
        memInterface->init(phase);
    }

    virtual void setInitialMemory(const uint64_t addr, std::vector<uint8_t>& payload) {
        output->verbose(CALL_INFO, 2, 0, "setting initial memory contents for address 0x%llx / size: %" PRIu64 "\n",
                        addr, (uint64_t)payload.size());
        memInterface->sendUntimedData(new StandardMem::Write(addr, payload.size(), payload));

        if (fault_on_memory_not_written) {
            // Mark every byte which we are initializing
            for (uint64_t i = addr; i < (addr + payload.size()); ++i) {
                memory_check_table->markByte(i);
            }
        }
    }

protected:
    // TODO should we use getString() here or does it need to be terse?
    void writeTrace(VanadisInstruction* ins, std::string req_type, std::string sub_type, uint64_t address, uint64_t size) {
        if (nullptr != address_trace_file)
            fprintf(address_trace_file, "%8s %5s 0x%016llx %5" PRIu64 " 0x%016llx\n", req_type.c_str(), sub_type.c_str(), address,
                    size, ins->getInstructionAddress());
            fflush(address_trace_file);

    }

    void printEventFlags(StandardMem::Request* ev) {
        output->verbose(CALL_INFO, 16, 0, "-> %s", ev->getString().c_str());
    }

    size_t max_q_size;

    std::deque<VanadisSequentialLoadStoreRecord*> op_q;
    std::unordered_set<VanadisSequentialLoadStoreSCRecord*> sc_inflight;

    StandardMem* memInterface;
    StandardMemHandlers* std_mem_handlers;
    FILE* address_trace_file;

    bool allow_speculated_operations;
    bool fault_on_memory_not_written;

    uint64_t flag_non_written_loads_count;

    VanadisMemoryWrittenRecord* memory_check_table;
};

} // namespace Vanadis
} // namespace SST

#endif
