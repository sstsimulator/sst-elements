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

#ifndef _H_VANADIS_SYSCALL_SETTIDADDR
#define _H_VANADIS_SYSCALL_SETTIDADDR

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallSetTidAddressEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallSetTidAddressEvent() : VanadisSyscallEvent() {}
    VanadisSyscallSetTidAddressEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t addr)
        : VanadisSyscallEvent(core, thr, bittype), addr(addr) { }

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_SET_TID_ADDRESS; }

    uint64_t getTidAddress() const { return addr; }

private:

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        ser& addr;
    }

    ImplementSerializable(SST::Vanadis::VanadisSyscallSetTidAddressEvent);

    uint64_t addr;
};

} // namespace Vanadis
} // namespace SST

#endif
