#ifndef COMPONENTS_FIREFLY_THINGHEAP_H
#define COMPONENTS_FIREFLY_THINGHEAP_H

template < class T>
class ThingHeap {

  public:

    ThingHeap() : m_pos(0), m_heap(256) {
    }
	~ThingHeap() {
		for ( size_t i = 0; i < m_heap.size(); i++ ) {
			delete m_heap[i];
		}
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
