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

#ifndef _H_VANADIS_SYSCALL_EVENT
#define _H_VANADIS_SYSCALL_EVENT

#include "voscallfunc.h"
#include "vosbittype.h"
#include <sst/core/event.h>

namespace SST {
namespace Vanadis {

class VanadisSyscallEvent : public SST::Event {
public:
    VanadisSyscallEvent() : SST::Event(), core_id(0), thread_id(0), inst_ptr(0) { }

    VanadisSyscallEvent(uint32_t core, uint32_t thr)
        : SST::Event(), core_id(core), thread_id(thr), bittage(VanadisOSBitType::VANADIS_OS_32B), inst_ptr(0) {}

    VanadisSyscallEvent(uint32_t core, uint32_t thr, VanadisOSBitType bit_type, uint64_t inst_ptr = 0 )
        : SST::Event(), core_id(core), thread_id(thr), bittage(bit_type), inst_ptr(inst_ptr) {}

    ~VanadisSyscallEvent() {}

    virtual VanadisSyscallOp getOperation() { return SYSCALL_OP_UNKNOWN; };
    uint32_t getCoreID() const { return core_id; }
    uint32_t getThreadID() const { return thread_id; }
    virtual VanadisOSBitType getOSBitType() const { return bittage; }
    uint64_t getInstPtr() const { return inst_ptr; }

protected:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(core_id);
        SST_SER(thread_id);
        SST_SER(bittage);
        SST_SER(inst_ptr);
    }

    ImplementSerializable(SST::Vanadis::VanadisSyscallEvent);

    uint64_t inst_ptr;
    uint32_t core_id;
    uint32_t thread_id;
    VanadisOSBitType bittage;
};

} // namespace Vanadis
} // namespace SST

#endif
