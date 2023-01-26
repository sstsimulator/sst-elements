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
    ThreadGrp( ProcessInfo* thread ) {
        m_group.insert( thread );
    }

    size_t size() { return m_group.size(); }

    void add( ProcessInfo* thread ) {
        m_group.insert( thread );
    }

    void remove( ProcessInfo* thread ) {
        m_group.erase( thread );
    }

    ProcessInfo* getThread( ProcessInfo* thread) {
        auto iter = m_group.begin();
        for ( ; iter != m_group.end(); ++iter ) {
            if ( *iter != thread ) {
                auto ret = *iter;
                m_group.erase(iter);
                return ret;
            }
        }
        return nullptr;
    } 

private:
    std::set<ProcessInfo*> m_group;
};

}
}
}

#endif
