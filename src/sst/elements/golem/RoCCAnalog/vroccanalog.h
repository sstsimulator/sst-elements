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
#include <../golem/computeArray.h>
#include <../golem/arrayEvent.h>
#include <../golem/crossSimComputeArray.h>

#include "rocc/vroccinterface.h"
#include "inst/vrocc.h"

#include <cinttypes>
#include <cstdint>
#include <vector>
#include <queue>
#include <limits>

#define VANADIS_RISCV_FUNC7_MASK  0xFE000000

using namespace SST::Interfaces;
using namespace SST::Golem;

namespace SST {
namespace Vanadis {

enum STATE { IDLE, START, READ, WRITE, EXECUTE };
class VanadisRoCCAnalog : public SST::Vanadis::VanadisRoCCInterface {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(VanadisRoCCAnalog, "vanadis", "VanadisRoCCAnalog",
                                          SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                          "Implements a RoCC accelerator interface for the analog core",
                                          SST::Vanadis::VanadisRoCCInterface)

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS({ "memory_interface", "Set the interface to memory", "SST::Interfaces::StandardMem" },
                                        { "array", "Analog array model", "SST::analogComputeArray::ComputeArray"})

    SST_ELI_DOCUMENT_PORTS({ "dcache_link", "Connects the RoCC frontend to the data cache", {} })

    SST_ELI_DOCUMENT_PARAMS(
            { "verbose", "Set the verbosity of output for the RoCC", "0" }, 
            { "max_instructions", "Set the maximum number of RoCC instructions permitted in the queue", "8" },
            {"clock",              "Clock frequency for component TimeConverter. MMIOTile is Unclocked but subcomponents use the TimeConverter", "1Ghz"},
            {"mmioAddr" ,          "Address MMIO interface"},
            {"numArrays",          "Number of distinct arrays in the the tile.", "1"},
            {"arrayInputSize",     "Length of input vector. Implies array rows."},
            {"arrayOutputSize",    "Length of output vector. Implies array columns."},
            {"inputOperandSize",   "Size of input operand in bytes.", "1"},
            {"outputOperandSize",  "Size of output operand in bytes.", "1"}
        )

    VanadisRoCCAnalog(ComponentId_t id, Params& params) : VanadisRoCCInterface(id, params),
        max_instructions(params.find<size_t>("max_instructions", 8)) {
        
        mmioStartAddr = params.find<uint64_t>("mmioAddr", 0);
        arrayInputSize = params.find<uint64_t>("arrayInputSize", 0);
        arrayOutputSize = params.find<uint64_t>("arrayOutputSize", 0);

        UnitAlgebra clock = params.find<UnitAlgebra>("clock", "1GHz");
        numArrays = params.find<uint64_t>("numArrays", 1);
        inputOperandSize = params.find<uint64_t>("inputOperandSize", 1);
        outputOperandSize = params.find<uint64_t>("outputOperandSize", 1);

        std_mem_handlers = new VanadisRoCCAnalog::StandardMemHandlers(this, output);

        busy = 0;

        memInterface = loadUserSubComponent<Interfaces::StandardMem>(
            "memory_interface", ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, getTimeConverter("1ps"),
            new StandardMem::Handler<SST::Vanadis::VanadisRoCCAnalog>(
                this, &VanadisRoCCAnalog::processIncomingDataCacheEvent));

        array = loadUserSubComponent<Golem::ComputeArray>(
            "array", ComponentInfo::SHARE_NONE, getTimeConverter("1ps"), 
            new SST::Event::Handler<VanadisRoCCAnalog>(
                this, &VanadisRoCCAnalog::handleArrayEvent), &arrayIns, &arrayOuts);
        if ( !array ) {
            output->fatal(CALL_INFO, -1, "Unable to load array model subcomponent.\n");
        }

        arrayInputSize = array->getInputArraySize();
    }

    virtual ~VanadisRoCCAnalog() {
        for(auto analogCmd_q_itr = analogCmd_q.begin(); analogCmd_q_itr != analogCmd_q.end(); ) {
            delete (*analogCmd_q_itr);
            analogCmd_q_itr = analogCmd_q.erase(analogCmd_q_itr);
        }

        delete std_mem_handlers;
    }

    bool RoCCFull() override { return analogCmd_q.size() >= max_instructions; }

    size_t roccQueueSize() override { return analogCmd_q.size(); }

    void push(VanadisRoCCInstruction* rocc_me) override {
        analogCmd_q.push_back( rocc_me );
    }

    // initialize subcomponents and parameterizable data structures
    void init(unsigned int phase) override { 
        memInterface->init(phase); 
        array->init(phase);

        // Set the sizes of the array interface vectors
        arrayIns.resize(numArrays);
        arrayOuts.resize(numArrays);
        for(int i = 0; i < numArrays; i++) {
            arrayIns[i].resize(arrayInputSize);
            arrayOuts[i].resize(arrayOutputSize);
        }
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
            for (int j = 0; j < arrayInputSize; j++){
                arrayIns[i][j] = 0;
            }
            for (int j = 0; j < arrayOutputSize; j++){
                arrayOuts[i][j] = 0;
            }
        }
    }

