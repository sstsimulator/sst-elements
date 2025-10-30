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

#ifndef _H_VANADIS_SYSCALL_CLONE3
#define _H_VANADIS_SYSCALL_CLONE3

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallClone3Event : public VanadisSyscallEvent {
public:
    VanadisSyscallClone3Event() : VanadisSyscallEvent() {}

    VanadisSyscallClone3Event(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t instPtr, uint64_t cl_args_addr,
                                uint64_t cl_args_size )
        : VanadisSyscallEvent(core, thr, bittype, instPtr), cl_args(cl_args_addr), size(cl_args_size) {}

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_CLONE3; }

    uint64_t getCloneArgsAddr() const { return cl_args; }
    uint64_t getCloneArgsSize() const { return size; }

private:

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        SST_SER(cl_args);
        SST_SER(size);
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallClone3Event);

    uint64_t cl_args;
    uint64_t size;
};

} // namespace Vanadis
} // namespace SST

#endif
