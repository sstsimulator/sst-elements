// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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
    size_t size() { return m_group.size(); }

    void add( ProcessInfo* thread, int tid ) {

        if ( m_group.find( tid ) != m_group.end() ) {
            assert( m_group[tid] == nullptr );
        }

        m_group[tid] = thread;
    }

    void remove( int tid ) {
        assert ( m_group.find( tid ) != m_group.end() );
        m_group.erase(tid);
    }

    std::map<int,ProcessInfo*>& getThreadList() {
        return  m_group;
    }

private:
    std::map<int,ProcessInfo*> m_group;
};

}
}
}

#endif
