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

#ifndef _H_VANADIS_BASIC_LSQ
#define _H_VANADIS_BASIC_LSQ

#include <sst/core/output.h>
#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/stdMem.h>

#include "lsq/vlsq.h"
#include "lsq/vbasiclsqentry.h"
#include "util/vsignx.h"
#include "inst/vstorecond.h"

#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <vector>
#include <queue>

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisBasicLoadStoreQueue : public SST::Vanadis::VanadisLoadStoreQueue {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(VanadisBasicLoadStoreQueue, "vanadis", "VanadisBasicLoadStoreQueue",
                                          SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                          "Implements a basic load-store queue for use with the SST standardInterface",
                                          SST::Vanadis::VanadisLoadStoreQueue)

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS({ "memory_interface", "Set the interface to memory",
                                          "SST::Interfaces::StandardMem" })

    SST_ELI_DOCUMENT_PORTS({ "dcache_link", "Connects the LSQ to the data cache", {} })

    SST_ELI_DOCUMENT_PARAMS(
            { "verbose", "Set the verbosity of output for the LSQ", "0" }, 
            { "max_stores", "Set the maximum number of stores permitted in the queue", "8" }, 
            { "max_loads", "Set the maximum number of loads permitted in the queue", "16" },
            { "address_mask", "Can mask off address bits if needed during construction of a operation", "0xFFFFFFFFFFFFFFFF"},
            { "issues_per_cycle", "Maximum number of issues the LSQ can attempt per cycle.", "2"}
        )

    SST_ELI_DOCUMENT_STATISTICS({ "bytes_read", "Count all the bytes read for data operations", "bytes", 1 },
                                { "bytes_stored", "Count all the bytes written for data operations", "bytes", 1 },
                                { "loads_issued", "Count the number of loads issued", "operations", 1 },
                                { "stores_issued", "Count the number of stores issued", "operations", 1 },
                                { "fences_issued", "Count the number of fences issued", "operations", 1},
                                { "loads_executed", "Count the number of loads issued", "operations", 1 },
                                { "stores_executed", "Count the number of stores issued", "operations", 1 },
                                { "fences_executed", "Count the number of fences issued", "operations", 1},
                                { "operations_pending", "Count the number of operations which are held by the LSQ and not ready to be issued to the memory subsystem", "operations", 1},
                                { "loads_in_flight", "Count the number of loads which are in-flight", "operations", 1},
                                { "stores_in_flight", "Count the number of stores which are in-flight", "operations", 1},
                                { "store_buffer_entries", "Count the number of stores held in the store buffer", "operations", 1},
                                { "split_stores", "Count the number of stores which are fractured due to cache boundaries", "operations", 1},
                                { "split_loads", "Count the number of loads which are fractured due to cache boundaries", "operations", 1})

    VanadisBasicLoadStoreQueue(ComponentId_t id, Params& params) : VanadisLoadStoreQueue(id, params),
        max_stores(params.find<size_t>("max_stores", 8)),
        max_loads(params.find<size_t>("max_loads", 16)),
        max_issue_attempts_per_cycle(params.find("issues_per_cycle", 2)) {

        std_mem_handlers = new VanadisBasicLoadStoreQueue::StandardMemHandlers(this, output);

        memInterface = loadUserSubComponent<Interfaces::StandardMem>(
            "memory_interface", ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, getTimeConverter("1ps"),
            new StandardMem::Handler<SST::Vanadis::VanadisBasicLoadStoreQueue>(
                this, &VanadisBasicLoadStoreQueue::processIncomingDataCacheEvent));

        address_mask = params.find<uint64_t>("address_mask", 0xFFFFFFFFFFFFFFFFULL);

        cache_line_width = params.find<uint64_t>("cache_line_width", 64);

        stat_loads_issued = registerStatistic<uint64_t>("loads_issued", "1");
        stat_stores_issued = registerStatistic<uint64_t>("stores_issued", "1");
        stat_fences_issued = registerStatistic<uint64_t>("fences_issued", "1");

        stat_loads_executed = registerStatistic<uint64_t>("loads_executed", "1");
        stat_stores_executed = registerStatistic<uint64_t>("stores_executed", "1");
        stat_fences_executed = registerStatistic<uint64_t>("fences_executed", "1");

        stat_loaded_bytes = registerStatistic<uint64_t>("bytes_read", "1");
        stat_stored_bytes = registerStatistic<uint64_t>("bytes_stored", "1");

        stat_store_buffer_entries = registerStatistic<uint64_t>("store_buffer_entries", "1");
        stat_stores_pending = registerStatistic<uint64_t>("stores_in_flight", "1");
        stat_loads_pending = registerStatistic<uint64_t>("loads_in_flight", "1");
        stat_op_q_size = registerStatistic<uint64_t>("operations_pending");
    }

    virtual ~VanadisBasicLoadStoreQueue() {
        for(auto op_q_itr = op_q.begin(); op_q_itr != op_q.end(); ) {
            delete (*op_q_itr);
            op_q_itr = op_q.erase(op_q_itr);
        }

        delete std_mem_handlers;
    }

    bool storeFull() override { return op_q.size() >= max_stores; }
    bool loadFull() override { return op_q.size() >= max_loads; }
    bool storeBufferFull() override { return std_stores_in_flight.size() >= max_stores; }

    size_t storeSize() override { return op_q.size(); }
    size_t loadSize() override { return op_q.size(); }
    size_t storeBufferSize() override { return std_stores_in_flight.size(); }

    void push(VanadisStoreInstruction* store_me) override {
        op_q.push_back( new VanadisBasicStoreEntry(store_me) );
        stat_store_issued->addData(1);
    }

    void push(VanadisLoadInstruction* load_me) override {
        op_q.push_back( new VanadisBasicLoadEntry(load_me) );
        stat_loads_issued->addData(1);
    }

    void push(VanadisFenceInstruction* fence) override {
        op_q.push_back( new VanadisBasicFenceEntry(fence) );
        stat_fences_issued->addData(1);
    }

    void clearLSQByThreadID(const uint32_t thread) override {
        // Iterate over the queue, anything with a matching thread ID is
        // first deleted and then removed from the queue, otherwise entry
        // is left alone
        for(auto op_q_itr = op_q.begin(); op_q_itr != op_q.end(); ) {
            if( (*op_q_itr)->getHWThread() == thread ) {
                delete (*op_q_itr);
                op_q_itr = op_q.erase(op_q_itr);
            }
        }

        for(auto load_itr = loads_pending.begin(); load_itr != loads_pending.end(); ) {
            if( (*load_itr)->getHWThread() == thread ) {
                delete (*load_itr);
                load_itr = loads_pending.erase(load_itr);
            }
        }

        for(auto store_itr = stores_pending.begin(); store_itr != stores_pending.end(); ) {
            if( (*store_itr)->getHWThread() == thread) {
                delete (*store_itr);
                store_itr = stores_pending.erase(store_itr);
            }
        }
    }

    // must be implemented to allow the memory system to initialize itself during
    // boot-up
    void init(unsigned int phase) override { 
        memInterface->init(phase); 

        // update the cache line size each cycle to make sure we get updates
        cache_line_width = memInterface->getLineSize();

        output->verbose(CALL_INFO, 2, 0, "updating cache line size to: %" PRIu64 "\n", cache_line_width);
    }

    void printStatus(SST::Output& out) override {
        int32_t next_line = 0;

        if(output->getVerboseLevel() >= 16) {
            for(auto op_q_itr = op_q.begin(); op_q_itr != op_q.end(); op_q_itr++) {
                VanadisBasicLoadStoreEntryOp op_type = (*op_q_itr)->getEntryOp();

                out.verbose(CALL_INFO, 16, 0, "-> [%4" PRId32 "] type: %5s ins: 0x%8llx thr: %4" PRIu32 "\n",
                    next_line,
                    (op_type == VanadisBasicLoadStoreEntryOp::LOAD) ? "LOAD" :
                    (op_type == VanadisBasicLoadStoreEntryOp::STORE) ? "STORE" : "FENCE",
                    (*op_q_itr)->getInstruction()->getInstructionAddress(),
                    (*op_q_itr)->getInstruction()->getHWThread());
                next_line++;
            }
        }
    }

    void tick(uint64_t cycle) override {
        if(output->getVerboseLevel() >= 16) {
            output->verbose(CALL_INFO, 16, 0, "-> tick LSQ at cycle %" PRIu64 "\n", cycle);

            if(loads_pending.size() > 0) {
                for(int i = loads_pending.size() - 1; i >= 0; i--) {
                    VanadisBasicLoadPendingEntry* load_entry = loads_pending.at(i);

                    output->verbose(CALL_INFO, 16, 0, "-->   load[%5d] ins: 0x%llx / thr: %" PRIu32 " / addr: 0x%llx / width: %" PRIu32 "\n",
                        i, load_entry->getLoadInstruction()->getInstructionAddress(),
                        load_entry->getLoadInstruction()->getHWThread(),
                        load_entry->getLoadAddress(), load_entry->getLoadWidth()); 
                }
            }

            if(stores_pending.size() > 0) {
                for(int i = stores_pending.size() - 1; i >= 0; i--) {
                    VanadisBasicStorePendingEntry* store_entry = stores_pending.at(i);

                    output->verbose(CALL_INFO, 16, 0, "--> stores[%5d] ins: 0x%llx / thr: %" PRIu32 " / addr: 0x%llx / width: %" PRIu32 "\n",
                        i, store_entry->getStoreInstruction()->getInstructionAddress(),
                        store_entry->getStoreInstruction()->getHWThread(),
                        store_entry->getStoreAddress(), store_entry->getStoreWidth());
                }
            }
        }

        stat_op_q_size->addData(op_q.size());
        stat_loads_pending->addData(loads_pending.size());
        stat_stores_pending->addData(std_stores_in_flight.size());
        stat_store_buffer_entries->addData(stores_pending.size());

        // this can be called multiple times per cycle if needed but lets just do once for now
        for(uint32_t attempt = 0; attempt < max_issue_attempts_per_cycle; ++attempt) {
            const bool attempt_result = attempt_to_issue(cycle, attempt);

            // if the attempt failed then stop because it will just fail again this cycle
            if(!attempt_result) {
                break;
            }
        }

        // attempt to issue any front of ROB stores into memory system
        issueStoreFront();
    }

protected:

    class StandardMemHandlers : public Interfaces::StandardMem::RequestHandler {
    public:
        friend class VanadisBasicLoadStoreQueue;

        StandardMemHandlers(VanadisBasicLoadStoreQueue* lsq, SST::Output* output) :
                Interfaces::StandardMem::RequestHandler(output), lsq(lsq) {}
        
        virtual ~StandardMemHandlers() {}

        virtual void handle(StandardMem::ReadResp* ev) {
            out->verbose(CALL_INFO, 16, 0, "-> handle read-response (virt-addr: 0x%llx)\n", ev->vAddr);
            lsq->stat_loaded_bytes->addData(ev->size);

            auto load_itr = lsq->loads_pending.begin();
            VanadisBasicLoadPendingEntry* load_entry = nullptr;

            for(; load_itr != lsq->loads_pending.end(); load_itr++) {
                if((*load_itr)->containsRequest(ev->getID())) {
                    load_entry = *load_itr;
                    break;
                }
            }

            if(nullptr == load_entry) {
                // not found, so previous cleared by a branch mis-predict ignore
                return;
            }

            if(out->getVerboseLevel() >= 16) {
                out->verbose(CALL_INFO, 16, 0,
                                "--> LSQ match load entry, unpacking payload "
                                "(load-addr: 0x%0llx, load-thr: %" PRIu32 ").\n",
                                ev->pAddr, load_entry->getHWThread());
            }

            VanadisLoadInstruction* load_ins = load_entry->getLoadInstruction();

            const uint16_t load_width = ev->size;
            const uint64_t load_address = load_entry->getLoadAddress();
            const uint32_t hw_thr     = load_ins->getHWThread();

            uint16_t target_reg = 0;
            uint16_t target_isa_reg = 0;
            uint64_t reg_offset  = load_ins->getRegisterOffset();
            uint64_t addr_offset = ev->vAddr - load_address;
            uint32_t reg_width = 0;

            if(out->getVerboseLevel() >= 16) {
                out->verbose(CALL_INFO, 16, 0, "---> LSQ recv load event ins: 0x%llx / hw-thr: %" PRIu32 " / entry-addr: 0x%llx / entry-width: %" PRIu16 " / reg-offset: %" PRIu64 " / ev-addr: 0x%llx / ev-width: %" PRIu16 " / addr-offset %" PRIu64 " / sign-extend: %s / target-isa-reg: %" PRIu16 " / target-phys-reg: %" PRIu16 " / reg-type: %s\n",
                    load_ins->getInstructionAddress(), hw_thr, load_address, load_width, reg_offset, ev->vAddr, ev->size, 
                    addr_offset, (load_ins->performSignExtension() ? "yes" : "no"),
                    target_isa_reg, target_reg,
                    (load_ins->getValueRegisterType() == LOAD_INT_REGISTER) ? "int" : "fp");
            }
            
            switch(load_ins->getValueRegisterType()) {
            case LOAD_INT_REGISTER: {
                target_isa_reg = load_ins->getISAIntRegOut(0);
                target_reg = load_ins->getPhysIntRegOut(0);

                assert(target_isa_reg < load_ins->getISAOptions()->countISAIntRegisters());

                if(target_reg != load_ins->getISAOptions()->getRegisterIgnoreWrites()) {
                    reg_width = lsq->registerFiles->at(hw_thr)->getIntRegWidth();
                    std::vector<uint8_t> register_value(reg_width);
                    // copy entire register here
                    lsq->registerFiles->at(hw_thr)->copyFromIntRegister(target_reg, 0, &register_value[0], reg_width);

                    assert((reg_offset + addr_offset + ev->size) <= reg_width);

                    for(auto i = 0; i < ev->size; ++i) {
                        register_value.at(reg_offset + addr_offset + i) = ev->data[i];
                    }

                    // if we are the last request to be processed for this load (if any were split)
                    // and we promised to do sign extension, then perform it now
                    if(load_entry->countRequests() == 1) {
                        if(load_ins->performSignExtension()) {
                            if((register_value.at(reg_offset + addr_offset + load_width - 1) & 0x80) != 0) {
                                for(auto i = reg_offset + addr_offset + load_width; i < reg_width; ++i) {
                                    register_value.at(i) = 0xFF;
                                }
                            } else {
                                for(auto i = reg_offset + addr_offset + load_width; i < reg_width; ++i) {
                                    register_value.at(i) = 0x00;
                                }
                            }
                        } else {
                            for(auto i = reg_offset + addr_offset + load_width; i < reg_width; ++i) {
                                register_value.at(i) = 0x00;
                            }
                        }
                    }

                    lsq->registerFiles->at(hw_thr)->copyToIntRegister(target_reg, 0, &register_value[0], register_value.size());
                }
            } break;
            case LOAD_FP_REGISTER: {
                target_isa_reg = load_ins->getISAFPRegOut(0);
                target_reg = load_ins->getPhysFPRegOut(0);

                reg_width = lsq->registerFiles->at(hw_thr)->getFPRegWidth();
                std::vector<uint8_t> register_value(reg_width);
                
                // copy entire register here
                lsq->registerFiles->at(hw_thr)->copyFromFPRegister(target_reg, 0, &register_value[0], reg_width);

                assert((reg_offset + addr_offset + ev->size) <= reg_width);

                for(auto i = reg_offset + addr_offset; i < ev->size; ++i) {
                    register_value.at(reg_offset + addr_offset + i) = ev->data[i];
                }

                if(load_entry->countRequests() == 1) {
                    for(auto i = reg_offset + addr_offset + load_width; i < reg_width; ++i) {
                        register_value.at(i) = 0x00;
                    }
                }

                lsq->registerFiles->at(hw_thr)->copyToFPRegister(target_reg, 0, &register_value[0], reg_width);
            } break;
            default:
                out->fatal(CALL_INFO, -1, "Unknown register type.\n");
            }

            ///////////////////////////////////////////////////////////////////////////////////

            if (ev->vAddr < 64) {
                load_ins->flagError();
            }

            load_entry->removeRequest(ev->getID());

            if(0 == load_entry->countRequests()) {
                if(out->getVerboseLevel() >= 16) {
                    out->verbose(CALL_INFO, 16, 0,
                        "---> LSQ Execute: %s (0x%llx / thr: %" PRIu32 ") load data instruction "
                        "marked executed.\n",
                        load_ins->getInstCode(), load_ins->getInstructionAddress(), load_ins->getHWThread());
                }

                load_ins->markExecuted();
                lsq->stat_loads_executed->addData(1);
                lsq->loads_pending.erase(load_itr);
                delete load_entry;
            } else {
                if(out->getVerboseLevel() >= 16) {
                    out->verbose(CALL_INFO, 16, 0,
                        "---> LSQ Execute: %s (0x%llx / thr:%" PRIu32 ") does not have all requests completed yet %" PRIu32 " left, will not execute until all done.\n",
                            load_ins->getInstCode(), load_ins->getInstructionAddress(), load_ins->getHWThread(), load_entry->countRequests());
                }
            }

            delete ev;
        } 
        
        virtual void handle(StandardMem::WriteResp* ev) {
            out->verbose(CALL_INFO, 16, 0, "-> handle write-response (virt-addr: 0x%llx)\n", ev->vAddr);
            lsq->stat_stored_bytes->addData(ev->size);

            bool std_store_found = false;

            for(auto std_store_itr = lsq->std_stores_in_flight.begin(); std_store_itr != lsq->std_stores_in_flight.end();
                std_store_itr++) {
                    if((*std_store_itr) == ev->getID()) {
                        lsq->std_stores_in_flight.erase(std_store_itr);
                        std_store_found = true;
                        break;
                    }
                }

            if(std_store_found) {
                out->verbose(CALL_INFO, 16, 0, "--> write-response is a standard store is matched and cleared from in-flight operations successfully.\n");
                delete ev;
                return;
            }

            // this was not a standard store OR was removed by a branch mis-predict but we need to find
            // out now

            if(0 == lsq->stores_pending.size()) {
                // no other pending stores so this request is free and can move on
                delete ev;
                return;
            }

            VanadisBasicStorePendingEntry* store_entry = lsq->stores_pending.front();

            if(store_entry->containsRequest(ev->getID())) {
                VanadisStoreInstruction* store_ins = store_entry->getStoreInstruction();

                switch(store_ins->getTransactionType()) {
                case MEM_TRANSACTION_LLSC_STORE:
                {
                    const uint16_t value_reg = store_ins->getPhysIntRegOut(0);

                    VanadisStoreConditionalInstruction* store_cond_ins = dynamic_cast<VanadisStoreConditionalInstruction*>(store_ins);

                    if(UNLIKELY(nullptr == store_cond_ins)) {
                        out->fatal(CALL_INFO, -1, "Unable to cast an LLSC_STORE into a store-conditional, logic failure.\n");
                    }

                    if (ev->getSuccess()) {
                        const int64_t success_result = store_cond_ins->getResultSuccess();

                        out->verbose(CALL_INFO, 16, 0,
                                        "---> LSQ LLSC-STORE rt: %" PRIu16 " <- %" PRId64 " (success)\n", 
                                        success_result, value_reg);
                        lsq->registerFiles->at(store_ins->getHWThread())->setIntReg<int64_t>(value_reg,
                            success_result);
                    } else {
                        const int64_t failure_result = store_cond_ins->getResultFailure();

                        out->verbose(CALL_INFO, 16, 0, "---> LSQ LLSC-STORE rt: %" PRIu16 " <- %" PRId64 " (failed)\n",
                                        failure_result, value_reg);
                        lsq->registerFiles->at(store_ins->getHWThread())->setIntReg<uint64_t>(value_reg,
                            failure_result);
                    }

                    store_entry->getInstruction()->markExecuted();
                    lsq->stores_pending.erase(lsq->stores_pending.begin());
                    delete store_entry;
                    delete ev;
                } break;
                case MEM_TRANSACTION_LOCK:
                {
                    store_entry->getInstruction()->markExecuted();
                    lsq->stores_pending.erase(lsq->stores_pending.begin());
                    delete store_entry;
                    delete ev;
                } break;
                default:
                {
                    // this is a logical error. fatal()
                    out->fatal(CALL_INFO, -1, "Error - reached a transaction NONE or LLSC_LOAD in a store return. Logical error (ins: 0x%llx / thr: %" PRIu32 ")\n",
                        store_ins->getInstructionAddress(), store_ins->getHWThread());
                } break;
                }
            } else {
                delete ev;
                return;
            }
        }
    
        VanadisBasicLoadStoreQueue* lsq;
    };

    void processIncomingDataCacheEvent(StandardMem::Request* ev) {
        output->verbose(CALL_INFO, 16, 0, "received incoming data cache request -> processIncomingDataCacheEvent()\n");

        assert(ev != nullptr);
        assert(std_mem_handlers != nullptr);

        ev->handle(std_mem_handlers);
        output->verbose(CALL_INFO, 16, 0, "completed pass off to incoming handlers\n");
    }

    void issueStoreFront() {
        if(output->getVerboseLevel() >= 16) {
            output->verbose(CALL_INFO, 16, 0, "issue store-front -> check store is front of ROB and attempt to issue\n");
        }

        if(stores_pending.empty()) {
            output->verbose(CALL_INFO, 16, 0, "-> stores-pending is empty, return without processing\n");
            return;
        }

        // check the front store of the pending queue, if this isn't currently dispatched
        // and is front of ROB, then we can execute it into the memory system
        VanadisBasicStorePendingEntry* current_store = stores_pending.front();
        VanadisStoreInstruction* store_ins = current_store->getStoreInstruction();

        if(output->getVerboseLevel() >= 16) {
            output->verbose(CALL_INFO, 16, 0, "-> current store queue front is ins addr: 0x%llx\n", store_ins->getInstructionAddress());
        }

        // check we have not already dispatched this entry
        if( UNLIKELY(!current_store->isDispatched()) ) {
            output->verbose(CALL_INFO, 16, 0, "--> store-front is not already dispatched so attempt to put into memory system.\n");

            if( UNLIKELY(store_ins->checkFrontOfROB()) && LIKELY(store_ins->completedIssue()) ) {
                output->verbose(CALL_INFO, 16, 0, "---> store is at front of ROB so OK to push into memory system.\n");

                // store instruction is current front of ROB so ready to be send to memory system
                bool issue_result = issueStore(current_store, store_ins);

                output->verbose(CALL_INFO, 16, 0, "---> attempt to issue store result is: %s\n",
                    issue_result ? "success" : "failed");

                // this was a standard store (not LLSC/LOCK) and we issued into system successfully
                if(LIKELY(issue_result)) {
                    stores_pending.pop_front();
                    delete current_store;

                    if(output->getVerboseLevel() >= 16) {
                        output->verbose(CALL_INFO, 16, 0, "---> issued store: 0x%llx / thr: %" PRIu32 " into memory system using standard store operation\n",
                            store_ins->getInstructionAddress(), store_ins->getHWThread());
                    }

                    // mark executed
                    store_ins->markExecuted();
                    stat_stores_executed->addData(1);
                } else {
                    output->verbose(CALL_INFO, 16, 0, "---> issued non-standard store: 0x%llx / thr: %" PRIu32 " (marked dispatch, will stall until response)\n",
                        store_ins->getInstructionAddress(), store_ins->getHWThread());
                    current_store->markDispatched();
                }
            } else {
                output->verbose(CALL_INFO, 16, 0, "----> store is not at ROB front so need to wait until this is marked\n");
            }
        }
    }

    bool issueStore(VanadisBasicStorePendingEntry* store_entry, 
            VanadisStoreInstruction* store_ins) {

        const uint64_t store_address = store_entry->getStoreAddress();
        const uint64_t store_width   = store_entry->getStoreWidth();
        StandardMem::Request* store_req = nullptr;
        std::vector<uint8_t> payload(store_width);

        const bool needs_split = operationStraddlesCacheLine(store_address, store_width);

        if(output->getVerboseLevel() >= 16) {
            output->verbose(CALL_INFO, 16, 0, "--> issue-store at ins: 0x%llx / store-addr: 0x%llx / width: %" PRIu64 " / partial: %s / split: %s / offset: %" PRIu32 "\n",
                store_ins->getInstructionAddress(), store_address, store_width, store_ins->isPartialStore() ? "yes" : "no", needs_split ? "yes" : "no",
                store_ins->getRegisterOffset());
        }

        // if the store is not a split operation, then copy payload we are good to go, if it is split
        // handle this case later after we do a load of address and width calculation
        if(LIKELY(! needs_split)) {
            registerFiles->at(store_entry->getHWThread())->copyFromRegister(store_ins->getValueRegisterType() == STORE_FP_REGISTER ?
                store_ins->getPhysFPRegIn(0) : store_ins->getPhysIntRegIn(1), store_ins->getRegisterOffset(), &payload[0], store_width, 
                store_ins->getValueRegisterType() == STORE_FP_REGISTER);
        }

        switch(store_ins->getTransactionType()) {
        case MEM_TRANSACTION_NONE:
        {
            if(UNLIKELY(needs_split)) {
                if(output->getVerboseLevel() >= 16) {
                    output->verbose(CALL_INFO, 16, 0, "---> [memory-transaction]: standard split-store\n");
                }

                const uint64_t store_width_right = (store_address + store_width) % cache_line_width;
                const uint64_t store_width_left  = store_width - store_width_right;

                assert(store_width_left > 0);
                assert(store_width_right > 0);

                const uint64_t store_address_right = store_address + store_width_left;

                if(output->getVerboseLevel() >= 16) {
                    output->verbose(CALL_INFO, 16, 0, "---> store-left-at: 0x%llx left-width: %" PRIu64 ", store-right-at: 0x%llx right-width: %" PRIu64 "\n",
                        store_address, store_width_left, store_address_right, store_width_right);
                }

                payload.resize(store_width_left);
                registerFiles->at(store_entry->getHWThread())->copyFromRegister(store_ins->getValueRegisterType() == STORE_FP_REGISTER ?
                    store_ins->getPhysFPRegIn(0) : store_ins->getPhysIntRegIn(1), store_ins->getRegisterOffset(), &payload[0], store_width_left, 
                    store_ins->getValueRegisterType() == STORE_FP_REGISTER);

                store_req = new StandardMem::Write(store_address & address_mask, payload.size(), payload, 
                    false, 0, store_address, store_ins->getInstructionAddress(), store_ins->getHWThread());

                std_stores_in_flight.insert(store_req->getID());
                memInterface->send(store_req);

                payload.clear();

                payload.resize(store_width_right);
                registerFiles->at(store_entry->getHWThread())->copyFromRegister(store_ins->getValueRegisterType() == STORE_FP_REGISTER ?
                    store_ins->getPhysFPRegIn(0) : store_ins->getPhysIntRegIn(1), store_ins->getRegisterOffset() + store_width_left, &payload[0], 
                    store_width_right, 
                    store_ins->getValueRegisterType() == STORE_FP_REGISTER);

                store_req = new StandardMem::Write(store_address_right & address_mask, payload.size(), payload, 
                    false, 0, store_address_right, store_ins->getInstructionAddress(), store_ins->getHWThread());
                memInterface->send(store_req);
                std_stores_in_flight.insert(store_req->getID());

                return true;
            } else {
                if(output->getVerboseLevel() >= 16) {
                    output->verbose(CALL_INFO, 16, 0, "---> [memory-transaction]: standard store ins: 0x%llx store-at: 0x%llx width: %" PRIu64 "\n",
                        store_ins->getInstructionAddress(), store_address, store_width);

                    output->verbose(CALL_INFO, 16, 0, "-----> payload = {");

                    for(auto i = 0; i < payload.size(); ++i) {
                        output->verbose(CALL_INFO, 16, 0, " %x", payload[i]);
                    }

                    output->verbose(CALL_INFO, 16, 0, "}\n");
                }

                store_req = new StandardMem::Write(store_address & address_mask, payload.size(), payload, 
                    false, 0, store_address, store_ins->getInstructionAddress(), store_ins->getHWThread());
                std_stores_in_flight.insert(store_req->getID());
                memInterface->send(store_req);

                return true;
            }
        } break;
        case MEM_TRANSACTION_LLSC_LOAD:
        {  
            output->fatal(CALL_INFO, -1, "Error - attempted to issue a LLSC-load via store instruction. Invalid operation.\n");
        } break;
        case MEM_TRANSACTION_LLSC_STORE:
        {
            if(UNLIKELY(needs_split)) {
                output->fatal(CALL_INFO, -1, "Error - attempted to perform an LLSC-store over a split-cache line. This is not permitted.\n");
            } else {
                output->verbose(CALL_INFO, 16, 0, "---> [memory-transaction]: LLSC-store store-at: 0x%llx width: %" PRIu64 "\n",
                    store_address, store_width);
                
                store_req = new StandardMem::StoreConditional(store_address & address_mask, payload.size(), payload,
                            0, store_address, store_ins->getInstructionAddress());
            }
        } break;
        case MEM_TRANSACTION_LOCK:
        {
            if(UNLIKELY(needs_split)) {
                output->fatal(CALL_INFO, -1, "Error - attempted to perform an LOCK-store over a split-cache line. This is not permitted.\n");
            } else {
                output->verbose(CALL_INFO, 16, 0, "---> [memory-transaction]: LOCK-store store-at: 0x%llx width: %" PRIu64 "\n",
                    store_address, store_width);
                
                store_req = new StandardMem::WriteUnlock(store_address & address_mask, payload.size(), payload,
                            0, store_address, store_ins->getInstructionAddress());
            }
        } break;
        }

        if(nullptr != store_req) {
            // equivalent to a seg-fault for the store
            if(store_address < 4096) {
                store_ins->flagError();
            }

            output->verbose(CALL_INFO, 16, 0, "-----> store-request sent to memory interface / entry marked dispatched\n");
            memInterface->send(store_req);
            store_entry->addRequest(store_req->getID());
            store_entry->markDispatched();
        } else {
            output->verbose(CALL_INFO, 16, 0, "-----> store-request was not sent to memory interface, record is nullptr\n");
        }

        return false;
    }

    void issueLoad(VanadisLoadInstruction* load_ins, uint64_t load_address, uint64_t load_width) {
        StandardMem::Request* load_req = nullptr;

        // do we need to perform a split load (which loads from two cache lines)?
        const bool needs_split = operationStraddlesCacheLine(load_address, load_width);

        VanadisBasicLoadPendingEntry* load_entry = new VanadisBasicLoadPendingEntry(load_ins, load_address, load_width);

        if ( load_address + load_width < load_address || (load_address + load_width) & ~address_mask) {
            load_ins->markExecuted();
            return;
        }

        switch (load_ins->getTransactionType()) {
            case MEM_TRANSACTION_NONE:
            {
                if(UNLIKELY(needs_split)) {
                    output->verbose(CALL_INFO, 16, 0, "---> [memory-transaction]: standard load (line auto-split)\n");
                    // How many bytes are in the left most line?
                    const uint64_t load_width_right = (load_address + load_width) % cache_line_width;
                    assert(load_width_right > 0);
                    
                    const uint64_t load_width_left = load_width - load_width_right;
                    assert(load_width_left > 0);

                    const uint64_t load_right_start = load_address + load_width_left;
                    assert((load_right_start % cache_line_width) == 0);

                    if(output->getVerboseLevel() >= 16) {
                        output->verbose(CALL_INFO, 16, 0, "---> split load at-left: 0x%llx left-width: %" PRIu64 " / at-right: 0x%llx right-width: %" PRIu64 "\n",
                            load_address, load_width_left, load_address + load_width_left, load_width_right);
                    }

                    load_req = new StandardMem::Read(load_address & address_mask, load_width_left, 0, 
                        load_address, load_ins->getInstructionAddress(), load_ins->getHWThread());
                    
                    load_entry->addRequest(load_req->getID());
                    memInterface->send(load_req);

                    load_req = new StandardMem::Read((load_address + load_width_left) & address_mask, load_width_right, 0, 
                        load_address + load_width_left, load_ins->getInstructionAddress(), load_ins->getHWThread());
                } else {
                    if(output->getVerboseLevel() >= 16) {   
                        output->verbose(CALL_INFO, 16, 0, "---> [memory-transaction]: standard load (not split) load-at: 0x%llx width: %" PRIu64 "\n",
                            load_address, load_width);
                    }

                    assert(load_width <= 8);
                    assert(load_width >= 0);

                    load_req = new StandardMem::Read(load_address & address_mask, load_width, 0, 
                        load_address, load_ins->getInstructionAddress(), load_ins->getHWThread());
                }
            } break;
            case MEM_TRANSACTION_LLSC_LOAD:
            {
                if(UNLIKELY(needs_split)) {
                    output->verbose(CALL_INFO, 16, 0, "---> load is marked LLSC but it requires a cache line split, generates an error\n");
                    load_ins->flagError();
                } else {
                    output->verbose(CALL_INFO, 16, 0, "---> [memory-transaction]: LLSC-load (not split) load-at: 0x%llx width: %" PRIu64 "\n",
                        load_address, load_width);
                    load_req = new StandardMem::LoadLink(load_address & address_mask, load_width, 0, 
                                        load_address, load_ins->getInstructionAddress(), load_ins->getHWThread());
                }
            } break;
            case MEM_TRANSACTION_LLSC_STORE:
            {
                output->fatal(CALL_INFO, -1,
                    "Error - logical error, LOAD instruction is marked with "
                    "an LLSC STORE transaction class.\n");
            } break;
            case MEM_TRANSACTION_LOCK:
            {
                if(UNLIKELY(needs_split)) {
                    output->verbose(CALL_INFO, 16, 0, "---> load is marked LOCK but it requires a cache line split, this generates an error\n");
                    load_ins->flagError();
                } else {
                    output->verbose(CALL_INFO, 16, 0, "---> [memory-transaction]: LOCK-load (not split) load-at: 0x%llx width: %" PRIu64 "\n",
                        load_address, load_width);
                    load_req = new StandardMem::ReadLock(load_address & address_mask, load_width, 0,
                                        load_address, load_ins->getInstructionAddress(), load_ins->getHWThread());
                }
            } break;
        }

        // if the instruction does not trap an error we will continue to process it
        if(LIKELY(! load_ins->trapsError())) {
            output->verbose(CALL_INFO, 16, 0, "-----> ins: 0x%llx / thr: %" PRIu32 " processed and requests sent to memory system.\n",
                load_ins->getInstructionAddress(), load_ins->getHWThread());

            assert(load_req != nullptr);

            load_entry->addRequest(load_req->getID());
            memInterface->send(load_req);

            loads_pending.push_back(load_entry);
        }
    }

    bool attempt_to_issue(uint64_t cycle, uint16_t attempt_this_cycle) {
        // if we don't have any work to do, return and get out of here
        if(0 == op_q.size()) {
            return false;
        }

        output->verbose(CALL_INFO, 16, 0, "-> cycle: %" PRIu64 " / attempt: %" PRIu16 "\n", cycle, attempt_this_cycle);

        VanadisBasicLoadStoreEntry* front_entry = op_q.front();

        if(! front_entry->getInstruction()->completedIssue()) {
            if(output->getVerboseLevel() >= 16) {
                output->verbose(CALL_INFO, 16, 0, "--> ins: 0x%llx / thr: %" PRIu32 " has not completed issue, will not process this cycle.\n",
                    front_entry->getInstruction()->getInstructionAddress(), front_entry->getInstruction()->getHWThread());
            }
            return false;
        }

        switch(front_entry->getEntryOp()) {
            case VanadisBasicLoadStoreEntryOp::LOAD:
            {
                VanadisLoadInstruction* load_ins = dynamic_cast<VanadisLoadInstruction*>(
                    front_entry->getInstruction());

                if(UNLIKELY(load_ins == nullptr)) {
                    output->fatal(CALL_INFO, -1, "Error: attempted to convert a load entry to a load instruction but failed, ins: 0x%llx / thr: %" PRIu32 "\n",
                        front_entry->getInstructionAddress(), front_entry->getHWThread());
                }
    
                // can't do anything cycle, so return false
                if(loads_pending.size() >= max_loads) {
                    return false;
                }

                if(output->getVerboseLevel() >= 16) {
                    output->verbose(CALL_INFO, 16, 0, "-> queue front is load: ins: 0x%llx / thr: %" PRIu32 " has issued so will process...\n", 
                        load_ins->getInstructionAddress(), load_ins->getHWThread());
                }

                VanadisRegisterFile* hw_thr_reg = registerFiles->at(load_ins->getHWThread());

                uint64_t load_address = 0;
                uint16_t load_width   = 0;

                load_ins->computeLoadAddress(output, hw_thr_reg, &load_address, &load_width);

                if(UNLIKELY(load_ins->trapsError())) {
                    if(output->getVerboseLevel() >= 16) {
                        output->verbose(CALL_INFO, 16, 0, "---> load ins: 0x%llx / thr: %" PRIu32 " traps error, will not process and allow pipeline to handle.\n",
                            load_ins->getInstructionAddress(), load_ins->getHWThread());
                    }
                } else {
                    if(output->getVerboseLevel() >= 16) {
                        output->verbose(CALL_INFO, 16, 0, "---> load ins: 0x%llx / thr: %" PRIu32 " want load at 0x%llx / width: %" PRIu16 "\n",
                            load_ins->getInstructionAddress(), load_ins->getHWThread(), load_address, load_width);
                    }
                    
                    // check to see if loading from this address would conflict with a store which
                    // we have pending, if yes, wait for conflict to clear and then we can proceed
                    if(UNLIKELY(checkStoreConflict(load_ins->getHWThread(), load_address, load_width))) {
                        if(output->getVerboseLevel() >= 16) {
                            output->verbose(CALL_INFO, 16, 0, "---> load ins: 0x%llx / thr: %" PRIu32 " conflicts with store entry, will not issue until conflict is resolved (load-addr: 0x%llx / width: %" PRIu32 ")\n",
                                load_ins->getInstructionAddress(), load_ins->getHWThread(), load_address, load_width);
                        }

                        // tell caller we would not issue
                        return false;
                    } else {
                        // We are good to issue with all checks completed!
                        issueLoad(load_ins, load_address, load_width);
                    }
                }

                // pop front entry and tell the caller we did something (true)
                op_q.pop_front();
                return true;
            } break;
            case VanadisBasicLoadStoreEntryOp::STORE:
            {
                VanadisStoreInstruction* store_ins = dynamic_cast<VanadisStoreInstruction*>(
                    front_entry->getInstruction());

                if(UNLIKELY(store_ins == nullptr)) {
                    output->fatal(CALL_INFO, -1, "Error: attempted to convert a store entry into store instruction but this failed (ins: 0x%llx, thr: %" PRIu32 ")\n",
                        front_entry->getInstructionAddress(), front_entry->getHWThread());
                }

                // if we have too many operations pending, return false to tell handler
                // we couldn't perform any operations this cycle
                if(stores_pending.size() >= max_stores) {
                    return false;
                }

                if(output->getVerboseLevel() >= 16) {
                    output->verbose(CALL_INFO, 16, 0, "-> queue front is store: ins: 0x%llx / thr: %" PRIu32 " has issued so will process...\n", 
                        store_ins->getInstructionAddress(), store_ins->getHWThread());
                }
                
                VanadisRegisterFile* hw_thr_reg = registerFiles->at(store_ins->getHWThread());

                uint64_t store_address = 0;
                uint16_t store_width   = 0;

                store_ins->computeStoreAddress(output, hw_thr_reg, &store_address, &store_width);
                
                if(store_ins->trapsError()) {
                    output->verbose(CALL_INFO, 16, 0, "----> warning: 0x%llx / thr: %" PRIu32 " traps error, marks executed and does not process.\n",
                        store_ins->getInstructionAddress(), store_ins->getHWThread());
                    store_ins->markExecuted();
                } else {
                    if(output->getVerboseLevel() >= 16) {
                        output->verbose(CALL_INFO, 16, 0, "----> computed store address: 0x%llx width: %" PRIu16 " thr: %" PRIu32 "\n",
                            store_address, store_width, store_ins->getHWThread());
                    }

                    VanadisBasicStorePendingEntry* new_pending_store = new VanadisBasicStorePendingEntry(
                        store_ins, store_address, store_width, store_ins->getValueRegisterType(),
                        store_ins->getValueRegister());

                    stores_pending.push_back(new_pending_store);
                }

                // clear the front entry as we have just processed it
                op_q.pop_front();
                return true;
            } break;
            case VanadisBasicLoadStoreEntryOp::FENCE: 
            {
                VanadisInstruction* current_ins = op_q.front()->getInstruction();
                VanadisFenceInstruction* current_fence_ins = (VanadisFenceInstruction*) current_ins;

                bool can_execute = true;

                // fence instruction can be executed IF there are no pending loads IF it fences loads
                // AND there are no pending stores IF if fences stores
                if(current_fence_ins->createsLoadFence()) {
                    // if no pending loads, then we are good to go
                    can_execute = (!pendingLoads(current_fence_ins->getHWThread()));
                }
                
                if(current_fence_ins->createsStoreFence()) {
                    // stores are fenced if there are no pending stores AND all issued to the memory system
                    // have returned so are currently visible.
                    can_execute = can_execute && (!pendingStores(current_fence_ins->getHWThread())) && 
                        (std_stores_in_flight.size() == 0);
                }

                if(can_execute) {
                    if(output->getVerboseLevel() >= 16) {
                        output->verbose(CALL_INFO, 16, 0, "-> execute fence instruction (0x%llx), all checks have passed.\n", 
                            current_fence_ins->getInstructionAddress());
                    }
                    current_fence_ins->markExecuted();
                    stat_fences_executed->addData(1);

                    // erase the front entry
                    op_q.pop_front();
                    return true;
                } else {
                    return false;
                }
            } break;
        }

        // default is do not call me again
        return false;
    }

    bool operationStraddlesCacheLine(uint64_t address, uint64_t width) const {
        const uint64_t cache_line_left  = (address / cache_line_width);
        const uint64_t cache_line_right = ((address + width - 1) / cache_line_width);

        const bool splits_line = cache_line_left != cache_line_right;

        if(output->getVerboseLevel() >= 16) {
            output->verbose(CALL_INFO, 16, 0, "---> check split addr: %" PRIu64 " (0x%llx) / width: %" PRIu64 " / cache-line: %" PRIu64 " / line-left: %" PRIu64 " / line-right: %" PRIu64 " / split: %3s\n",
                address, address, width, cache_line_width, cache_line_left, cache_line_right, splits_line ? "yes" : "no");
        }

        return splits_line;
    }

    void copyPayload(std::vector<uint8_t>& buffer, uint8_t* reg, uint16_t offset, uint16_t length) const {
        for(uint16_t i = 0; i < length; ++i) {
            buffer.push_back(reg[offset + i]);
        }
    }

    bool pendingStores(const uint32_t thr) {
        bool matchID = false;

        for(auto store_itr = stores_pending.begin(); store_itr != stores_pending.end(); store_itr++) {
            if(thr == (*store_itr)->getHWThread()) {
                matchID = true;
                break;
            }
        }

        return matchID;
    }

    bool pendingLoads(const uint32_t thr) {
        bool matchID = false;

        for(auto load_itr = loads_pending.begin(); load_itr != loads_pending.end(); load_itr++) {
            if((*load_itr)->getHWThread() == thr) {
                matchID = true;
                break;
            }
        }

        return matchID;
    }

    bool checkStoreConflict(const uint32_t thread, const uint64_t address, const uint64_t width) {
        bool conflicts = false;

        for(auto store_itr = stores_pending.begin(); store_itr != stores_pending.end(); store_itr++) {
            VanadisBasicStorePendingEntry* current_entry = (*store_itr);

            if(thread == current_entry->getHWThread()) {
                if(UNLIKELY(current_entry->storeAddressOverlaps(address, width))) {
                    conflicts = true;
                    break;
                }
            }
        }

        return conflicts;
    }

    std::deque<VanadisBasicLoadStoreEntry*> op_q;
    std::deque<VanadisBasicStorePendingEntry*> stores_pending;
    std::deque<VanadisBasicLoadPendingEntry*> loads_pending;
    std::unordered_set<StandardMem::Request::id_t> std_stores_in_flight;

    StandardMem* memInterface;
    StandardMemHandlers* std_mem_handlers;

    const size_t max_stores;
    const size_t max_loads;

    const uint32_t max_issue_attempts_per_cycle;

    uint64_t cache_line_width;
    uint64_t address_mask;

    Statistic<uint64_t>* stat_store_buffer_entries;
    Statistic<uint64_t>* stat_op_q_size;
    Statistic<uint64_t>* stat_stores_pending;
    Statistic<uint64_t>* stat_loads_pending;
    Statistic<uint64_t>* stat_stores_issued;
    Statistic<uint64_t>* stat_loads_issued;
    Statistic<uint64_t>* stat_fences_issued;
    Statistic<uint64_t>* stat_stores_executed;
    Statistic<uint64_t>* stat_loads_executed;
    Statistic<uint64_t>* stat_fences_executed;
    Statistic<uint64_t>* stat_split_stores;
    Statistic<uint64_t>* stat_split_loads;
    Statistic<uint64_t>* stat_stored_bytes;
    Statistic<uint64_t>* stat_loaded_bytes;
};

} // namespace Vanadis
} // namespace SST

#endif
