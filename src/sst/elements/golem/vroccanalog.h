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

#ifndef _H_VANADIS_ANALOG_ROCC
#define _H_VANADIS_ANALOG_ROCC

#include <sst/core/output.h>
#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/elements/golem/computeArray.h>
#include <sst/elements/golem/arrayEvent.h>
#include <sst/elements/vanadis/rocc/vroccinterface.h>

#include <cinttypes>
#include <cstdint>
#include <vector>
#include <queue>
#include <limits>

#define VANADIS_RISCV_FUNC7_MASK  0xFE000000

using namespace SST::Interfaces;
using namespace SST::Golem;

namespace SST {
namespace Golem {

class VanadisRoCCAnalog : public SST::Vanadis::VanadisRoCCInterface {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(VanadisRoCCAnalog, "golem", "VanadisRoCCAnalog",
                                          SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                          "Implements a RoCC accelerator interface for the analog core",
                                          SST::Vanadis::VanadisRoCCInterface)

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS({ "memory_interface", "Set the interface to memory", "SST::Interfaces::StandardMem" },
                                        { "array", "Analog array model", "SST::Golem::ComputeArray"})

    SST_ELI_DOCUMENT_PORTS({ "dcache_link", "Connects the RoCC frontend to the data cache", {} })

    SST_ELI_DOCUMENT_PARAMS(
            { "verbose", "Set the verbosity of output for the RoCC", "0" }, 
            { "max_instructions", "Set the maximum number of RoCC instructions permitted in the queue", "8" },
            { "clock",              "Clock frequency for component TimeConverter. MMIOTile is Unclocked but subcomponents use the TimeConverter", "1Ghz"},
            { "mmioAddr" ,          "Address MMIO interface"},
            { "numArrays",          "Number of distinct arrays in the the tile.", "1"},
            { "arrayInputSize",     "Length of input vector. Implies array rows."},
            { "arrayOutputSize",    "Length of output vector. Implies array columns."},
            { "inputOperandSize",   "Size of input operand in bytes.", "4"},
            { "outputOperandSize",  "Size of output operand in bytes.", "4"}
        )

    SST_ELI_DOCUMENT_STATISTICS({ "roccs_issued", "Count number of rocc instructions that are issued", "operations", 1 })

    VanadisRoCCAnalog(ComponentId_t id, Params& params) : VanadisRoCCInterface(id, params),

        max_instructions(params.find<size_t>("max_instructions", 8)) {

        stat_roccs_issued = registerStatistic<uint64_t>("roccs_issued", "1");

        UnitAlgebra clock = params.find<UnitAlgebra>("clock", "1GHz");
        
        mmioStartAddr = params.find<uint64_t>("mmioAddr", 0);
        arrayInputSize = params.find<uint64_t>("arrayInputSize", 3);
        arrayOutputSize = params.find<uint64_t>("arrayOutputSize", 3);

        numArrays = params.find<uint64_t>("numArrays", 1);
        inputOperandSize = params.find<uint64_t>("inputOperandSize", 4);
        outputOperandSize = params.find<uint64_t>("outputOperandSize", 4);

        output->verbose(CALL_INFO, 1, 0, "%s: numArrays: %d, arrayInputSize: %d, arrayOutputSize: %d \n",
            getName().c_str(), numArrays, arrayInputSize, arrayOutputSize);

        std_mem_handlers = new VanadisRoCCAnalog::StandardMemHandlers(this, output);

        busy = false;

        memInterface = loadUserSubComponent<Interfaces::StandardMem>(
            "memory_interface", ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, getTimeConverter("1ps"),
            new StandardMem::Handler<VanadisRoCCAnalog>(
                this, &VanadisRoCCAnalog::processIncomingDataCacheEvent));

        array = loadUserSubComponent<Golem::ComputeArray>(
            "array", ComponentInfo::SHARE_NONE, getTimeConverter("1ps"), 
            new SST::Event::Handler<VanadisRoCCAnalog>(
                this, &VanadisRoCCAnalog::handleArrayEvent), &arrayIns, &arrayOuts, &matrices);

        if ( !array ) {
            output->fatal(CALL_INFO, -1, "Unable to load array model subcomponent.\n");
        }
    }

    virtual ~VanadisRoCCAnalog() {
        for(auto roccCmd_q_itr = roccCmd_q.begin(); roccCmd_q_itr != roccCmd_q.end(); ) {
            delete (*roccCmd_q_itr);
            roccCmd_q_itr = roccCmd_q.erase(roccCmd_q_itr);
        }

        delete std_mem_handlers;
    }

