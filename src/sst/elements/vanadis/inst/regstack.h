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

#ifndef _H_VANADIS_REG_STACK
#define _H_VANADIS_REG_STACK

namespace SST {
namespace Vanadis {

class VanadisRegisterStack
{
public:
    VanadisRegisterStack(const size_t count) : max_capacity(count)
    {
        regs = new uint16_t[max_capacity];
        reset();
    }

    ~VanadisRegisterStack() {
        delete[] regs;
    }

    uint16_t pop()
    {
/*
        if(stack_top < 0) {
            printf("LOGIC-ERROR - stack_top=%" PRId32 " / capacity=%" PRId32 "\n", stack_top, max_capacity);
            int32_t* address = 0;
            *address = 0;
            printf("address=%" PRId32 "\n", *address);
        }
        assert(stack_top >= 0);
*/
        const uint16_t temp = regs[stack_top];
        stack_top--;
        return temp;
    }

    void push(const uint16_t v)
    {
/*
        if(stack_top >= max_capacity) {
            printf("LOGIC-ERROR - stack_top=%" PRId32 " / capacity=%" PRId32 "\n", stack_top, max_capacity);
            int32_t* address = 0;
            *address = 0;
            printf("address=%" PRId32 "\n", *address);
        } else {
            printf("-> stack_top=%" PRId32 " / capacity=%" PRId32 "\n", stack_top, max_capacity);
        }
        assert(stack_top < max_capacity);
*/
        stack_top++;
        regs[stack_top] = v;
    }

    size_t capacity() const { return max_capacity; }
    size_t unused() const { return (stack_top > 0) ? stack_top : 0; }

    bool full() { return (stack_top == (max_capacity - 1)); }
    bool empty() { return -1 == stack_top; }

    void clear() {
        stack_top = -1;
    }

    void reset() {
        stack_top = max_capacity - 1;

        for(auto i = 0; i < max_capacity; ++i) {
            regs[i] = i;
        }
    }

    void print() {
        printf("----> free registers = { ");

        for(size_t i = 0; i < stack_top; ++i) {
            printf("%" PRIu16 ", ", regs[i]);
        }

        printf("}\n");
    }

private:
    const int32_t    max_capacity;
    int32_t          stack_top;
    
    uint16_t* regs;
};

} // namespace Vanadis
} // namespace SST

#endif
