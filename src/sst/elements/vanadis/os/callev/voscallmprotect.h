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

#ifndef _H_VANADIS_SYSCALL_MPROTECT
#define _H_VANADIS_SYSCALL_MPROTECT

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallMprotectEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallMprotectEvent() : VanadisSyscallEvent() {}
    VanadisSyscallMprotectEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t addr, uint64_t len, int64_t prot)
        : VanadisSyscallEvent(core, thr, bittype), addr(addr), len(len), prot(prot) {}

    VanadisSyscallOp getOperation() { return SYSCALL_OP_MPROTECT; }

    int64_t getAddr() const { return addr; }
    uint64_t getLen() const { return len; }
    int64_t getProt() const { return prot; }

private:
    uint64_t addr;
    uint64_t len;
    uint64_t prot;
};

} // namespace Vanadis
} // namespace SST

#endif