    bool RoCCFull() override { return roccCmd_q.size() >= max_instructions; }

    bool isBusy() override { return busy; }

    size_t roccQueueSize() override { return roccCmd_q.size(); }

    void push(SST::Vanadis::RoCCCommand* rocc_me) override {
        stat_roccs_issued->addData(1);
        roccCmd_q.push_back( rocc_me );
    }

    SST::Vanadis::RoCCResponse* respond() override {
        SST::Vanadis::RoCCResponse* temp = curr_resp;
        curr_resp = NULL;
        return temp;
    }

    // initialize subcomponents and parameterizable data structures
    void init(unsigned int phase) override { 

        // Set the sizes of the array interface vectors
        arrayIns.resize(numArrays);
        arrayOuts.resize(numArrays);
	matrices.resize(numArrays);
        arrayStates.resize(numArrays);
        for(int i = 0; i < numArrays; i++) {
            arrayIns[i].resize(arrayInputSize);
            arrayOuts[i].resize(arrayOutputSize);
    	    matrices[i].resize(arrayInputSize * arrayOutputSize);
        }

        // Set the address delimiters
        inputDataSize = inputOperandSize * arrayInputSize;
        inputTotalSize = inputDataSize * numArrays;
        outputDataSize = outputOperandSize * arrayOutputSize;
        outputTotalSize = outputDataSize * numArrays;
        inputStartAddr = mmioStartAddr + numArrays;
        outputStartAddr = inputStartAddr + inputTotalSize;

        for (int i = 0; i < numArrays; i++) {
            arrayStates[i] = 0;

	    // Resize the matrix to fit the data
	    for (int j = 0; j < arrayInputSize * arrayOutputSize; j++) {
		    matrices[i][j] = 0;
	    }

            for (int j = 0; j < arrayInputSize; j++){
                arrayIns[i][j] = 0;
            }
            for (int j = 0; j < arrayOutputSize; j++){
                arrayOuts[i][j] = 0;
            }
        }

        memInterface->init(phase); 
        array->init(phase);
    }

    // main clock cycle tick function
    // checks if there is any command to be processed
    // if so, and as long as there is nothing else already being processed:
    // decode the command's func7 field to determine function to be performed
    // Also, unconditionally tick the all-in-one state machine 
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

            switch (curr_cmd->inst->func7) { 
                case 0x1: // Set Matrix
                {
                    output->verbose(CALL_INFO, 2, 0, "Instruction read: mvm.set (MVM set matrix)\n");
		    setMatrix();
                } break;
                case 0x2: // Load Vector
                {
                    output->verbose(CALL_INFO, 2, 0, "Instruction read: mvm.l (MVM load vector)\n");
		    loadVector();
                } break;
                case 0x3: // Compute MVM
                {
                    output->verbose(CALL_INFO, 2, 0, "Instruction read: mvm (MVM compute)\n");
		    computeMVM();
                } break;
                case 0x4: // Store Vector
                {
                    output->verbose(CALL_INFO, 2, 0, "Instruction read: mvm.s (MVM store vector)\n");
		    storeVector();
                } break;
                case 0x5: // Move Vector
                {
                    output->verbose(CALL_INFO, 2, 0, "Instruction read: mvm.mv (MVM move vector)\n");
		    moveVector();
                } break;
                default: 
                {
                    output->verbose(CALL_INFO, 2, 0, "ERROR: unrecognized RoCC func7\n");
                } break;
            }
        }
    }

    // issues the read request for the matrix that will be set in the analog array
    // setting the matrix into compute array happens in read request response handler
    void setMatrix() {
        StandardMem::Request* load_req = nullptr;

        uint32_t load_matrix_flag = 0x0;
	uint64_t matrix_size = arrayInputSize * arrayInputSize * inputOperandSize;
//        load_req = new StandardMem::Read(curr_cmd->rs1, matrix_size, load_matrix_flag, curr_cmd->rs1, curr_cmd->inst->ins_address, curr_cmd->inst->hw_thr);
        load_req = new StandardMem::Read(curr_cmd->rs1, matrix_size, load_matrix_flag, curr_cmd->rs1, 0, 0);
        output->verbose(CALL_INFO, 9, 0, "----> Read Req: physAddr: %llx, size: %llx, vAddr: %llx, inst ptr: %llx, tid: %llx\n", 
                        curr_cmd->rs1, matrix_size, curr_cmd->rs1, 0, 0);
//                        curr_cmd->rs1, matrix_size, curr_cmd->rs1, curr_cmd->inst->ins_address, curr_cmd->inst->hw_thr);

        assert(load_req != nullptr);
        memInterface->send(load_req);
    }


