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
    VanadisRegisterStack(const size_t count) : reg_count(count), max_capacity(count)
    {
        regs.reserve(count);

        for(auto i = 0; i < count; ++i) {
            regs.push_back(i);
        }
    }

    ~VanadisRegisterStack() {}

    uint16_t pop()
    {
        uint16_t temp = regs.back();
        regs.pop_back();
        return temp;
    }

    void push(const uint16_t v)
    {
        regs.push_back(v);
    }

    size_t capacity() const { return max_capacity; }

    size_t unused() const { return size(); }

    size_t size() const { return regs.size(); }

    bool full() { return (0 == size()); }
    bool empty() { return size() == capacity(); }

    void clear() { regs.clear(); }

    void print() {
        printf("----> free registers = { ");

        for(size_t i = 0; i < regs.size(); ++i) {
            printf("%" PRIu16 ", ", regs[i]);
        }

        printf("}\n");
    }

private:
    size_t    max_capacity;
    size_t    reg_count;
    std::vector<uint16_t> regs;
};

} // namespace Vanadis
} // namespace SST

#endif
