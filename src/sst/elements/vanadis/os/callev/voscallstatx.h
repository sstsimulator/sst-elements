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

#ifndef _H_VANADIS_SYSCALL_STATX
#define _H_VANADIS_SYSCALL_STATX

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallStatxEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallStatxEvent() : VanadisSyscallEvent() {}
    VanadisSyscallStatxEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, int64_t dirFd, uint64_t path_ptr, uint64_t flags, uint32_t mask, uint64_t stackPtr )
        : VanadisSyscallEvent(core, thr, bittype), dirFd(dirFd), path_ptr(path_ptr),flags(flags), mask(mask), stackPtr(stackPtr) {}

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_STATX; }

    uint64_t getPathPointer() const { return path_ptr; }
    uint64_t getDirectoryFileDescriptor() const { return dirFd; }
    uint64_t getFlags() const { return flags; }
    uint32_t getMask() const { return mask; }
    uint64_t getStackPtr() const { return stackPtr; }

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        ser& dirFd;
        ser& path_ptr;
        ser& flags;
        ser& mask;
        ser& stackPtr;
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallStatxEvent);

    uint64_t dirFd;
    uint64_t path_ptr;
    uint64_t flags;
    uint32_t mask;
    uint64_t stackPtr;
};

} // namespace Vanadis
} // namespace SST

#endif
