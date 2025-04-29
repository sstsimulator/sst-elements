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

#ifndef _H_ANALOG_ROCC
#define _H_ANALOG_ROCC

#include <sst/core/output.h>
#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/elements/golem/array/computeArray.h>
#include <sst/elements/vanadis/rocc/vroccinterface.h>

#include <cinttypes>
#include <cstdint>
#include <limits>
#include <queue>
#include <vector>
#include <iostream>

using namespace SST::Interfaces;
using namespace SST::Golem;

namespace SST {
namespace Golem {

template <typename T>
class RoCCAnalog : public SST::Vanadis::VanadisRoCCInterface {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(RoCCAnalog<T>, SST::Vanadis::VanadisRoCCInterface)
  
    RoCCAnalog(ComponentId_t id, Params &params)
        : VanadisRoCCInterface(id, params),
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
  
        mmioStartAddr = params.find<uint64_t>("mmioAddr", 0);
        arrayInputSize = params.find<uint64_t>("arrayInputSize", 2);
        arrayOutputSize = params.find<uint64_t>("arrayOutputSize", 2);
  
        numArrays = params.find<uint64_t>("numArrays", 1);
        inputOperandSize = params.find<uint64_t>("inputOperandSize", 4);
        outputOperandSize = params.find<uint64_t>("outputOperandSize", 4);
  
        output->verbose(
            CALL_INFO, 1, 0,
            "%s: numArrays: %d, arrayInputSize: %d, arrayOutputSize: %d \n",
            getName().c_str(), numArrays, arrayInputSize, arrayOutputSize);
  
        std_mem_handlers = new StandardMemHandlers(this, output);
  
        busy = false;
        curr_resp = nullptr;
    
        memInterface = loadUserSubComponent<Interfaces::StandardMem>(
            "memory_interface",
            ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS,
            getTimeConverter("1ps"),
            new StandardMem::Handler2<RoCCAnalog<T>, &RoCCAnalog<T>::processIncomingDataCacheEvent>(this));

        if ( nullptr == memInterface ) {
            output->fatal(
                CALL_INFO, -1,
                "Error: unable to load memory interface subcomponent for RoCCAnalog.\n");
        }

        array = loadUserSubComponent<Golem::ComputeArray>(
            "array", ComponentInfo::SHARE_NONE, getTimeConverter("1ps"),
            new SST::Event::Handler2<RoCCAnalog<T>, &RoCCAnalog<T>::handleArrayEvent>(this));

        if ( nullptr == array ) {
            output->fatal(
                CALL_INFO, -1,
                "Error: Unable to load array model subcomponent for RoCCAnalog.\n");
        }
    }
  
    virtual ~RoCCAnalog() {
        for (auto roccCmd_q_itr = roccCmd_q.begin(); roccCmd_q_itr != roccCmd_q.end();) {
            delete (*roccCmd_q_itr);
            roccCmd_q_itr = roccCmd_q.erase(roccCmd_q_itr);
        }

        delete std_mem_handlers;
        delete memInterface;
        delete array;
    }
  
    bool RoCCFull() override { return roccCmd_q.size() >= max_instructions; }
  
    bool isBusy() override { return busy; }
  
    size_t roccQueueSize() override { return roccCmd_q.size(); }
  
    void push(SST::Vanadis::RoCCCommand *rocc_me) override {
        stat_rocc_issued->addData(1);
        roccCmd_q.push_back(rocc_me);
    }
  
    SST::Vanadis::RoCCResponse *respond() override {
        SST::Vanadis::RoCCResponse *temp = curr_resp;
        curr_resp = nullptr;
        return temp;
    }
  
