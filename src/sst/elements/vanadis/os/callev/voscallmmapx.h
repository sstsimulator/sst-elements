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

#ifndef _H_VANADIS_SYSCALL_MMAPX
#define _H_VANADIS_SYSCALL_MMAPX

#include "os/voscallev.h"

#include <cinttypes>
#include <cstdint>

namespace SST {
namespace Vanadis {

class VanadisSyscallMemoryMapExtEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallMemoryMapExtEvent() : VanadisSyscallEvent() {}

    VanadisSyscallMemoryMapExtEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t addr, uint64_t len, int64_t prot, int64_t flags,
											int64_t _fd, uint64_t offst, uint64_t offset_multiplier)
        : VanadisSyscallEvent(core, thr, bittype), address(addr), length(len), page_prot(prot), alloc_flags(flags),
          fd(_fd), offset(offst), offset_units(offset_multiplier) {
		isAnon = (alloc_flags & 0x800) != 0;
	}

    VanadisSyscallOp getOperation() { return SYSCALL_OP_MMAPX; }

    uint64_t getAllocationAddress() const { return address; }
    uint64_t getAllocationLength() const { return length; }
    int64_t getAllocationFlags() const { return alloc_flags; }
    int64_t getProtectionFlags() const { return page_prot; }
	 int64_t getFileDescriptor() const { return fd; }
	 uint64_t getOffset() const { return offset; }

    bool isAllocationReadable() const { return true; }
    bool isAllocationWritable() const { return true; }
    bool isAllocationExecutable() const { return true; }

    bool isAllocationAnonymous() const { return isAnon; }

    uint64_t getOffsetUnits() const { return offset_units; }

private:
    uint64_t address;
    uint64_t length;
    int64_t page_prot;
    int64_t alloc_flags;
	 int64_t fd;
	 uint64_t offset;
    uint64_t offset_units;

    bool isAnon;
};

} // namespace Vanadis
} // namespace SST

#endif
