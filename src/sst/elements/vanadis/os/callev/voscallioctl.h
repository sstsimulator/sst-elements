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

#ifndef _H_VANADIS_SYSCALL_IOCTL
#define _H_VANADIS_SYSCALL_IOCTL

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallIOCtlEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallIOCtlEvent() : VanadisSyscallEvent() {}
    VanadisSyscallIOCtlEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, int64_t file_d, bool o_read, bool o_write,
                             uint64_t io_request, uint64_t io_driver, uint64_t data_ptr, uint64_t data_len)
        : VanadisSyscallEvent(core, thr, bittype), fd(file_d), op_read(o_read), op_write(o_write), io_op(io_request),
          io_drv(io_driver), ptr(data_ptr), ptr_len(data_len) {}

    VanadisSyscallOp getOperation() { return SYSCALL_OP_IOCTL; }

    int64_t getFileDescriptor() const { return fd; }
    uint64_t getIOOperation() const { return io_op; }
    uint64_t getIODriver() const { return io_drv; }

    uint64_t getDataPointer() const { return ptr; }
    uint64_t getDataLength() const { return ptr_len; }

    bool isRead() const { return op_read; }
    bool isWrite() const { return op_write; }

private:
    int64_t fd;

    bool op_read;
    bool op_write;

    uint64_t io_op;
    uint64_t io_drv;

    uint64_t ptr;
    uint64_t ptr_len;
};

} // namespace Vanadis
} // namespace SST

#endif