    // Initialize subcomponents and parameterizable data structures
    void init(unsigned int phase) override {
  
        // Initialize arrayStates
        arrayStates.resize(numArrays);
  
        // Set the address delimiters
        inputDataSize = inputOperandSize * arrayInputSize;
        inputTotalSize = inputDataSize * numArrays;
        outputDataSize = outputOperandSize * arrayOutputSize;
        outputTotalSize = outputDataSize * numArrays;
        inputStartAddr = mmioStartAddr + numArrays;
        outputStartAddr = inputStartAddr + inputTotalSize;
  
        for (int i = 0; i < numArrays; i++) {
            arrayStates[i] = 0;
        }
  
        memInterface->setMemoryMappedAddressRegion(mmioStartAddr, inputTotalSize);
        memInterface->init(phase);
        array->init(phase);
    }
  
    // Main clock cycle tick function
    void tick(uint64_t cycle) override {
        output->verbose(CALL_INFO, 16, 0, "-> tick RoCC at cycle %" PRIu64 "\n", cycle);
        if (roccCmd_q.empty()) {
            output->verbose(CALL_INFO, 16, 0, "--> nothing to do in RoCC\n");
            return;
        }
        output->verbose(CALL_INFO, 16, 0, "busy? %d\n", busy);
  
        if (!busy) {
            busy = true;
            curr_cmd = roccCmd_q.front();
  
            switch (curr_cmd->inst->func7) {
                case 0x1: // Set Matrix
                {
                    output->verbose(CALL_INFO, 9, 0,
                              "Instruction read: mvm.set (MVM set matrix)\n");
                    setMatrix();
                } break;
                case 0x2: // Load Vector
                {
                    output->verbose(CALL_INFO, 9, 0,
                              "Instruction read: mvm.l (MVM load vector)\n");
                    loadVector();
                } break;
                case 0x3: // Compute MVM
                {
                    output->verbose(CALL_INFO, 9, 0,
                              "Instruction read: mvm (MVM compute)\n");
                    computeMVM();
                } break;
                case 0x4: // Store Vector
                {
                    output->verbose(CALL_INFO, 9, 0,
                              "Instruction read: mvm.s (MVM store vector)\n");
                    storeVector();
                } break;
                case 0x5: // Move Vector
                {
                    output->verbose(CALL_INFO, 9, 0,
                              "Instruction read: mvm.mv (MVM move vector)\n");
                    moveVector();
                } break;
                default: {
                    output->verbose(CALL_INFO, 9, 0, "ERROR: unrecognized RoCC func7\n");
                    completeRoCC(1);
                } break;
            }
        }
    }
  
    // Issues the read request for the matrix that will be set in the analog array
    void setMatrix() {
        uint64_t rs1 = curr_cmd->rs1;
        uint32_t load_matrix_flag = 0x0;
        matrix_total_size = arrayInputSize * arrayOutputSize * inputOperandSize;
        uint64_t cache_line_size = memInterface->getLineSize();

        matrix_read_offset = 0;

        uint64_t physAddr = rs1; // Assuming rs1 is physical address
        uint64_t addr_offset = physAddr % cache_line_size;

        // Calculate initial request size
        uint32_t request_size = std::min(static_cast<uint64_t>(cache_line_size - addr_offset), matrix_total_size);

        // Send first cache request
        auto *load_req = new StandardMem::Read(physAddr, request_size, load_matrix_flag);
        memInterface->send(load_req);
    }
  
    void loadVector() {
        uint64_t rs1 = curr_cmd->rs1;
        uint32_t load_vector_flag = 0x1;
        vector_total_size = arrayInputSize * inputOperandSize;
        uint64_t cache_line_size = memInterface->getLineSize();

        vector_read_offset = 0;

        uint64_t physAddr = rs1; // Assuming rs1 is physical address
        uint64_t addr_offset = physAddr % cache_line_size;

        // Calculate initial request size
        uint32_t request_size = std::min(static_cast<uint64_t>(cache_line_size - addr_offset), vector_total_size);

        // Send first cache request
        auto *load_req = new StandardMem::Read(physAddr, request_size, load_vector_flag);
        memInterface->send(load_req);
    }
  
