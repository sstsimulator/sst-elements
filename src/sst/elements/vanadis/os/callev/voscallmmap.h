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

#ifndef _H_VANADIS_SYSCALL_MMAP
#define _H_VANADIS_SYSCALL_MMAP

#include "os/voscallev.h"

#include <cinttypes>
#include <cstdint>

namespace SST {
namespace Vanadis {

class VanadisSyscallMemoryMapEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallMemoryMapEvent() : VanadisSyscallEvent() {}

    VanadisSyscallMemoryMapEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t addr, uint64_t len, int64_t prot, int64_t flags, int fd,
                                 uint64_t offset, uint64_t stack_p, uint64_t offset_multiplier)
        : VanadisSyscallEvent(core, thr, bittype), address(addr), length(len), page_prot(prot), alloc_flags(flags), fd(fd), offset(offset),
          stack_pointer(stack_p), offset_units(offset_multiplier) {}

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_MMAP; }

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
    uint64_t getOffset() const { return offset; }

    int getFd() { return fd; }
private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        SST_SER(address);
        SST_SER(length);
        SST_SER(page_prot);
        SST_SER(alloc_flags);
        SST_SER(stack_pointer);
        SST_SER(offset);
        SST_SER(offset_units);
        SST_SER(fd);
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallMemoryMapEvent);

    uint64_t address;
    uint64_t length;
    int64_t page_prot;
    int64_t alloc_flags;
    uint64_t stack_pointer;
    uint64_t offset;
    uint64_t offset_units;
    int fd;
};

} // namespace Vanadis
} // namespace SST

#endif
