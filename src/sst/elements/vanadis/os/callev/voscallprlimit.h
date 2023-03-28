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

#ifndef _H_VANADIS_SYSCALL_PRLIMIT
#define _H_VANADIS_SYSCALL_PRLIMIT

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallPrlimitEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallPrlimitEvent() : VanadisSyscallEvent() {}
    VanadisSyscallPrlimitEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t pid, uint64_t resource, uint64_t new_limit, uint64_t old_limit )
        : VanadisSyscallEvent(core, thr, bittype), pid(pid), resource(resource), new_limit(new_limit), old_limit(old_limit) {}

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_PRLIMIT; }

    uint64_t getPid() const { return pid; }
    uint64_t getResource() const { return resource; }
    uint64_t getOldLimit() const { return old_limit; }
    uint64_t getNewLimit() const { return new_limit; }

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        ser& pid;
        ser& resource;
        ser& new_limit;
        ser& old_limit;
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallPrlimitEvent);

    uint64_t pid;
    uint64_t resource;
    uint64_t new_limit;
    uint64_t old_limit;
};

} // namespace Vanadis
} // namespace SST

#endif
