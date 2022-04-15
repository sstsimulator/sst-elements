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

namespace SST {
namespace Vanadis {

template <typename T>
class VanadisCircularQueue
{
public:
    VanadisCircularQueue(const size_t size) : max_capacity(size) {}

    ~VanadisCircularQueue() {}

    bool empty() { return 0 == data.size(); }

    bool full() { return max_capacity == data.size(); }

    void push(T item) { data.push_back(item); }

    T peek() { return data.front(); }

    T peekAt(const size_t index) { return data.at(index); }

    T pop()
    {
        T tmp = data.front();
        data.pop_front();
        return tmp;
    }

    size_t size() const { return data.size(); }

    size_t capacity() const { return max_capacity; }

    void clear() { data.clear(); }

    void removeAt(const size_t index)
    {
        auto remove_itr = data.begin();

        for ( size_t i = 0; i < index; ++i, remove_itr++ ) {}

        data.erase(remove_itr);
    }

private:
    const size_t  max_capacity;
    std::deque<T> data;
};

} // namespace Vanadis
} // namespace SST

#endif