    void computeMVM() {
        uint64_t rs1 = curr_cmd->rs1;
        arrayStates[rs1] = 1;
        array->beginComputation(static_cast<uint32_t>(rs1));
    }
  
    void storeVector() {
        uint64_t rs1 = curr_cmd->rs1; // Destination address (physical)
        uint64_t rs2 = curr_cmd->rs2; // Array ID or source vector index
        vector_total_size = arrayOutputSize * outputOperandSize;
        uint64_t cache_line_size = memInterface->getLineSize();

        write_offset = 0;
        uint64_t physAddr = rs1; // Assuming rs1 is physical address
        uint64_t addr_offset = physAddr % cache_line_size;

        // Resize the output payload to hold the entire vector
        outputPayload.resize(vector_total_size);

        // Reference to the output vector we need to store
        auto& outputVector = *static_cast<std::vector<T>*>(array->getOutputVector(rs2));

        // Fill the output payload with the vector data
        for (size_t i = 0; i < static_cast<size_t>(arrayOutputSize); i++) {
            T value = outputVector[i];
            uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(&value);
            for (size_t j = 0; j < static_cast<size_t>(outputOperandSize); j++) {
                outputPayload[i * outputOperandSize + j] = byte_ptr[j];
            }
        }

        // Optional: Output the stored array for debugging purposes
        output->verbose(CALL_INFO, 9, 0, "Stored array %" PRIu64 ":\n", rs2);
        for (size_t i = 0; i < static_cast<size_t>(arrayOutputSize); i++) {
            if constexpr (std::is_same<T, float>::value || std::is_same<T, double>::value) {
                output->verbose(CALL_INFO, 9, 0, "%f ", static_cast<double>(outputVector[i]));
            } else {
                output->verbose(CALL_INFO, 9, 0, "%lld ", static_cast<long long>(outputVector[i]));
            }
        }
        output->verbose(CALL_INFO, 9, 0, "\n\n");

        // Calculate the size of the first memory request
        uint32_t request_size = static_cast<uint32_t>(std::min(
            cache_line_size - addr_offset, 
            vector_total_size - write_offset
        ));

        // Prepare the first chunk of data to write
        std::vector<uint8_t> data_chunk(
            outputPayload.begin() + write_offset,
            outputPayload.begin() + write_offset + request_size
        );

        // Create a new write request to send to the memory interface
        auto* store_req = new StandardMem::Write(physAddr + write_offset, request_size, data_chunk, false, 0, rs1, 0, 0);

        // Send the write request
        memInterface->send(store_req);

        // Update the write offset for subsequent writes
        write_offset += request_size;
    }
  
    void moveVector() {
        uint64_t rs1 = curr_cmd->rs1;
        uint64_t rs2 = curr_cmd->rs2;
        array->moveOutputToInput(rs1, rs2);

        auto& inputVector = *static_cast<std::vector<T>*>(array->getInputVector(rs2));

        output->verbose(CALL_INFO, 9, 0,
                      "Moved array %" PRIu64 " to array %" PRIu64 ". Array %" PRIu64 ":\n", rs1, rs2, rs2);

        for (int i = 0; i < arrayInputSize; i++) {
            if constexpr (std::is_same<T, float>::value || std::is_same<T, double>::value) {
                output->verbose(CALL_INFO, 9, 0, "%f ", static_cast<double>(inputVector[i]));
            } else {
                output->verbose(CALL_INFO, 9, 0, "%ld ", static_cast<long>(inputVector[i]));
            }
        }
        output->verbose(CALL_INFO, 9, 0, "\n");

        completeRoCC(0);
    }
  
    void completeRoCC(uint64_t rd_val) {
        output->verbose(CALL_INFO, 9, 0,
            "Finalize RoCC command w/ rd %" PRIu16 ", rd_val %" PRIu64 " \n",
            curr_cmd->inst->rd, rd_val
        );

        roccCmd_q.pop_front();
        busy = false;
        curr_resp = new SST::Vanadis::RoCCResponse(curr_cmd->inst->rd, rd_val);
        delete curr_cmd;
        curr_cmd = nullptr;
    }
  
