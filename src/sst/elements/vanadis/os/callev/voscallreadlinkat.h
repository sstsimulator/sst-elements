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

#ifndef _H_VANADIS_SYSCALL_READLINKAT
#define _H_VANADIS_SYSCALL_READLINKAT

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallReadLinkAtEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallReadLinkAtEvent() : VanadisSyscallEvent() {}
    VanadisSyscallReadLinkAtEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t dirfd, uint64_t pathname, uint64_t buf, int64_t bufsize)
        : VanadisSyscallEvent(core, thr, bittype), dirfd(dirfd), pathname(pathname), buf(buf), bufsize(bufsize) {} 
        
    VanadisSyscallOp getOperation() override { return SYSCALL_OP_READLINKAT; }

    uint64_t getDirfd() const { return dirfd; }
    uint64_t getPathname() const { return pathname; }
    uint64_t getBuf() const { return buf; }
    int64_t getBufsize() const { return bufsize; }

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        ser& dirfd;
        ser& pathname;
        ser& buf;
        ser& bufsize;
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallReadLinkAtEvent);

    uint64_t dirfd; 
    uint64_t pathname;
    uint64_t buf;
    int64_t  bufsize;
};

} // namespace Vanadis
} // namespace SST

#endif
