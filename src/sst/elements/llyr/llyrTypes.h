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

#include <bitset>
#include <string>

#define Bit_Length 64
typedef std::bitset< Bit_Length > LlyrData;
typedef std::string Arg;
typedef uint64_t Addr;

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
    uint16_t        fp_latency_;
    uint16_t        fp_mul_Latency_;
    uint16_t        fp_div_Latency_;
} LlyrConfig;

typedef enum {
    RTR = 0x00,
    ANY,
    ANY_MEM,
    LD,
    LDADDR,
    STREAM_LD,
    ST,
    STADDR,
    STREAM_ST,
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
    REM,
    ADDCONST,
    SUBCONST,
    MULCONST,
    DIVCONST,
    REMCONST,
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
    SEL,
    RET,
    OTHER
} opType;

inline opType getOptype(std::string &opString)
{
    opType operation;

    // transform to make opString case insensitive
    std::transform(opString.begin(), opString.end(), opString.begin(),
                   [](unsigned char c){ return std::toupper(c); }
    );

    if( opString == "RTR" )
        operation = RTR;
    else if( opString == "ANY" )
        operation = ANY;
    else if( opString == "ANY_MEM" )
        operation = ANY_MEM;
    else if( opString == "LD" )
        operation = LD;
    else if( opString == "LDADDR" )
        operation = LDADDR;
    else if( opString == "STREAM_LD" )
        operation = STREAM_LD;
    else if( opString == "ST" )
        operation = ST;
    else if( opString == "STADDR" )
        operation = STADDR;
    else if( opString == "STREAM_ST" )
        operation = STREAM_ST;
    else if( opString == "ANY_LOGIC" )
        operation = ANY_LOGIC;
    else if( opString == "AND" )
        operation = AND;
    else if( opString == "OR" )
        operation = OR;
    else if( opString == "XOR" )
        operation = XOR;
    else if( opString == "NOT" )
        operation = NOT;
    else if( opString == "SLL" )
        operation = SLL;
    else if( opString == "SLR" )
        operation = SLR;
    else if( opString == "ROL" )
        operation = ROL;
    else if( opString == "ROR" )
        operation = ROR;
    else if( opString == "EQ" )
        operation = EQ;
    else if( opString == "NE" )
        operation = NE;
    else if( opString == "UGT" )
        operation = UGT;
    else if( opString == "UGE" )
        operation = UGE;
    else if( opString == "SGT" )
        operation = SGT;
    else if( opString == "SGE" )
        operation = SGE;
    else if( opString == "ULT" )
        operation = ULT;
    else if( opString == "ULE" )
        operation = ULE;
    else if( opString == "SLT" )
        operation = SLT;
    else if( opString == "SLE" )
        operation = SLE;
    else if( opString == "ANY_INT" )
        operation = ANY_INT;
    else if( opString == "ADD" )
        operation = ADD;
    else if( opString == "SUB" )
        operation = SUB;
    else if( opString == "MUL" )
        operation = MUL;
    else if( opString == "DIV" )
        operation = DIV;
    else if( opString == "REM" )
        operation = REM;
    else if( opString == "ADDCONST" )
        operation = ADDCONST;
    else if( opString == "SUBCONST" )
        operation = SUBCONST;
    else if( opString == "MULCONST" )
        operation = MULCONST;
    else if( opString == "DIVCONST" )
        operation = DIVCONST;
    else if( opString == "REMCONST" )
        operation = REMCONST;
    else if( opString == "ANY_FP" )
        operation = ANY_FP;
    else if( opString == "FADD" )
        operation = FADD;
    else if( opString == "FSUB" )
        operation = FSUB;
    else if( opString == "FMUL" )
        operation = FMUL;
    else if( opString == "FDIV" )
        operation = FDIV;
    else if( opString == "FMatMul" )
        operation = FMatMul;
    else if( opString == "ANY_CP" )
        operation = ANY_CP;
    else if( opString == "TSIN" )
        operation = TSIN;
    else if( opString == "TCOS" )
        operation = TCOS;
    else if( opString == "TTAN" )
        operation = TTAN;
    else if( opString == "DUMMY" )
        operation = DUMMY;
    else if( opString == "BUFFER" )
        operation = BUFFER;
    else if( opString == "SEL" )
        operation = SEL;
    else if( opString == "RET" )
        operation = RET;
    else
        operation = OTHER;

    return operation;
}

inline std::string getOpString(opType &op)
{
    std::string operation;

    if( op == RTR )
        operation = "RTR";
    else if( op == ANY )
        operation = "ANY";
    else if( op == ANY_MEM )
        operation = "ANY_MEM";
    else if( op == LD )
        operation = "LD";
    else if( op == LDADDR )
        operation = "LDADDR";
    else if( op == STREAM_LD )
        operation = "STREAM_LD";
    else if( op == ST )
        operation = "ST";
    else if( op == STADDR )
        operation = "STADDR";
    else if( op == STREAM_ST )
        operation = "STREAM_ST";
    else if( op == ANY_LOGIC )
        operation = "ANY_LOGIC";
    else if( op == AND )
        operation = "AND";
    else if( op == OR )
        operation = "OR";
    else if( op == XOR )
        operation = "XOR";
    else if( op == NOT )
        operation = "NOT";
    else if( op == SLL )
        operation = "SLL";
    else if( op == SLR )
        operation = "SLR";
    else if( op == ROL )
        operation = "ROL";
    else if( op == ROR )
        operation = "ROR";
    else if( op == EQ )
        operation = "EQ";
    else if( op == NE )
        operation = "NE";
    else if( op == UGT )
        operation = "UGT";
    else if( op == UGE )
        operation = "UGE";
    else if( op == SGT )
        operation = "SGT";
    else if( op == SGE )
        operation = "SGE";
    else if( op == ULT )
        operation = "ULT";
    else if( op == ULE )
        operation = "ULE";
    else if( op == SLT )
        operation = "SLT";
    else if( op == SLE )
        operation = "SLE";
    else if( op == ANY_INT )
        operation = "ANY_INT";
    else if( op == ADD )
        operation = "ADD";
    else if( op == SUB )
        operation = "SUB";
    else if( op == MUL )
        operation = "MUL";
    else if( op == DIV )
        operation = "DIV";
    else if( op == REM )
        operation = "REM";
    else if( op == ADDCONST )
        operation = "ADDCONST";
    else if( op == SUBCONST )
        operation = "SUBCONST";
    else if( op == MULCONST )
        operation = "MULCONST";
    else if( op == DIVCONST )
        operation = "DIVCONST";
    else if( op == REMCONST )
        operation = "REMCONST";
    else if( op == ANY_FP )
        operation = "ANY_FP";
    else if( op == FADD )
        operation = "FADD";
    else if( op == FSUB )
        operation = "FSUB";
    else if( op == FMUL )
        operation = "FMUL";
    else if( op == FDIV )
        operation = "FDIV";
    else if( op == FMatMul )
        operation = "FMatMul";
    else if( op == ANY_CP )
        operation = "ANY_CP";
    else if( op == TSIN )
        operation = "TSIN";
    else if( op == TCOS )
        operation = "TCOS";
    else if( op == TTAN )
        operation = "TTAN";
    else if( op == DUMMY )
        operation = "DUMMY";
    else if( op == BUFFER )
        operation = "BUFFER";
    else if( op == SEL )
        operation = "SEL";
    else if( op == RET )
        operation = "RET";
    else
        operation = "OTHER";

    return operation;
}

// application graph node
typedef struct alignas(uint64_t) {
    opType optype_;
    Arg    argument_[2];
} AppNode;

}//Llyr
}//SST

#endif
