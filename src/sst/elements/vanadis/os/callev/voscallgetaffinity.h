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

#ifndef _H_VANADIS_SYSCALL_SCHED_GET_AFFINITY
#define _H_VANADIS_SYSCALL_SCHED_GET_AFFINITY

#include "os/voscallev.h"
#include "os/vosbittype.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallGetaffinityEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallGetaffinityEvent() : VanadisSyscallEvent() {}
    VanadisSyscallGetaffinityEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, int64_t pid, int64_t cpusetsize, uint64_t maskAddr )
        : VanadisSyscallEvent(core, thr, bittype), pid(pid), cpusetsize(cpusetsize), maskAddr(maskAddr) {}

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_SCHED_GETAFFINITY; }

    int64_t getPid() const { return pid; }
    int64_t getCpusetsize() const { return cpusetsize; }
    int64_t getMaskAddr() const { return maskAddr; }

private:

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        SST_SER(pid);
        SST_SER(cpusetsize);
        SST_SER(maskAddr);
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallGetaffinityEvent);

    int64_t pid;
    int64_t cpusetsize;
    uint64_t maskAddr;
};

} // namespace Vanadis
} // namespace SST

#endif
