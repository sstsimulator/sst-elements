// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_SYSCALL_GETRANDOM
#define _H_VANADIS_SYSCALL_GETRANDOM

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallGetrandomEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallGetrandomEvent() : VanadisSyscallEvent() {}
    VanadisSyscallGetrandomEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t buf, uint64_t buflen, uint64_t flags)
        : VanadisSyscallEvent(core, thr, bittype), buf(buf), buflen(buflen), flags(flags) {}

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_GETRANDOM; }

    int64_t getBuf() const { return buf; }
    int64_t getBuflen() const { return buflen; }
    int64_t getFlags() const { return flags; }

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        ser& buf;
        ser& buflen;
        ser& flags;
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallGetrandomEvent);

    uint64_t buf;
    uint64_t buflen;
    uint64_t flags;
};

} // namespace Vanadis
} // namespace SST

#endif
