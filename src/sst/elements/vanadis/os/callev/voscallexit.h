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

#ifndef _H_VANADIS_SYSCALL_EXIT
#define _H_VANADIS_SYSCALL_EXIT

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallExitEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallExitEvent() : VanadisSyscallEvent() {}
    VanadisSyscallExitEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t rc)
        : VanadisSyscallEvent(core, thr, bittype), return_code(rc) {}

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_EXIT; }

    uint64_t getExitCode() const { return return_code; }

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        ser& return_code;
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallExitEvent);

    uint64_t return_code;
};

} // namespace Vanadis
} // namespace SST

#endif
