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

#ifndef _H_VANADIS_SYSCALL_FSTATAT
#define _H_VANADIS_SYSCALL_FSTATAT

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallFstatAtEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallFstatAtEvent() : VanadisSyscallEvent() {}
    VanadisSyscallFstatAtEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t dirfd, uint64_t pathname, uint64_t statbuf, int64_t flags)
        : VanadisSyscallEvent(core, thr, bittype), dirfd(dirfd), pathname(pathname), statbuf(statbuf), flags(flags) {} 
        
    VanadisSyscallOp getOperation() override { return SYSCALL_OP_FSTATAT; }

    uint64_t getDirfd() const { return dirfd; }
    uint64_t getPathname() const { return pathname; }
    uint64_t getStatbuf() const { return statbuf; }
    int64_t getFlags() const { return flags; }

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        SST_SER(dirfd);
        SST_SER(pathname);
        SST_SER(statbuf);
        SST_SER(flags);
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallFstatAtEvent);

    uint64_t dirfd; 
    uint64_t pathname;
    uint64_t statbuf;
    int64_t  flags;
};

} // namespace Vanadis
} // namespace SST

#endif
