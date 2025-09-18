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

#ifndef _H_VANADIS_SYSCALL_TGKILL
#define _H_VANADIS_SYSCALL_TGKILL

#include "os/voscallev.h"
#include "os/vosbittype.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallTgKillEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallTgKillEvent() : VanadisSyscallEvent() {}
    VanadisSyscallTgKillEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, int tgid, int tid, int sig)
        : VanadisSyscallEvent(core, thr, bittype), tgid(tgid), tid(tid), sig(sig) {}

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_TGKILL; }

    int getTgid() const { return tgid; }
    int getTid() const { return tid; }
    int getSig() const { return sig; }

private:

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        SST_SER(tgid);
        SST_SER(tid);
        SST_SER(sig);
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallTgKillEvent);

    int tgid;
    int tid;
    int sig;
};

} // namespace Vanadis
} // namespace SST

#endif
