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

#ifndef _H_SHOGUN_Q
#define _H_SHOGUN_Q

namespace SST {
namespace Shogun {

    template <typename T>
    class ShogunQueue {

    public:
        ShogunQueue(const int buffSize)
            : buffMax(buffSize)
        {
            queue = new T[buffSize];
            clear();
        }

        ~ShogunQueue()
        {
            delete[] queue;
        }

        bool empty() const
        {
            return size == 0;
        }

        void clear()
        {
            head = 0;
            tail = 0;
            size = 0;
        }

        bool full() const
        {
            return (size == buffMax);
        }

        bool hasNext() const
        {
            return !empty();
        }

        T peek() const
        {
            T item = queue[head];
            return item;
        }

        T pop()
        {
            T item = queue[head];
            incHead();

            size--;
            return item;
        }

        void push(T newItem)
        {
            queue[tail] = newItem;
            incTail();
            size++;
        }

        int capacity() const
        {
            return buffMax;
        }

        int count() const
        {
            return size;
        }

    private:
        int head;
        int tail;
        int size;
        ;
        T* queue;
        const int buffMax;

        void incHead()
        {
            head = nextHead();
        }

        void incTail()
        {
            tail = nextTail();
        }

        int nextHead()
        {
            return (head + 1) % buffMax;
        }

        int nextTail() const
        {
            return (tail + 1) % buffMax;
        }
    };

}
}

#endif
