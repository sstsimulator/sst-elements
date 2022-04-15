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

#ifndef _H_VANADIS_SYSCALL_ACCESS
#define _H_VANADIS_SYSCALL_ACCESS

#include "os/voscallev.h"
#include "os/vosbittype.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallAccessEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallAccessEvent() : VanadisSyscallEvent() {}
    VanadisSyscallAccessEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, const uint64_t path, const uint64_t mode)
        : VanadisSyscallEvent(core, thr, bittype), path_ptr(path), access_mode(mode) {}

    VanadisSyscallOp getOperation() { return SYSCALL_OP_ACCESS; }

    uint64_t getPathPointer() const { return path_ptr; }

    uint64_t getAccessMode() const { return access_mode; }

protected:
    uint64_t path_ptr;
    uint64_t access_mode;
};

} // namespace Vanadis
} // namespace SST

#endif
