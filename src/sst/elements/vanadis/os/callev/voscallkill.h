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

#ifndef _H_VANADIS_SYSCALL_KILL
#define _H_VANADIS_SYSCALL_KILL

#include "os/voscallev.h"
#include "os/vosbittype.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallKillEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallKillEvent() : VanadisSyscallEvent() {}
    VanadisSyscallKillEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t pid, uint64_t sig)
        : VanadisSyscallEvent(core, thr, bittype), pid(pid), sig(sig) {}

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_KILL; }

    uint64_t getPid() const { return pid; }
    uint64_t getSig() const { return sig; }

private:

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        SST_SER(pid);
        SST_SER(sig);
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallKillEvent);

    uint64_t pid;
    uint64_t sig;
};

} // namespace Vanadis
} // namespace SST

#endif
