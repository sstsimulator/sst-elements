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

#ifndef _H_VANADIS_SYSCALL_WRITE
#define _H_VANADIS_SYSCALL_WRITE

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallWriteEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallWriteEvent() : VanadisSyscallEvent() {}
    VanadisSyscallWriteEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, int64_t fd, uint64_t buff_addr, int64_t buff_count)
        : VanadisSyscallEvent(core, thr, bittype), write_fd(fd), write_buffer(buff_addr), write_count(buff_count) {}

    VanadisSyscallOp getOperation() { return SYSCALL_OP_WRITE; }

    int64_t getFileDescriptor() const { return write_fd; }
    uint64_t getBufferAddress() const { return write_buffer; }
    uint64_t getBufferCount() const { return write_count; }

private:
    int64_t write_fd;
    uint64_t write_buffer;
    uint64_t write_count;
};

} // namespace Vanadis
} // namespace SST

#endif
