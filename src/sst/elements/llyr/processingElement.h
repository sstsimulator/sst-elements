// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _LLYR_PE_H
#define _LLYR_PE_H

#include <sst/core/output.h>
#include <sst/core/interfaces/simpleMem.h>

#include <map>
#include <queue>
#include <vector>
#include <bitset>
#include <cstdint>

#define Bit_Length 64

using namespace SST::Interfaces;

namespace SST {
namespace Llyr {

typedef enum {
    ANY,
    LD,
    ST,
    ADD = 0x40,
    SUB,
    MUL,
    DIV,
    FPADD = 0x80,
    FPSUB,
    FPMUL,
    FPDIV,
    DUMMY = 0xFF,
    OTHER
} opType;

class ProcessingElement
{
public:
    ProcessingElement();
    ProcessingElement(opType op_binding, uint32_t processor_id, uint32_t queue_depth,
                      SimpleMem*  mem_interface);
    ProcessingElement(opType op_binding, uint32_t processor_id, uint32_t queue_depth,
                      SimpleMem*  mem_interface,
                      std::queue< std::bitset< Bit_Length > >* init_input_0);
    ProcessingElement(opType op_binding, uint32_t processor_id, uint32_t queue_depth,
                      SimpleMem*  mem_interface,
                      std::queue< std::bitset< Bit_Length > >* init_input_0,
                      std::queue< std::bitset< Bit_Length > >* init_input_1);

    ~ProcessingElement();

    uint32_t bindInputQueue(uint32_t id);
    uint32_t bindOutputQueue(uint32_t id);

    uint32_t getInputQueue(uint32_t id) { return input_queue_map_[id]; }
    uint32_t getOutputQueue(uint32_t id) { return output_queue_map_[id]; }

    void     setOpBinding(opType binding);
    opType   getOpBinding() const { return op_binding_; }

    void     setProcessorId(uint32_t id) { processor_id_ = id; }
    uint32_t getProcessorId() const { return processor_id_; }

    bool     getPendingOp() const { return pending_op_; }

    bool doCompute();
    bool doLoad(uint64_t addr);
    std::bitset< Bit_Length > popOutput();

    bool fakeInit();

protected:


private:
    opType   op_binding_;
    uint32_t processor_id_;

    //used to stall execution - waiting on mem/queues full
    bool pending_op_;

    //input and output queues per PE -- note that these are currently fixed based on operation
    uint32_t num_inputs_;
    uint32_t num_outputs_;
    uint32_t queue_depth_;

    std::vector< std::queue< std::bitset< Bit_Length > >* >* input_queues_;
    std::vector< std::queue< std::bitset< Bit_Length > >* >* output_queues_;

    //need to connect PEs to queues -- queue_id, src/dst
    std::map< uint32_t, uint32_t > input_queue_map_;
    std::map< uint32_t, uint32_t > output_queue_map_;

    SimpleMem*  mem_interface_;
    SST::Output* output_;

    void printInputQueue();
    void printOutputQueue();

};


}
}

#endif
