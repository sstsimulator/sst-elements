
#include "cacheList.h"

class Cache {
  public:
    Cache( int cacheSize ) : m_cacheSize( cacheSize ) {}

    void flush() {
        m_addrMap.clear();
        m_ageList.clear();
    }

    bool isValid( Hermes::Vaddr addr ) {
        return m_addrMap.find(addr) != m_addrMap.end();
    }

    void updateAge( Hermes::Vaddr addr ) {
        m_ageList.move_to_back( m_addrMap[addr] );
        m_addrMap[addr] = m_ageList.end();
    }

    Hermes::Vaddr evict() {
        if ( ! m_addrMap.empty() ) {
            Hermes::Vaddr addr = m_ageList.get_front_value();

            m_addrMap.erase( addr );
            m_ageList.pop_front();
            return addr;
        } else {
            return -1;
        }
    }

    void insert( Hermes::Vaddr addr ) {
        assert( m_addrMap.find(addr) == m_addrMap.end() );
        assert( m_addrMap.size() < m_cacheSize );
        m_ageList.push_back( addr );
        m_addrMap[addr] = m_ageList.end();
    }
  private:
    int m_cacheSize;
    List<Hermes::Vaddr> m_ageList;
    std::map<Hermes::Vaddr, List<Hermes::Vaddr>::Entry > m_addrMap;
};