    void loadVector() {
        StandardMem::Request* load_req = nullptr;

	uint32_t load_vector_flag = 0x1;
        uint64_t vector_size = arrayInputSize * inputOperandSize;
//        load_req = new StandardMem::Read(curr_cmd->rs1, vector_size, load_vector_flag, curr_cmd->rs1, curr_cmd->inst->ins_address, curr_cmd->inst->hw_thr);
        load_req = new StandardMem::Read(curr_cmd->rs1, vector_size, load_vector_flag, curr_cmd->rs1, 0, 0);
        output->verbose(CALL_INFO, 9, 0, "----> Read Req: physAddr: %llx, size: %llx, vAddr: %llx, inst ptr: %llx, tid: %llx\n", 
                        curr_cmd->rs1, vector_size, curr_cmd->rs1, 0, 0);
//                        curr_cmd->rs1, vector_size, curr_cmd->rs1, curr_cmd->inst->ins_address, curr_cmd->inst->hw_thr);

        assert(load_req != nullptr);
        memInterface->send(load_req);
    }

    void computeMVM() {
        uint64_t rs1 = curr_cmd->rs1;
        arrayStates[rs1] = 1;
	array->beginComputation(rs1);
    }

    void storeVector() {
        StandardMem::Request* store_req = nullptr;

        uint64_t vector_size = arrayOutputSize * outputOperandSize;
        std::vector<uint8_t> payload(vector_size);	
	uint64_t rs2 = curr_cmd->rs2;

        for (int i = 0; i < arrayOutputSize; i++) {
                unsigned int ch = *reinterpret_cast<unsigned int*>(&arrayOuts[rs2][i]);
                for (int j = 0; j < outputOperandSize; j++) {
                        payload.at(i*outputOperandSize + j) = (ch >> j*8) & 0xff;
                }
        }

	std::cout << "Stored array " << rs2 << ":" << std::endl;
	for (int i = 0; i < arrayOutputSize; i++) {
	   std::cout << arrayOuts[rs2][i] << " ";
	}
	std::cout << std::endl;
	std::cout << std::endl;

        store_req = new StandardMem::Write(curr_cmd->rs1, 4, payload,
            false, 0, curr_cmd->rs1, 0, 0);
        memInterface->send(store_req);
    }

    void moveVector() {
        uint64_t rs1 = curr_cmd->rs1;
        uint64_t rs2 = curr_cmd->rs2;
        for (int i = 0; i < arrayInputSize; i++){
            arrayIns[rs2][i] = arrayOuts[rs1][i];
        }

	std::cout << "Moved array " << rs1 << " to array " << rs2 << ". Array " << rs2 << ":" << std::endl;
        for (int i = 0; i < arrayInputSize; i++){
            std::cout << arrayIns[rs2][i] << " "; 
        }
	std::cout << std::endl;
	std::cout << std::endl;

	completeRoCC(0);
    }
 

    // finalizes the execution of a RoCC instruction by:
    // writing any status data to rd
    // mark the instruction executed so main CPU pipeline knows it is finished
    // pop it out of the command queue so it will no longer be processed 
    // reset state of this analog RoCC frontend in case all-in-one just finished
    // turn off busy bit so host knows this accelerator is no longer busy
    void completeRoCC(uint64_t rd_val) {
        output->verbose(CALL_INFO, 2, 0, "Finalize RoCC command w/ rd %" PRIu16 ", rd_val %" PRIu64 " \n", curr_cmd->inst->rd, rd_val);
        roccCmd_q.pop_front();
        busy = false;
        curr_resp = new SST::Vanadis::RoCCResponse(curr_cmd->inst->rd, rd_val);
    }

    void handleArrayEvent(Event *ev) {
        Golem::ArrayEvent* aev = static_cast<Golem::ArrayEvent*>(ev);
        uint32_t arrayID = aev->getArrayID();
//        output->verbose(CALL_INFO, 1, 0, "%s: Array %d completed array operation\n", getName().c_str(), arrayID);
	std::cout << std::endl;
        arrayStates[arrayID] = 0;
	completeRoCC(0);
    }

    class StandardMemHandlers : public Interfaces::StandardMem::RequestHandler {
    public:
        friend class VanadisRoCCAnalog;

        StandardMemHandlers(VanadisRoCCAnalog* rocc, SST::Output* output) :
                Interfaces::StandardMem::RequestHandler(output), rocc(rocc) {output = output;}
        
