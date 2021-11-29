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

#ifndef _H_VANADIS_SYSCALL_UNMAP
#define _H_VANADIS_SYSCALL_UNMAP

#include "os/voscallev.h"

#include <cinttypes>
#include <cstdint>

namespace SST {
namespace Vanadis {

class VanadisSyscallMemoryUnMapEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallMemoryUnMapEvent() : VanadisSyscallEvent() {}

    VanadisSyscallMemoryUnMapEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t addr, uint64_t len)
		  : VanadisSyscallEvent(core, thr, bittype), address(addr), length(len) {}

    VanadisSyscallOp getOperation() { return SYSCALL_OP_UNMAP; }

    uint64_t getDeallocationAddress() const { return address; }
    uint64_t getDeallocationLength() const { return length; }

private:
    uint64_t address;
    uint64_t length;
};

} // namespace Vanadis
} // namespace SST

#endif
