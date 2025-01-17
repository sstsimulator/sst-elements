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

#ifndef _H_VANADIS_OS_SYSCALL_UNMAP
#define _H_VANADIS_OS_SYSCALL_UNMAP

#include "os/syscall/syscall.h"
#include "os/callev/voscallunmap.h"

namespace SST {
namespace Vanadis {

class VanadisUnmapSyscall : public VanadisSyscall {
public:
    VanadisUnmapSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, PhysMemManager* memMgr, VanadisSyscallMemoryUnMapEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "unmap" ) 
    {
        uint64_t address = event->getDeallocationAddress();
        uint64_t length = event->getDeallocationLength();

        m_output->verbose(CALL_INFO, 16, 0, "[syscall-unmap] addr=%#" PRIx64 " lenght=%" PRIu64 "\n",address, length);

        auto threads = process->getThreadList();
        for ( const auto iter : threads) {
            auto thread = iter.second;
            if ( thread ) {
                m_os->getMMU()->flushTlb( thread->getCore(), thread->getHwThread() );
            }
        } 

        m_os->getMMU()->unmap( process->getpid(), address >> m_os->getPageShift(), length/m_os->getPageSize() );

        int ret = process->unmap( address, length );
        if ( ret ) {
            setReturnFail( -ret );
        } else {
            setReturnSuccess( 0 );
        }
    }
};

} // namespace Vanadis
} // namespace SST

#endif
