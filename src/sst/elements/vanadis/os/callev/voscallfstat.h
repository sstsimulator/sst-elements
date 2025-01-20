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

#ifndef _H_VANADIS_SYSCALL_FSTAT
#define _H_VANADIS_SYSCALL_FSTAT

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallFstatEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallFstatEvent() : VanadisSyscallEvent() {}
    VanadisSyscallFstatEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, int fd, uint64_t s_addr)
        : VanadisSyscallEvent(core, thr, bittype), file_id(fd), fstat_struct_addr(s_addr) {}

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_FSTAT; }

    int getFileHandle() const { return file_id; }
    uint64_t getStructAddress() const { return fstat_struct_addr; }

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        ser& file_id;
        ser& fstat_struct_addr;
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallFstatEvent);

    int file_id;
    uint64_t fstat_struct_addr;
};

} // namespace Vanadis
} // namespace SST

#endif
