// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_INST_CLASS
#define _H_VANADIS_INST_CLASS

namespace SST {
namespace Vanadis {

enum VanadisFunctionalUnitType {
    INST_INT_ARITH,
    INST_INT_DIV,
    INST_FP_ARITH,
    INST_FP_DIV,
    INST_LOAD,
    INST_STORE,
    INST_BRANCH,
    INST_SPECIAL,
    INST_SYSCALL,
    INST_FENCE,
    INST_NOOP,
    INST_FAULT
};

const char*
funcTypeToString(VanadisFunctionalUnitType unit_type)
{
    switch ( unit_type ) {
    case INST_INT_ARITH:
        return "INT_ARITH";
    case INST_INT_DIV:
        return "INT_DIV";
    case INST_FP_ARITH:
        return "FP_ARITH";
    case INST_FP_DIV:
        return "FP_DIV";
    case INST_LOAD:
        return "LOAD";
    case INST_STORE:
        return "STORE";
    case INST_BRANCH:
        return "BRANCH";
    case INST_SPECIAL:
        return "SPECIAL";
    case INST_FENCE:
        return "FENCE";
    case INST_NOOP:
        return "NOOP";
    case INST_FAULT:
        return "FAULT";
    case INST_SYSCALL:
        return "SYSCALL";
    default:
        return "UNKNOWN";
    }
};

} // namespace Vanadis
} // namespace SST

#endif
