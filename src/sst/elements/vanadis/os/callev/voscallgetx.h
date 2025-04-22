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

#ifndef _H_VANADIS_SYSCALL_GET_X
#define _H_VANADIS_SYSCALL_GET_X

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallGetxEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallGetxEvent() : VanadisSyscallEvent() {}
    VanadisSyscallGetxEvent( uint32_t core, uint32_t thr, VanadisOSBitType bittype, VanadisSyscallOp op )
        : VanadisSyscallEvent(core, thr, bittype ), m_op(op) {}

    VanadisSyscallOp getOperation() override { return m_op; }

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        SST_SER(m_op);
    }

    ImplementSerializable(SST::Vanadis::VanadisSyscallGetxEvent);

    VanadisSyscallOp m_op;
};

} // namespace Vanadis
} // namespace SST

#endif
