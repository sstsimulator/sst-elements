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

#ifndef _H_VANADIS_SYSCALL_UNAME
#define _H_VANADIS_SYSCALL_UNAME

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallUnameEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallUnameEvent() : VanadisSyscallEvent() {}
    VanadisSyscallUnameEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t uname_info_adr)
        : VanadisSyscallEvent(core, thr, bittype), uname_info_addr(uname_info_adr) {}

    VanadisSyscallOp getOperation() { return SYSCALL_OP_UNAME; }

    uint64_t getUnameInfoAddress() const { return uname_info_addr; }

private:
    uint64_t uname_info_addr;
};

} // namespace Vanadis
} // namespace SST

#endif
