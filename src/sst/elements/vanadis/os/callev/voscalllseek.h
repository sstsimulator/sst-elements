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

#ifndef _H_VANADIS_SYSCALL_LSEEK
#define _H_VANADIS_SYSCALL_LSEEK

#include "os/callev/voscalliovec.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallLseekEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallLseekEvent() : VanadisSyscallEvent() {}
    VanadisSyscallLseekEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, int64_t fd, uint64_t offset, int64_t whence)
        : VanadisSyscallEvent(core, thr, bittype), fd(fd), offset(offset), whence(whence) {}

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_LSEEK; }
    int32_t getFileDescriptor() const { return fd; }
    int64_t getOffset() const { return offset; }
    int32_t getWhence() const { return whence; }

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        SST_SER(fd);
        SST_SER(offset);
        SST_SER(whence);
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallLseekEvent);

    int64_t fd;
    int64_t offset;
    int64_t whence;
};

} // namespace Vanadis
} // namespace SST

#endif
