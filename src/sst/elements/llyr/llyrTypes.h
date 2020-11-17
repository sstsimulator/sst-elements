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

#include <bitset>

#define Bit_Length 64
typedef std::bitset< Bit_Length > LlyrData;

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

}//Llyr
}//SST

#endif
