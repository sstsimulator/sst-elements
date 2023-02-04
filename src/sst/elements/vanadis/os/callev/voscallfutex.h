// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_SYSCALL_FUTEX
#define _H_VANADIS_SYSCALL_FUTEX

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallFutexEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallFutexEvent() : VanadisSyscallEvent() {}
    VanadisSyscallFutexEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t addr, int32_t op, uint32_t val, uint64_t time_addr, uint64_t stack_ptr)
        : VanadisSyscallEvent(core, thr, bittype), addr(addr), op(op), val(val), time_addr(time_addr), stack_ptr(stack_ptr) {}
    VanadisSyscallFutexEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t addr, int32_t op, uint32_t val, uint64_t time_addr,
        uint32_t val2, uint64_t addr2, uint32_t val3)
        : VanadisSyscallEvent(core, thr, bittype), addr(addr), op(op), val(val), time_addr(time_addr), stack_ptr(0), val2(val2), addr2(addr2), val3(val3)  {}


    VanadisSyscallOp getOperation() { return SYSCALL_OP_FUTEX; }

    uint64_t getAddr() const { return addr; }
    uint64_t getAddr2() const { return addr2; }
    int32_t getOp() const { return op; }
    uint32_t getVal() const { return val; }
    uint32_t getVal2() const { return val2; }
    uint32_t getVal3() const { return val3; }
    uint64_t getTimeAddr() const { return time_addr; }
    uint64_t getStackPtr() const { return stack_ptr; }

private:
    uint64_t addr;
    uint64_t addr2;
    int32_t op;
    uint32_t val;
    uint32_t val2;
    uint32_t val3;

    uint64_t time_addr;
    uint64_t stack_ptr;
};

} // namespace Vanadis
} // namespace SST

#endif
