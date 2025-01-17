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

#include <sst_config.h>

#include "os/syscall/exitgroup.h"
#include "os/vnodeos.h"
#include "os/vgetthreadstate.h"
#include "os/resp/vosexitresp.h"

using namespace SST::Vanadis;

VanadisExitGroupSyscall::VanadisExitGroupSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallExitGroupEvent* event )
    : VanadisSyscall( os, coreLink, process, event, "exitgroup" )
{
    m_output->verbose(CALL_INFO, 16, 0, "[syscall-exitgroup] core=%d thread=%d tid=%d pid=%d exit_code=%" PRIu64 "\n",
            event->getCoreID(), event->getThreadID(), process->gettid(), process->getpid(), event->getExitCode() );

    // given we are terminating all threads in the group, I'm assuming we don't have to write 0 to the tidAddress
    
    auto threads = process->getThreadList();
    for ( const auto thread : threads) {

        auto tmp = thread.second; 
        if ( thread.first == process->gettid() || nullptr == tmp ) {
            continue;
        }

        if ( m_os->getNodeNum() >= 0 ) {
            printf("node=%d pid=%d tid=%d has exited\n",m_os->getNodeNum(), tmp->getpid(), tmp->gettid());
        } else {
            printf("pid=%d tid=%d has exited\n",tmp->getpid(), tmp->gettid());
        }

        unsigned core = tmp->getCore();
        unsigned hwThread = tmp->getHwThread();

        // test the hwthread to halt execution
        coreLink->send( new VanadisExitResponse( -1, hwThread ) ); 

        // remove the process/thread from the map
        os->removeThread( core, hwThread, tmp->gettid() );

        // delete the process/thread
        delete tmp; 
    }

    if ( m_os->getNodeNum() >= 0 ) {
        printf("node=%d pid=%d tid=%d has exited\n",m_os->getNodeNum(), process->getpid(), process->gettid());
    } else {
        printf("pid=%d tid=%d has exited\n",process->getpid(), process->gettid());
    }

    // clear the process from the core/hwThread it was running on
    os->removeThread( event->getCoreID(),event->getThreadID(), process->gettid() );

    setReturnExited();
}
