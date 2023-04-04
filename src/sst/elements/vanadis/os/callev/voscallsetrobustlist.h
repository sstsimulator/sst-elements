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

#ifndef _H_VANADIS_SYSCALL_SET_ROBUST_LIST
#define _H_VANADIS_SYSCALL_SET_ROBUST_LIST

#include "os/voscallev.h"
#include "os/vosbittype.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallSetRobustListEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallSetRobustListEvent() : VanadisSyscallEvent() {}
    VanadisSyscallSetRobustListEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t head, uint64_t len)
        : VanadisSyscallEvent(core, thr, bittype), head(head), len(len) {}

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_SET_ROBUST_LIST; }

    uint64_t getHead() const { return head; }
    uint64_t getLen() const { return len; }

private:

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        ser& head;
        ser& len; 
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallSetRobustListEvent);

    uint64_t head;
    uint64_t len;
};

} // namespace Vanadis
} // namespace SST

#endif
