#ifndef COMPONENTS_FIREFLY_THINGHEAP_H
#define COMPONENTS_FIREFLY_THINGHEAP_H

template < class T>
class ThingHeap {

  public:

    ThingHeap() : m_pos(0), m_heap(256) {
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

template < class T>
class ThingQueue {

  public:

    ThingQueue() : m_pos(0), m_heap(256) {
    }

    void push( T* ) {
#if 0
        if ( m_pos > 0 ) {
            ev = m_heap[--m_pos];
        } else {
            ev = new T;
        }
        return ev;
#endif
    }

    T* pop( ) {
#if 0
        if ( m_pos == m_heap.size() ) {
            m_heap.resize( m_heap.size() + 256 );
        }
        m_heap[m_pos++] = ev;
#endif
    }

  private:
    std::vector<T*> m_heap;
    int m_pos;
};


#endif
