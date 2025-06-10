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

#ifndef _H_VANADIS_SYSCALL_UNLINK
#define _H_VANADIS_SYSCALL_UNLINK

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallUnlinkEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallUnlinkEvent() : VanadisSyscallEvent() {}
    VanadisSyscallUnlinkEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t path_ptr)
        : VanadisSyscallEvent(core, thr, bittype), path_ptr(path_ptr) {}

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_UNLINK; }

    uint64_t getPathPointer() const { return path_ptr; }

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        SST_SER(path_ptr);
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallUnlinkEvent);
    uint64_t path_ptr;
};

} // namespace Vanadis
} // namespace SST

#endif