    void handleArrayEvent(Event *ev) {
        Golem::ArrayEvent *aev = static_cast<Golem::ArrayEvent *>(ev);
        uint32_t arrayID = aev->getArrayID();
        arrayStates[arrayID] = 0;
        completeRoCC(0);
    }
  
    class StandardMemHandlers : public Interfaces::StandardMem::RequestHandler {
    public:
        StandardMemHandlers(RoCCAnalog *rocc, SST::Output *output)
            : Interfaces::StandardMem::RequestHandler(output), rocc(rocc) {}
  
        virtual ~StandardMemHandlers() {}
  
        virtual void handle(StandardMem::ReadResp *ev) {
            out->verbose(CALL_INFO, 9, 0,
                     "-> handle read-response (virt-addr: 0x%" PRI_ADDR ")\n", ev->vAddr);
            SST::Vanadis::RoCCCommand *rocc_cmd = rocc->curr_cmd;
  
            if (ev->getFail()) {
                out->verbose(CALL_INFO, 9, 0, "RoCC load failed\n");
                rocc->completeRoCC(1);
                delete ev;
                return;
            }
  
            int32_t array_id = rocc_cmd->rs2;  // Array ID is in rs2
            switch (ev->getAllFlags()) {
                case 0x0: // Read response data is matrix to be set
                {
                    rocc->output->verbose(CALL_INFO, 9, 0,
                                "Set matrix read response detected\n");
  
                    size_t payload_size = ev->size;
                    unsigned char *payload_data = ev->data.data();

                    // Assign the received data to the matrix
                    for (size_t i = 0; i < payload_size; i += rocc->inputOperandSize) {
                        T value = 0;
                        memcpy(&value, &payload_data[i], rocc->inputOperandSize);
                        int index = (rocc->matrix_read_offset + i) / rocc->inputOperandSize;
                        rocc->array->setMatrixItem(array_id, index, value);
                    }
  
                    rocc->matrix_read_offset += payload_size;
  
                    if (rocc->matrix_read_offset < rocc->matrix_total_size) {

                        // Send the next read request
                        uint64_t cache_line_size = rocc->memInterface->getLineSize();
                        uint32_t request_size = static_cast<uint32_t>(std::min(
                        cache_line_size, rocc->matrix_total_size - rocc->matrix_read_offset));
                        uint64_t next_addr = rocc_cmd->rs1 + rocc->matrix_read_offset;
                        auto *load_req = new StandardMem::Read(next_addr, request_size, 0x0);
                        rocc->memInterface->send(load_req);
                    } else {
                        // Matrix read complete
                        rocc->completeRoCC(0);
                    }
                } break;
                
                case 0x1: // Read response data is input vector
                {
                    rocc->output->verbose(CALL_INFO, 9, 0,
                                "Input vector read response detected\n");
  
                    size_t payload_size = ev->size;
                    unsigned char *payload_data = ev->data.data();
  
                    // Assign the received data to the input vector
                    for (size_t i = 0; i < payload_size; i += rocc->inputOperandSize) {
                        T value = 0;
                        memcpy(&value, &payload_data[i], rocc->inputOperandSize);
                        int index = (rocc->vector_read_offset + i) / rocc->inputOperandSize;
                        rocc->array->setVectorItem(array_id, index, value);
                    }
  
                    rocc->vector_read_offset += payload_size;
  
                    if (rocc->vector_read_offset < rocc->vector_total_size) {

                        // Send the next read request
                        uint64_t cache_line_size = rocc->memInterface->getLineSize();
                        uint32_t request_size = static_cast<uint32_t>(std::min(
                            cache_line_size, 
                            rocc->vector_total_size - rocc->vector_read_offset
                        ));

                        uint64_t next_addr = rocc_cmd->rs1 + rocc->vector_read_offset;
                        auto *load_req = new StandardMem::Read(next_addr, request_size, 0x1);
                        rocc->memInterface->send(load_req);
                    } else {
                        
                        rocc->completeRoCC(0);
                    }
                } break;
  
                default:
                {
                    rocc->output->verbose(CALL_INFO, 9, 0,
                                "ERROR: unrecognized read response flag\n");
                    rocc->completeRoCC(1);
                } break;
            }
  
            delete ev;
        }
  
