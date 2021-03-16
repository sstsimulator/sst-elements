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

class NWayCache {
public:
    NWayCache( int assoc, uint32_t nSets, int pageSize ) : m_pageShift(0), m_setShift(0), m_setMask( nSets-1 ) {
        //printf("%s():%d assoc=%d nSets=%d pageSize=%d setMask=%#lx\n",__func__,__LINE__,assoc, nSets, pageSize,m_setMask);
        for ( unsigned i = 0; i < nSets; i++ ) {
            m_sets.push_back( new Cache( assoc ) );
            m_stats.push_back( std::make_pair(0,0) );
        }
        //printf("%s():%d %#x\n",__func__,__LINE__,pageSize);

        m_pageShift = calcPow( pageSize );
        m_setShift = calcPow( nSets );

        printf("%s():%d m_setMask=%" PRIx64 " m_pageShift=%d m_setShift=%d\n",__func__,__LINE__,m_setMask,m_pageShift,m_setShift);
    }

    bool isValid( Hermes::Vaddr addr ) {
        int set = addrToSet(addr);
        uint64_t tag = addrToTag( addr );
        printf("%s():%d %#" PRIx64 " set=%#x tag=%#" PRIx64 "  \n",__func__,__LINE__,addr, set, tag );
        ++m_stats[set].first;
        bool rc = m_sets[set]->isValid( addr );
        if ( rc ) {
            ++m_stats[set].second;
        }
        return rc;
    }

    void updateAge( Hermes::Vaddr addr ) {
        int set = addrToSet(addr);
        uint64_t tag = addrToTag( addr );
        m_sets[set]->updateAge( addr );
    }

    Hermes::Vaddr evict( Hermes::Vaddr addr ) {
        int set = addrToSet(addr);
        uint64_t tag = addrToTag( addr );
        return m_sets[set]->evict( );
    }

    void insert( Hermes::Vaddr addr ) {
        int set = addrToSet(addr);
        uint64_t tag = addrToTag( addr );
        m_sets[set]->insert( addr );
    }

    void printStats( Output& output ) {
        for ( unsigned i = 0; i < m_stats.size(); i++ ) {
            if ( m_stats[i].first ) {
            output.output("set=%d: total=%" PRIi64 " %f hits\n", i, m_stats[i].first, (float) m_stats[i].second / (float)m_stats[i].first );
            }
        }
    }

private:
    uint64_t m_setMask;
    int m_pageShift;
    int m_setShift;
    std::vector<Cache*> m_sets;
    std::vector<std::pair<uint64_t,uint64_t> > m_stats;

    int calcPow( double val ) {
        int x = 0;
        while( val > 1 ) {
            val /= 2;
            ++x;
        }
        assert( val == 1.0 );
        return x;
    }

    int addrToSet( Hermes::Vaddr addr ) {
        int set = (addr >> m_pageShift) & m_setMask;
        //printf("%s():%d %#lx set=%#lx\n",__func__,__LINE__,addr,set);
        return set;
    }

    uint64_t addrToTag( Hermes::Vaddr addr ) {
        int tag = (addr >> (m_pageShift + m_setShift));
        //printf("%s():%d %#lx tag=%#lx\n",__func__,__LINE__,addr,tag);
        return tag;
    }
};
