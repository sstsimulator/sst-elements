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

#include "os/syscall/fork.h"
#include "os/vnodeos.h"
#include "os/vgetthreadstate.h"

using namespace SST::Vanadis;

VanadisForkSyscall::VanadisForkSyscall( Output* output, Link* link, OS::ProcessInfo* thread, SendMemReqFunc* func, VanadisSyscallForkEvent* event, VanadisNodeOSComponent* os )
        : VanadisSyscall( output, link, thread, func, event, "fork" ), m_os(os)
{
    m_output->verbose(CALL_INFO, 16, 0, "[syscall-fork]\n");

    // get a new hwThread to run the new process on
    m_threadID = m_os->allocHwThread();

    if ( 0 == m_threadID ) {
        output->fatal(CALL_INFO, -1, "Error: fork, out of hardware threads for new process\n");
    }

    assert( m_os->m_mmu );

    // do this before we create the child becuase the child gets a copy of this the parents page table
    // and we want the child to fault on write as well
    m_os->m_mmu->removeWrite( thread->getpid() );

    // create a new process/thread object with a new pid/tid
    m_child = new OS::ProcessInfo( *thread, m_os->getNewTid() );  

    m_os->m_threadMap[m_child->gettid()] = m_child;

    m_child->setHwThread( *m_threadID );

    // flush the TLB for this hwThread, there shouldn't be any race conditions give the parent is blocked in the syscall 
    m_os->m_mmu->flushTlb( event->getCoreID(), event->getThreadID() );

    // create a page table for the child that is a copy of the parents
    m_os->m_mmu->dup( thread->getpid(), m_child->getpid() );

    m_os->m_mmu->setCoreToPageTable( m_threadID->core, m_threadID->hwThread, m_child->getpid() );

    m_os->m_coreInfoMap.at(m_threadID->core).setProcess( m_threadID->hwThread, m_child );

    m_child->printRegions("child");

    // save this syscall so when we get the response we know what to do with it
    m_os->m_coreInfoMap[ m_event->getCoreID()].setSyscall( m_event->getThreadID(), this );
    m_respLink->send( new VanadisGetThreadStateReq( m_event->getThreadID() ) );
}

// we have the instPtr and registers from the parent core/hwThread, 
void VanadisForkSyscall::complete( VanadisGetThreadStateResp* resp )
{
    m_output->verbose(CALL_INFO, 16, 0, "[syscall-fork] got thread state response\n");

    VanadisStartThreadForkReq* req = new VanadisStartThreadForkReq( m_threadID->hwThread, resp->getInstPtr(), resp->getTlsPtr() );
    req->setIntRegs( resp->intRegs );
    req->setFpRegs( resp->fpRegs );

#if 0 
    printf("%s() %#lx\n", __func__,resp->getInstPtr() );
    printf("thread=%d instPtr=%lx\n",resp->getThread(), resp->getInstPtr() );
    for ( int i = 0; i < resp->intRegs.size(); i++ ) {
        printf("int r%d %" PRIx64 "\n",i,resp->intRegs[i]);
    }
    for ( int i = 0; i < resp->fpRegs.size(); i++ ) {
        printf("int r%d %" PRIx64 "\n",i,resp->fpRegs[i]);
    }
#endif

    m_os->startProcess( m_threadID->core, m_threadID->hwThread, m_child->getpid(), req ); 

    setReturnSuccess(m_child->getpid());
    delete resp;
}