        virtual ~StandardMemHandlers() {}

        virtual void handle(StandardMem::ReadResp* ev) {
            out->verbose(CALL_INFO, 2, 0, "-> handle read-response (virt-addr: 0x%llx)\n", ev->vAddr);
            SST::Vanadis::RoCCCommand* rocc_cmd = rocc->curr_cmd; // need to grab the instruction that generated the read request

            if ( ev->getFail() ) { 
                out->verbose(CALL_INFO, 2, 0, "RoCC load failed\n");
                rocc->completeRoCC(1);
		return;
            }

            uint64_t reg_offset  = 0;
            uint64_t addr_offset = 0;            
            uint64_t reg_width = 8;

            std::vector<uint8_t> register_value(reg_width);
            for (auto i = 0; i < reg_width; ++i) {
                register_value.at(reg_offset + addr_offset + i) = ev->data[i];
            }

            uint64_t rs2 = rocc_cmd->rs2;
            switch (ev->getAllFlags()) { // treat read response data differently based on who issued it (flags)

                case 0x0: // read response data is matrix to be set
                {
                    rocc->output->verbose(CALL_INFO, 2, 0, "Set matrix read response detected\n");

                    uint32_t op_size = rocc->inputOperandSize;
		    uint32_t num_cols = rocc->arrayInputSize;
                    uint32_t num_rows = ev->size / (num_cols * op_size); // compute number of rows in matrix
		    unsigned char* matrix_data = ev->data.data();

		    rocc->array->setMatrix(matrix_data, rs2, num_rows, num_cols, op_size);

                    rocc->completeRoCC(0);
                } break;

                case 0x1: // read response data  is input vector
                {
                    rocc->output->verbose(CALL_INFO, 2, 0, "Input vector read response detected\n");

		    uint32_t op_size = rocc->inputOperandSize;
		    uint32_t num_elem = rocc->arrayInputSize;
		    unsigned char* vector_data = ev->data.data();

		    rocc->array->setInputVector(vector_data, rs2, num_elem, op_size);

                    rocc->completeRoCC(0);
                } break;

                default:
                {
                    rocc->output->verbose(CALL_INFO, 2, 0, "ERROR: unrecognized read response flag\n");
                } break;
            }
        } 
        
        virtual void handle(StandardMem::WriteResp* ev) {
            // write is much simpler because we aren't handling any reponse data
            // just need to make sure it went through properly
            out->verbose(CALL_INFO, 2, 0, "-> handle write-response (virt-addr: 0x%llx)\n", ev->vAddr);
            if ( ev->getFail() ) { 
                out->verbose(CALL_INFO, 2, 0, "RoCC store failed, responding with error code 1\n");
                rocc->completeRoCC(1);
            }
            
            delete ev;
            rocc->completeRoCC(0);
        }
    
        VanadisRoCCAnalog* rocc;
        SST::Output* output;
    };

    void processIncomingDataCacheEvent(StandardMem::Request* ev) {
        output->verbose(CALL_INFO, 2, 0, "received incoming data cache request -> processIncomingDataCacheEvent()\n");

        assert(ev != nullptr);
        assert(std_mem_handlers != nullptr);

        ev->handle(std_mem_handlers);
        output->verbose(CALL_INFO, 2, 0, "completed pass off to incoming handlers\n");
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

    std::deque<SST::Vanadis::RoCCCommand*> roccCmd_q; // queue of RoCC commands issued from CPU
    bool busy; // busy line from analog accelerator that keeps this interface from sending more commands
    SST::Vanadis::RoCCCommand* curr_cmd;
    SST::Vanadis::RoCCResponse* curr_resp;

    StandardMemHandlers* std_mem_handlers;
    StandardMem* memInterface;

    int max_instructions;

    Statistic<uint64_t>* stat_roccs_issued;

    Golem::ComputeArray* array;
    std::vector<std::vector<float>> arrayIns;
    std::vector<std::vector<float>> arrayOuts;
    std::vector<std::vector<float>> matrices;
    float* crossSim_output;
    std::vector<char> arrayStates;

    // Tile Parameters
    int numArrays;
    int arrayInputSize;
    int arrayOutputSize;
    int inputOperandSize;
    int outputOperandSize;

    // MMIO range delimiters
    uint64_t mmioStartAddr;
    uint64_t inputStartAddr;
    uint64_t outputStartAddr;
    uint64_t inputDataSize;
    uint64_t outputDataSize;
    uint64_t inputTotalSize;
    uint64_t outputTotalSize;

};

} // namespace Golem
} // namespace SST

#endif
