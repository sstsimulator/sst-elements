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

#ifndef _H_VANADIS_SYSCALL_CLONE
#define _H_VANADIS_SYSCALL_CLONE

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallCloneEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallCloneEvent() : VanadisSyscallEvent() {}
    VanadisSyscallCloneEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t instPtr, uint64_t threadStackAddr,
                                uint64_t flags, uint64_t ptid, uint64_t tls, uint64_t stackAddr )
        : VanadisSyscallEvent(core, thr, bittype, instPtr), threadStackAddr(threadStackAddr), flags(flags), ptid(ptid), tls(tls), stackAddr(stackAddr) {}

    VanadisSyscallOp getOperation() { return SYSCALL_OP_CLONE; }

    uint64_t getThreadStackAddr() const { return threadStackAddr; }
    uint64_t getFlags() const { return flags; }
    uint64_t getParentTidAddr() const { return ptid; }
    uint64_t getTlsAddr() const { return tls; }
    uint64_t getCallStackAddr() const { return stackAddr; }

private:
    uint64_t threadStackAddr; 
    uint64_t flags;
    uint64_t ptid;
    uint64_t tls;
    uint64_t stackAddr;
};

} // namespace Vanadis
} // namespace SST

#endif
