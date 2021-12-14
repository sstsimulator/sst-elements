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

#ifndef _H_VANADIS_SYSCALL_INIT_BRK
#define _H_VANADIS_SYSCALL_INIT_BRK

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallInitBRKEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallInitBRKEvent() : VanadisSyscallEvent() {}
    VanadisSyscallInitBRKEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t newBrkAddr)
        : VanadisSyscallEvent(core, thr, bittype), newBrk(newBrkAddr) {}

    VanadisSyscallOp getOperation() { return SYSCALL_OP_INIT_BRK; }

    uint64_t getUpdatedBRK() const { return newBrk; }

private:
    uint64_t newBrk;
};

} // namespace Vanadis
} // namespace SST

#endif
