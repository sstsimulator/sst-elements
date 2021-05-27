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

#ifndef _H_VANADIS_MEM_FLAG_TYPE
#define _H_VANADIS_MEM_FLAG_TYPE

namespace SST {
namespace Vanadis {

enum VanadisMemoryTransaction {
    MEM_TRANSACTION_NONE,
    MEM_TRANSACTION_LLSC_LOAD,
    MEM_TRANSACTION_LLSC_STORE,
    MEM_TRANSACTION_LOCK
};

const char*
getTransactionTypeString(VanadisMemoryTransaction transT) {
    switch (transT) {
    case MEM_TRANSACTION_NONE:
        return "STD";
    case MEM_TRANSACTION_LLSC_LOAD:
        return "LLSC_LOAD";
    case MEM_TRANSACTION_LLSC_STORE:
        return "LLSC_STORE";
    case MEM_TRANSACTION_LOCK:
        return "LOCK";
    default:
        return "UNKNOWN";
    }
};

} // namespace Vanadis
} // namespace SST

#endif
