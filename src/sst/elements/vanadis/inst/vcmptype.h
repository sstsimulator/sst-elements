// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_COMPARE_TYPE
#define _H_VANADIS_COMPARE_TYPE

namespace SST {
namespace Vanadis {

enum VanadisRegisterCompareType {
    REG_COMPARE_EQ,
    REG_COMPARE_ULT,
    REG_COMPARE_LT,
    REG_COMPARE_LTE,
    REG_COMPARE_GT,
    REG_COMPARE_GTE,
    REG_COMPARE_NEQ
};

const char*
convertCompareTypeToString(VanadisRegisterCompareType cType)
{
    switch ( cType ) {
    case REG_COMPARE_EQ:
        return "EQ";
    case REG_COMPARE_ULT:
        return "ULT";
    case REG_COMPARE_LT:
        return "LT";
    case REG_COMPARE_LTE:
        return "LTE";
    case REG_COMPARE_GT:
        return "GT";
    case REG_COMPARE_GTE:
        return "GTE";
    case REG_COMPARE_NEQ:
        return "NEQ";
    }

    return "";
};

} // namespace Vanadis
} // namespace SST

#endif
