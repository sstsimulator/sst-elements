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

#ifndef _H_VANADIS_CIRC_Q
#define _H_VANADIS_CIRC_Q

#include <cassert>
#include <cstddef>
#include <deque>
#include <bitset>

namespace SST {
namespace Vanadis {

template <typename T>
class VanadisCircularQueue
{
public:
    VanadisCircularQueue(const int size) : max_capacity(size) {
        data = new T[size];
        clear();

        std::bitset<32> size_bits(size);
        max_power_two = (size_bits.count() == 1);
        bit_mask = max_power_two ? size - 1 : 0;
    }

    ~VanadisCircularQueue() {
        delete[] data;
    }

    bool empty() const { return 0 == count; }
    bool full() const { return max_capacity == count; }

    void push(T item) {
        assert(count < max_capacity);

        data[tail] = item;
        tail = incrementIndex(tail);
        count++;
    }

    T peek() {
        assert(count > 0);

        T peek_me = data[head];
        return peek_me;
    }

    T peekAt(const size_t index) { 
        assert(index < count);
        return data[calculateIndex(index)];
    }

    T pop()
    {
        assert(count > 0);

        T pop_me = data[head];
        head = incrementIndex(head);
        count--;

        return pop_me;
    }

    size_t size() const { return count; }
    size_t capacity() const { return max_capacity; }

    void clear() {
        head = 0;
        tail = 0;
        count = 0;
    }

private:
    int calculateIndex(const int index) const {
        return max_power_two ? (head + index) & bit_mask : (head + index) % max_capacity;
    }

    int incrementIndex(const int index) const {
        return max_power_two ? (index + 1) & bit_mask : (index + 1) % max_capacity;
    }

    const size_t  max_capacity;
    int head;
    int tail;
    int count;

    T* data;

    bool max_power_two;
    int bit_mask;
};

} // namespace Vanadis
} // namespace SST

#endif
