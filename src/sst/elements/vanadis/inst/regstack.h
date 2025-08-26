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

#ifndef _H_VANADIS_REG_STACK
#define _H_VANADIS_REG_STACK

namespace SST {
namespace Vanadis {

/*
    A stack to manage allocation of registers. Registers on the stack are available.
    The stack holds indices of available registers as a uint16_t.

    The stack begins in 'full' (all registers available) state.
    pop() removes a register from the stack
    push() places a register back on the stack

    USAGE NOTES
     - push() and pop() do not do bounds checks. Caller must check if needed.
*/
class VanadisRegisterStack
{
public:
    // count - number of registers to manage
    VanadisRegisterStack(const size_t count) : max_capacity_(count)
    {
        regs_ = new uint16_t[max_capacity_];
        reset();
    }

    ~VanadisRegisterStack() {
        delete[] regs_;
    }

    // Pop a free register off the stack (reserve/fill it)
    uint16_t pop()
    {
        stack_top_--;
        return regs_[stack_top_];
    }

    // Push a free register back onto the stack, record its ptr
    void push(const uint16_t v)
    {
        regs_[stack_top_] = v;
        stack_top_++;
    }

    // Returns the total number of registers
    size_t capacity() const { return max_capacity_; }

    // Returns how many registers are free/unallocated
    size_t unused() const { return stack_top_; }

    // Returns whether all registers are available (reg stack is full)
    bool full() { return (max_capacity_ == stack_top_); }

    // Returns whether no registers are available (stack is empty)
    bool empty() { return ( max_capacity_ == 0); }

    // For debug
    void print(Output& out) {
        out.output("----> free registers = { ");

        for(size_t i = 0; i < stack_top_; ++i) {
            out.output("%" PRIu16 ", ", regs_[i]);
        }

        out.output("}\n");
    }

private:

    // Reset the register stack to all available
    void reset() {
        stack_top_ = max_capacity_;

        for(auto i = 0; i < max_capacity_; ++i) {
            regs_[i] = i;
        }
    }

    const size_t    max_capacity_; // Total number of registers
    size_t          stack_top_;    // Stack index of the most recently allocated (pop'd) entry

    uint16_t* regs_; // Indices of available registers
};

} // namespace Vanadis
} // namespace SST

#endif
