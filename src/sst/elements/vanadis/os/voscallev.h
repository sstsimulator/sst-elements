// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
    VanadisSyscallEvent() : SST::Event() {
        core_id = 0;
        thread_id = 0;
    }

    VanadisSyscallEvent(uint32_t core, uint32_t thr) : SST::Event(), core_id(core), thread_id(thr),
			bittage(VanadisOSBitType::VANADIS_OS_32B) {}
    VanadisSyscallEvent(uint32_t core, uint32_t thr, VanadisOSBitType bit_type) : SST::Event(), core_id(core), thread_id(thr),
			bittage(bit_type) {}

    ~VanadisSyscallEvent() {}

    virtual VanadisSyscallOp getOperation() { return SYSCALL_OP_UNKNOWN; };
    uint32_t getCoreID() const { return core_id; }
    uint32_t getThreadID() const { return thread_id; }
    virtual VanadisOSBitType getOSBitType() const { return bittage; }

protected:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        ser& core_id;
        ser& thread_id;
    }

    ImplementSerializable(SST::Vanadis::VanadisSyscallEvent);

    uint32_t core_id;
    uint32_t thread_id;
	 VanadisOSBitType bittage;
};

} // namespace Vanadis
} // namespace SST

#endif
