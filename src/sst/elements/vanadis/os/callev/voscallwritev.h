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

#ifndef _H_VANADIS_SYSCALL_WRITEV
#define _H_VANADIS_SYSCALL_WRITEV

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallWritevEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallWritevEvent() : VanadisSyscallEvent() {}
    VanadisSyscallWritevEvent(uint32_t core, uint32_t thr, int64_t fd, uint64_t iovec_addr, int64_t iovec_count)
        : VanadisSyscallEvent(core, thr), writev_fd(fd), writev_iovec_addr(iovec_addr), writev_iov_count(iovec_count) {}

    VanadisSyscallOp getOperation() { return SYSCALL_OP_WRITEV; }

    int64_t getFileDescriptor() const { return writev_fd; }
    uint64_t getIOVecAddress() const { return writev_iovec_addr; }
    int64_t getIOVecCount() const { return writev_iov_count; }

private:
    int64_t writev_fd;
    uint64_t writev_iovec_addr;
    int64_t writev_iov_count;
};

} // namespace Vanadis
} // namespace SST

#endif