    // main clock cycle tick function
    // checks if there is any command to be processed
    // if so, and as long as there is nothing else already being processed:
    // decode the command's func7 field to determine function to be performed
    // Also, unconditionally tick the all-in-one state machine 
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
                case 0x0: // basic register transfer
                {
                    output->verbose(CALL_INFO, 16, 0, "performing register transfer\n");
                    performRegisterTransfer();
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
                case 0x3: // all-in-one
                // loads input vector, passes it to analog compute array for computation, then writes result back memory
                {
                    output->verbose(CALL_INFO, 16, 0, "starting all-in-one\n");
                    state = START;
                } break;
                case 0x4: // load matrix from memory then set it in the analog compute array
                {
                    output->verbose(CALL_INFO, 16, 0, "setting matrix\n");
                    setMatrix();
                } break;
                default: 
                {
                    output->verbose(CALL_INFO, 16, 0, "ERROR: unrecognized RoCC func7\n");
                } break;
            }
        }

        tick_allInOne(); // tick the all-in-one state machine
    }

    // writes value of rs1 into rs2
    void performRegisterTransfer() {
        const uint16_t value_reg = curr_inst->getPhysIntRegIn(1);
        registerFiles->at(curr_inst->getHWThread())->setIntReg<int64_t>(value_reg, curr_roccCmd->rs1);
        completeRoCC(new RoCCResponse(curr_inst->getPhysIntRegOut(0), 4));
        return;
    }

    // issues the read request for the matrix that will be set in the analog array
    // setting the matrix into compute array happens in read request response handler
    void setMatrix() {
        StandardMem::Request* load_req = nullptr;
        uint32_t setMatrix_flag = 0x1;
        load_req = new StandardMem::Read(curr_roccCmd->rs1, array->getMatrixSize(), setMatrix_flag, 
                            curr_roccCmd->rs1, curr_roccCmd->inst->getInstructionAddress(), curr_roccCmd->inst->getHWThread());
        output->verbose(CALL_INFO, 9, 0, "----> Read Req: physAddr: %llx, size: %llx, vAddr: %llx, inst ptr: %llx, tid: %llx\n", 
                        curr_roccCmd->rs1, array->getMatrixSize(), curr_roccCmd->rs1, curr_roccCmd->inst->getInstructionAddress(), curr_roccCmd->inst->getHWThread());
        // if the instruction does not trap an error we will continue to process it
        if(LIKELY(! curr_roccCmd->inst->trapsError())) {
            output->verbose(CALL_INFO, 16, VANADIS_DBG_LSQ_LOAD_FLG, "-----> ins: 0x%llx / thr: %" PRIu32 " processed and requests sent to memory system.\n",
                curr_roccCmd->inst->getInstructionAddress(), curr_roccCmd->inst->getHWThread());

            assert(load_req != nullptr);

            memInterface->send(load_req);
        }
    }
    
    // just a generic load request based on instruction's register values
    void issueLoad() {
        StandardMem::Request* load_req = nullptr;

        load_req = new StandardMem::Read(curr_roccCmd->rs1, array->getInputArraySize() * 8, 0, 
                            curr_roccCmd->rs1, curr_roccCmd->inst->getInstructionAddress(), curr_roccCmd->inst->getHWThread());
        output->verbose(CALL_INFO, 9, 0, "----> Read Req: physAddr: %llx, size: %llx, vAddr: %llx, inst ptr: %llx, tid: %llx\n", 
                        curr_roccCmd->rs1, array->getInputArraySize() * 8, curr_roccCmd->rs1, curr_roccCmd->inst->getInstructionAddress(), curr_roccCmd->inst->getHWThread());
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
        int pieces = (outputOperandSize / 8) * arrayOutputSize;
        std::vector<uint8_t> payload(pieces);
        
        std::cout << "store payload: ";
        for (int i = 0; i < pieces; i++) {
            payload.at(i) = (int)crossSim_output[i] & (0xFF << i);
            std::cout << payload.at(i) << " ";
        }
        std::cout << std::endl;
        
        store_req = new StandardMem::Write(curr_roccCmd->rs1, 4, payload, 
            false, 0, curr_roccCmd->rs1, curr_roccCmd->inst->getInstructionAddress(), curr_roccCmd->inst->getHWThread());
        memInterface->send(store_req);

        return;
    }

    // All-in-one instruction runs through a set of steps
    // This function uses enum of state to traverse those steps
    // START: issue the load to pull the input vector
    // READ: wait for the load to return 
    // EXECUTE: pass the input vector into the analog compute array, then issue write for result when done
    // WRITE: wait for write to finish 
    void tick_allInOne() {
        switch (state) {
            case IDLE:
            {

            } break;
            case START:
            {
                output->verbose(CALL_INFO, 16, 0, "STATE START\n");             
                output->verbose(CALL_INFO, 16, 0, "Issuing load w/ rs1_val: %llx, rs2_val: %" PRIu64 "\n", curr_roccCmd->rs1, curr_roccCmd->rs2);
                issueLoad();
                state = READ;
            } break;
            case READ: // waiting state for the read response
            {
                output->verbose(CALL_INFO, 16, 0, "STATE READ\n");
            } break;
            case EXECUTE:
            {
                output->verbose(CALL_INFO, 16, 0, "STATE EXECUTE\n");
                array->setMatrix(); // hardcoded version of setMatrix() because parameterizable setMatrix() doesn't work yet
                array->compute(0);
                std::cout << "Normalized MVM results in RoCC: " << std::endl;
                crossSim_output = array->getOutputVector();

                for (int i = 0; i < 4; i++) { // hardcoded size 4 result for now
                    // normalize result onto [-1, 1] range
                    crossSim_output[i] = 2.0*((crossSim_output[i] - std::numeric_limits<float>::min()) /
                    (std::numeric_limits<float>::max() - std::numeric_limits<float>::min())); //-1
                    std::cout << crossSim_output[i] << " ";
                }
                std::cout << std::endl;
                issueStore();
                state = WRITE;
            } break;
            case WRITE: // waiting state for the write response
            {
                output->verbose(CALL_INFO, 16, 0, "STATE WRITE\n");
            } break;
        }
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
        state = IDLE;
        busy = false;
    }

    void handleArrayEvent(Event *ev) {
        Golem::ArrayEvent* aev = static_cast<Golem::ArrayEvent*>(ev);
        uint32_t arrayID = aev->getArrayID();

        output->verbose(CALL_INFO, 1, 0, "%s: Array %d completed array operation\n", getName().c_str(), arrayID);

        arrayStates[arrayID] = 0;
    }

    class StandardMemHandlers : public Interfaces::StandardMem::RequestHandler {
    public:
        friend class VanadisRoCCAnalog;

        StandardMemHandlers(VanadisRoCCAnalog* rocc, SST::Output* output) :
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
                case 0x0: // read response data  is input vector
                {
                    rocc->output->verbose(CALL_INFO, 16, 0, "input vector read response detected\n");
                    // build input vector from read response
                    for (auto i = 0; i < rocc->arrayInputSize; i++) { // for each entry in input vector
                        uint64_t operand = 0;
                        for (auto j = 0; j < rocc->inputOperandSize; j++) { // for each 8bit (I think?) chunk in a input vector entry
                            operand += ev->data[(rocc->inputOperandSize * i) + j];
                        }
                        rocc->arrayIns[0][i] = operand; // write this entry into input vector
                    }
                    rocc->array->setInputVector(rocc->arrayIns[0]); // another way of writing input vector to compute array
                } break;
                case 0x1: // read response data is matrix to be set
                {
                    rocc->output->verbose(CALL_INFO, 16, 0, "set matrix read response detected\n");
                    float* buffer_row = new float[rocc->arrayInputSize]; // single row of matrix
                    auto num_rows = (ev->size) / (rocc->arrayInputSize * rocc->inputOperandSize); // compute number of rows in matrix
                    std::cout << "writing " << num_rows << " rows to matrix" << std::endl;
                    // build matrix and send each row individually 
                    for (auto k = 0; k < num_rows; k++) { // for each row in matrix
                        for (auto i = 0; i < rocc->arrayInputSize; i++) { // for each entry in a matrix row
                            uint64_t operand = 0;
                            for (auto j = 0; j < rocc->inputOperandSize; j++) { // for each 8bit (I think?) chunk in a matrix row entry
                                operand += ev->data[(rocc->inputOperandSize * i) + j];
                            }
                            buffer_row[i] = (float)operand; // put one matrix row entry into the row
                        }
                        // send individual row to analog compute array
                        if (k == num_rows - 1) {
                            rocc->array->setMatrixBuffered(buffer_row, true); // this is the last row and should trigger the full set matrix
                        } else {
                            rocc->array->setMatrixBuffered(buffer_row, false); // this is not the last row and should just accumulate into a buffer in the analog compute array
                        }
                    }
                    rocc->completeRoCC(new RoCCResponse(rocc->curr_inst->getPhysIntRegOut(0), 1));
                } break;
                default:
                {
                    rocc->output->verbose(CALL_INFO, 16, 0, "ERROR: unrecognized read response flag\n");
                } break;
            }

            //rocc->registerFiles->at(hw_thr)->print(out);
            rocc->registerFiles->at(hw_thr)->copyToIntRegister(target_reg, 0, &register_value[0], register_value.size()); // write the read response data into the destination register of the instruciton
            //rocc->registerFiles->at(hw_thr)->print(out);
            if (rocc->state == READ) rocc->state = EXECUTE; // if there is an all-in-one being processsed, progress it 
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
    
        VanadisRoCCAnalog* rocc;
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
    STATE state; // state for all-in-one

    StandardMemHandlers* std_mem_handlers;
    StandardMem* memInterface;

    Golem::ComputeArray* array;
    std::vector<std::vector<uint64_t>> arrayIns;
    std::vector<std::vector<uint64_t>> arrayOuts;
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

    int max_instructions;
};

} // namespace Vanadis
} // namespace SST

#endif