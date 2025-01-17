// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_SYSCALL_READV
#define _H_VANADIS_SYSCALL_READV

#include "os/callev/voscalliovec.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallReadvEvent : public VanadisSyscallIoVecEvent {
public:
    VanadisSyscallReadvEvent() : VanadisSyscallIoVecEvent() {}
    VanadisSyscallReadvEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, int64_t fd, uint64_t iovec_addr, int64_t iovec_count)
        : VanadisSyscallIoVecEvent(core, thr, bittype, fd, iovec_addr, iovec_count) {}

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_READV; }
private:
    ImplementSerializable(SST::Vanadis::VanadisSyscallReadvEvent);
};

} // namespace Vanadis
} // namespace SST

#endif
