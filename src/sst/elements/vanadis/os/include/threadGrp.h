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

#ifndef _H_VANADIS_NODE_OS_INCLUDE_THREAD_GRP
#define _H_VANADIS_NODE_OS_INCLUDE_THREAD_GRP

#include <string>

#include <set>
namespace SST {
namespace Vanadis {

namespace OS {

class ProcessInfo;

class ThreadGrp {
public:
    size_t size() { return m_numThreads; }

    void add( ProcessInfo* thread, int tid ) {

        if ( m_group.find( tid ) != m_group.end() ) {
            assert( m_group[tid] == nullptr );
        }

        m_group[tid] = thread ;
        ++m_numThreads;
    }

    void remove( int tid ) {
        assert ( m_group.find( tid ) != m_group.end() );
        m_group[tid] = nullptr;

        --m_numThreads;
    }

    std::map<int,ProcessInfo*>& getThreadList() {
        return  m_group;
    }

private:
    std::map<int,ProcessInfo*> m_group;
    size_t m_numThreads;
};

}
}
}

#endif
