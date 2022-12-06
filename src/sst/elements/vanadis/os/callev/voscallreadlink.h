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

#ifndef _H_VANADIS_SYSCALL_READLINK
#define _H_VANADIS_SYSCALL_READLINK

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallReadLinkEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallReadLinkEvent() : VanadisSyscallEvent() {}
    VanadisSyscallReadLinkEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t path_ptr, uint64_t buff_ptr, int64_t size)
        : VanadisSyscallEvent(core, thr, bittype), readlink_path_ptr(path_ptr), readlink_buff_ptr(buff_ptr),
          readlink_buff_size(size) {}

    VanadisSyscallOp getOperation() { return SYSCALL_OP_READLINK; }

    uint64_t getPathPointer() const { return readlink_path_ptr; }
    uint64_t getBufferPointer() const { return readlink_buff_ptr; }
    int64_t getBufferSize() const { return readlink_buff_size; }

private:
    uint64_t readlink_path_ptr;
    uint64_t readlink_buff_ptr;
    int64_t readlink_buff_size;
};

} // namespace Vanadis
} // namespace SST

#endif
