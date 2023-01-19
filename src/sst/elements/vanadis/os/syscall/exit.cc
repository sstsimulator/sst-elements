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

#include <sst_config.h>

#include "os/syscall/exit.h"
#include "os/vnodeos.h"
#include "os/vgetthreadstate.h"
#include "os/resp/vosexitresp.h"

using namespace SST::Vanadis;

VanadisExitSyscall::VanadisExitSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallExitEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "exit" )
{
    m_output->verbose(CALL_INFO, 16, 0, "[syscall-exit] core %d thread %d process %d\n",event->getCoreID(), event->getThreadID(), process->getpid() );

    printf("pid=%d tid=%d has exited\n",process->getpid(), process->gettid());
    uint64_t tid_addr = process->getTidAddress();
    if ( tid_addr ) {
        // this has to be the size of pid_t
        m_buffer.resize(sizeof(uint32_t));
        *((uint32_t*)m_buffer.data()) = 0;
        writeMemory( tid_addr, m_buffer );
    } else {
        memReqIsDone();
    }
}

void VanadisExitSyscall::memReqIsDone() {
    m_os->removeThread( getEvent<VanadisSyscallExitEvent*>()->getCoreID(),getEvent<VanadisSyscallExitEvent*>()->getThreadID(), m_process->gettid() );

    m_output->verbose(CALL_INFO, 16, 0, "[syscall-exit] %s() called\n",__func__ );
    setReturnExited();
}
