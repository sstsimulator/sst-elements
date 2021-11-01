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


#ifndef _LLYR_TYPES
#define _LLYR_TYPES

#include <sst/core/interfaces/stdMem.h>

#include <bitset>
#include <string>

#define Bit_Length 64
typedef std::bitset< Bit_Length > LlyrData;
typedef uint64_t Addr;

using namespace SST::Experimental::Interfaces;

namespace SST {
namespace Llyr {

// forward declaration of LSQueue
class LSQueue;

// data type to pass between Llyr, mapper, and PEs
typedef struct alignas(64) {
    LSQueue*        lsqueue_;
    StandardMem*    mem_interface_;

    uint32_t    verbosity_;
    uint16_t    queueDepth_;
    uint16_t    arith_latency_;
    uint16_t    int_latency_;
    uint16_t    fp_latency_;
    uint16_t    fp_mul_Latency_;
    uint16_t    fp_div_Latency_;
} LlyrConfig;

typedef enum {
    ANY,
    ANY_MEM,
    LD,
    LDADDR,
    LD_ST,
    ST,
    STADDR,
    ANY_LOGIC = 0x20,
    AND,
    OR,
    XOR,
    NOT,
    SLL,
    SLR,
    ROL,
    ROR,
    ANY_TEST = 0x40,
    EQ,
    NE,
    UGT,
    UGE,
    SGT,
    SGE,
    ULT,
    ULE,
    SLT,
    SLE,
    ANY_INT = 0x80,
    ADD,
    SUB,
    MUL,
    DIV,
    ADDCONST,
    SUBCONST,
    MULCONST,
    DIVCONST,
    ANY_FP = 0xC0,
    FPADD,
    FPSUB,
    FPMUL,
    FPDIV,
    FPMATMUL,
    ANY_CP = 0xF0,
    TSIN,
    TCOS,
    DUMMY = 0xFF,
    BUFFER,
    SEL,
    OTHER
} opType;

// application graph node
typedef struct alignas(64) {
    opType      optype_;
    std::string constant_val_;
} AppNode;

}//Llyr
}//SST

#endif
