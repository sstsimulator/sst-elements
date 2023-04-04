// Copyright 2013-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2022, NTESS
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

#include <list>
#include <tuple>
#include <bitset>
#include <string>

#define Bit_Length 64
typedef std::bitset< Bit_Length > LlyrData;
typedef std::string Arg;
typedef uint64_t Addr;

typedef std::pair< const std::string, uint32_t > pair_t;
typedef std::tuple< const std::string, uint32_t, uint32_t > triple_t;
typedef std::list< std::string > slist_t;
typedef std::list< pair_t >   plist_t;
typedef std::list< triple_t > tlist_t;

using namespace SST::Interfaces;

namespace SST {
namespace Llyr {

// forward declaration of LSQueue
class LSQueue;

// data type to pass between Llyr, mapper, and PEs
typedef struct alignas(uint64_t) {
    LSQueue*        lsqueue_;
    StandardMem*    mem_interface_;
    Addr            starting_addr_;
    std::string     mapping_tool_;

    uint32_t        verbosity_;
    uint16_t        queueDepth_;
    uint16_t        arith_latency_;
    uint16_t        int_latency_;
    uint16_t        int_div_latency_;
    uint16_t        fp_latency_;
    uint16_t        fp_mul_latency_;
    uint16_t        fp_div_latency_;
    uint16_t        complex_latency_;
} LlyrConfig;

// data type to store PE data
typedef std::pair< std::string, std::string > PairEdge;
typedef std::pair< std::string, uint32_t > PairPE;
typedef std::tuple< std::string, uint32_t, uint32_t > TriplePE;

// data type for adv pes
typedef std::map< uint32_t, Arg > QueueArgMap;

typedef struct alignas(uint64_t) {
    std::string     pe_id_;
    std::string     job_id_;
    std::string     op_;
    std::list< std::string >* const_list_;
    std::list< PairPE >*      input_list_;
    std::list< PairPE >*      output_list_;
    std::list< TriplePE >*    route_list_;
} HardwareNode;

typedef enum {
    ROUTE = 0x00,
    ANY,
    ANY_MEM,
    LD,
    LDADDR,
    STREAM_LD,
    ST,
    STADDR,
    STREAM_ST,
    ALLOCA,
    ANY_LOGIC = 0x20,
    AND,
    OR,
    XOR,
    NOT,
    SLL,
    SLR,
    ROL,
    ROR,
    AND_IMM,
    OR_IMM,
    ANY_TEST = 0x40,
    EQ,
    NE,
    UGT,
    UGT_IMM,
    UGE,
    UGE_IMM,
    SGT,
    SGT_IMM,
    SGE,
    ULT,
    ULE,
    SLT,
    SLT_IMM,
    SLE,
    ANY_INT = 0x80,
    ADD,
    SUB,
    MUL,
    DIV,
    REM,
    ADDCONST,
    SUBCONST,
    MULCONST,
    DIVCONST,
    REMCONST,
    INC,
    ACC,
    ANY_FP = 0xC0,
    FADD,
    FSUB,
    FMUL,
    FDIV,
    FMatMul,
    ANY_CP = 0xF0,
    TSIN,
    TCOS,
    TTAN,
    DUMMY = 0xFF,
    BUFFER,
    REPEATER,
    ROS,
    SEL,
    RET,
    OTHER
} opType;

// application graph node
typedef struct alignas(uint64_t) {
    opType optype_;
    Arg    argument_[2];
} AppNode;

}//Llyr
}//SST

#endif // _LLYR_TYPES