        virtual void handle(StandardMem::WriteResp *ev) {
            out->verbose(CALL_INFO, 9, 0,
                     "-> handle write-response (virt-addr: 0x%" PRI_ADDR ")\n", ev->vAddr);

            if (ev->getFail()) {
                out->verbose(CALL_INFO, 9, 0,
                       "RoCC store failed, responding with error code 1\n");
                rocc->completeRoCC(1);

            } else {
                
                // Continue sending write requests if there is remaining data
                if (rocc->write_offset < rocc->vector_total_size) {

                    // Calculate the size of the next write request
                    uint64_t cache_line_size = rocc->memInterface->getLineSize();
                    uint32_t request_size = static_cast<uint32_t>(std::min(
                        cache_line_size, 
                        rocc->vector_total_size - rocc->write_offset
                    ));
  
                    // Prepare the next chunk of data to write
                    std::vector<uint8_t> data_chunk(
                        rocc->outputPayload.begin() + rocc->write_offset,
                        rocc->outputPayload.begin() + rocc->write_offset + request_size
                    );
      
                    // Compute the next physical address to write to
                    uint64_t next_addr = rocc->curr_cmd->rs1 + rocc->write_offset;
      
                    // Create a new write request
                    auto* store_req = new StandardMem::Write(
                        next_addr, request_size, data_chunk,
                        false, 0, rocc->curr_cmd->rs1, 0, 0
                    );
      
                    // Send the write request
                    rocc->memInterface->send(store_req);
      
                    // Update the write offset
                    rocc->write_offset += request_size;
                } else {
                    // All data has been written; complete the RoCC command
                    rocc->completeRoCC(0);
                }
            }
            delete ev;
        }
  
    private:
        RoCCAnalog *rocc;
    };
  
    void processIncomingDataCacheEvent(StandardMem::Request *ev) {
        output->verbose(CALL_INFO, 9, 0,
                      "received incoming data cache request -> "
                      "processIncomingDataCacheEvent()\n");
  
        assert(ev != nullptr);
        assert(std_mem_handlers != nullptr);
  
        ev->handle(std_mem_handlers);
        output->verbose(CALL_INFO, 9, 0,
                      "completed pass off to incoming handlers\n");
    }
  
private:
    std::deque<SST::Vanadis::RoCCCommand *> roccCmd_q;
    bool busy;
    SST::Vanadis::RoCCCommand *curr_cmd;
    SST::Vanadis::RoCCResponse *curr_resp;
  
    StandardMemHandlers *std_mem_handlers;
    StandardMem *memInterface;
  
    int max_instructions;
  
    Golem::ComputeArray *array;
    std::vector<char> arrayStates;
  
    // Tile Parameters
    int numArrays;
    int arrayInputSize;
    int arrayOutputSize;
    int inputOperandSize;
    int outputOperandSize;
  
    // MMIO range delimiters
    uint64_t mmioStartAddr;
    uint64_t inputDataSize;
    uint64_t outputDataSize;
    uint64_t inputTotalSize;
    uint64_t outputTotalSize;
    uint64_t inputStartAddr;
    uint64_t outputStartAddr;
  
    // Variables to keep track of read/write request progress
    uint64_t matrix_read_offset;
    uint64_t matrix_total_size;
    uint64_t vector_read_offset;
    uint64_t vector_total_size;
    uint64_t write_offset;
    std::vector<uint8_t> outputPayload;
};
  
} // namespace Golem
} // namespace SST
  
#endif
