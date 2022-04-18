// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_SYSCALL_UNLINK
#define _H_VANADIS_SYSCALL_UNLINK

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallUnlinkEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallUnlinkEvent() : VanadisSyscallEvent() {}
    VanadisSyscallUnlinkEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t path_ptr)
        : VanadisSyscallEvent(core, thr, bittype), path_ptr(path_ptr) {}

    VanadisSyscallOp getOperation() { return SYSCALL_OP_UNLINK; }

    uint64_t getPathPointer() const { return path_ptr; }

private:
    uint64_t path_ptr;
};

} // namespace Vanadis
} // namespace SST

#endif
