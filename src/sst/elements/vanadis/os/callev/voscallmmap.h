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

#ifndef _H_VANADIS_SYSCALL_MMAP
#define _H_VANADIS_SYSCALL_MMAP

#include "os/voscallev.h"

#include <cinttypes>
#include <cstdint>

namespace SST {
namespace Vanadis {

// void *mmap(void *addr, size_t length, int prot, int flags,
//                  int fd, off_t offset);

class VanadisSyscallMemoryMapEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallMemoryMapEvent() : VanadisSyscallEvent() {}

    VanadisSyscallMemoryMapEvent(uint32_t core, uint32_t thr, uint64_t addr, uint64_t len, int64_t prot, int64_t flags,
                                 uint64_t stack_p, uint64_t offset_multiplier)
        : VanadisSyscallEvent(core, thr), address(addr), length(len), page_prot(prot), alloc_flags(flags),
          stack_pointer(stack_p), offset_units(offset_multiplier) {}

    VanadisSyscallOp getOperation() { return SYSCALL_OP_MMAP; }

    uint64_t getAllocationAddress() const { return address; }
    uint64_t getAllocationLength() const { return length; }
    int64_t getAllocationFlags() const { return alloc_flags; }
    int64_t getProtectionFlags() const { return page_prot; }

    bool isAllocationReadable() const { return true; }
    bool isAllocationWritable() const { return true; }
    bool isAllocationExecutable() const { return true; }

    bool isAllocationAnonymous() const { return (alloc_flags & 0x800) != 0; }

    uint64_t getStackPointer() const { return stack_pointer; }
    uint64_t getOffsetUnits() const { return offset_units; }

private:
    uint64_t address;
    uint64_t length;
    int64_t page_prot;
    int64_t alloc_flags;
    uint64_t stack_pointer;
    uint64_t offset_units;
};

} // namespace Vanadis
} // namespace SST

#endif
