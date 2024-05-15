// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_NODE_OS_INCLUDE_FUTEX
#define _H_VANADIS_NODE_OS_INCLUDE_FUTEX

#include <string>

#include <map>
namespace SST {
namespace Vanadis {


#if 0
#define OSFutexDbg( format, ... ) printf( "OS Futex::%s() " format, __func__, ##__VA_ARGS__ )
#else
#define OSFutexDbg( ... )
#endif

class VanadisSyscall;

namespace OS {

class Futex {
public:
    void addWait( uint64_t addr, VanadisSyscall* syscall ) {
        OSFutexDbg("Futex::%s() addr=%#" PRIx64 " syscall=%p\n", addr,syscall );

        auto& tmp = m_futexMap[addr]; 

        for ( auto iter = tmp.begin(); iter != tmp.end(); ++iter ) {
            assert( *iter != syscall );
        }

        tmp.push_back( syscall );
        OSFutexDbg("Futex::%s() %p %zu\n",this,tmp.size());
    }

    size_t getNumWaiters( uint64_t addr ) {
        OSFutexDbg("Futex::%s() addr=%#" PRIx64 "\n", addr );
        if ( m_futexMap.find( addr ) == m_futexMap.end() ) {
            return 0;
        }
        auto& tmp = m_futexMap[addr]; 
        OSFutexDbg("Futex::%s() %p %zu\n",this,tmp.size());
        return tmp.size();
    }
    VanadisSyscall* findWait( uint64_t addr ) {
        OSFutexDbg("Futex::%s() addr=%#" PRIx64 "\n", addr );
        if (  m_futexMap.find( addr ) == m_futexMap.end() ) {
            return nullptr;
        }
        
        auto& tmp = m_futexMap[addr]; 

        OSFutexDbg("Futex::%s() %p %zu\n",this,tmp.size());
        assert( ! tmp.empty() );

        auto syscall = tmp.front();

        tmp.pop_front( );
        if ( tmp.empty() ) {
            m_futexMap.erase( addr );
        }
        OSFutexDbg("Futex::%s() found addr=%#" PRIx64 " syscall=%p\n", addr,syscall );
        return syscall;
    }
    bool isEmpty() { return m_futexMap.empty(); } 

private:
    std::map< uint64_t, std::deque<VanadisSyscall* > > m_futexMap;
};

}
}
}

#endif
