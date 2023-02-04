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

#ifndef _H_VANADIS_SYSCALL_FORK
#define _H_VANADIS_SYSCALL_FORK

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallForkEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallForkEvent() : VanadisSyscallEvent() {}
    VanadisSyscallForkEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype)
        : VanadisSyscallEvent(core, thr, bittype) {}

    VanadisSyscallOp getOperation() { return SYSCALL_OP_FORK; }

private:
};

} // namespace Vanadis
} // namespace SST

#endif
