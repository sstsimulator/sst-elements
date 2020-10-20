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

#include <queue>
#include <vector>
#include <bitset>
#include <cstdint>

#define Bit_Length 64

namespace SST {
namespace Llyr {

typedef enum {
    ANY,        //0
    LD,         //1
    ST,         //2
    ADD,        //3
    SUB,        //4
    MUL,        //5
    DIV,        //6
    FPADD,      //7
    FPSUB,      //8
    FPMUL,      //9
    FPDIV,      //10
    DUMMY,      //11
    OTHER       //12
} opType;

class ProcessingElement
{
public:
    ProcessingElement();
    ProcessingElement(SST::Llyr::opType op_binding, uint32_t processor_id, uint32_t queue_depth);

    ~ProcessingElement();

    void setOpBinding(opType binding);
    opType getOpBinding() const { return op_binding_; }

    void setProcessorId(uint32_t id) { processor_id_ = id; }
    uint32_t getProcessorId() const { return processor_id_; }

    bool doCompute();
    std::bitset< Bit_Length > popOutput();

    bool fakeInit();

protected:


private:
    opType op_binding_;
    uint32_t processor_id_;

    std::vector< std::queue< std::bitset< 64 > >* >* input_queues_;
    std::vector< std::queue< std::bitset< 64 > >* >* output_queues_;

    uint32_t num_inputs_;
    uint32_t num_outputs_;
    uint32_t queue_depth_;

    SST::Output* output;

};


}
}

#endif
