
#include "cacheList.h"

class Cache {
  public:
    Cache( int cacheSize ) : m_cacheSize( cacheSize ) {
        for ( int i = 0; i < cacheSize; i++ ){
            insert( -1 );    
        }
    }

    void flush() {
        m_addrMap.clear();
        m_ageList.clear();
    }

    bool isValid( Hermes::Vaddr addr ) {
        return m_addrMap.find(addr) != m_addrMap.end();
    }

    void updateAge( Hermes::Vaddr addr ) {
        m_ageList.move_to_back( m_addrMap.find(addr)->second );
        m_addrMap.find( addr )->second = m_ageList.end();
    }

    Hermes::Vaddr evict() {
        Hermes::Vaddr addr = m_ageList.get_front_value();
        //printf("%s(%p) return %lx %lu\n", __func__, this, addr, m_addrMap.size());

        m_addrMap.erase( m_addrMap.find(addr) );

        m_ageList.pop_front();
        //printf("%s(%p) return %lx %lu\n", __func__, this, addr, m_addrMap.size());
        return addr;
    }

    void insert( Hermes::Vaddr addr ) {
        //printf("%s(%p) %lx %lu\n", __func__, this, addr, m_addrMap.size());
        if ( addr != - 1 ) {
            assert( m_addrMap.find(addr) == m_addrMap.end() );
        }
        assert( m_addrMap.size() < m_cacheSize );
        m_ageList.push_back( addr );
        m_addrMap.insert( std::make_pair(addr, m_ageList.end() ) );
    }
  private:
    int m_cacheSize;
    List<Hermes::Vaddr> m_ageList;
    std::unordered_multimap<Hermes::Vaddr, List<Hermes::Vaddr>::Entry > m_addrMap;
};
