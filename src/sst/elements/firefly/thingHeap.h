// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_THINGHEAP_H
#define COMPONENTS_FIREFLY_THINGHEAP_H

template < class T>
class ThingHeap {

  public:

    ThingHeap() : m_pos(0), m_heap(256) {
    }
	~ThingHeap() {
#if 0
		//for some reason this causes a fault
		//could it be some interaction problem with the mempool?
		for ( size_t i = 0; i < m_heap.size(); i++ ) {
			delete m_heap[i];
		}
#endif
	}

    T* alloc() {
        T* ev;
        if ( m_pos > 0 ) {
            ev = m_heap[--m_pos];
        } else {
            ev = new T;
        }
        return ev;
    }

    void free( T* ev ) {
        if ( m_pos == m_heap.size() ) {
            m_heap.resize( m_heap.size() + 256 );
        }
        m_heap[m_pos++] = ev;
    }

  private:
    std::vector<T*> m_heap;
    int m_pos;
};

#endif
