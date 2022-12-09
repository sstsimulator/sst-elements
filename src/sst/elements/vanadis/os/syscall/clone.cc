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

#include "os/syscall/clone.h"
#include "os/vnodeos.h"

using namespace SST::Vanadis;

VanadisCloneSyscall::VanadisCloneSyscall( Output* output, Link* link, OS::ProcessInfo* process, SendMemReqFunc* func, VanadisSyscallCloneEvent* event, VanadisNodeOSComponent* os )
        : VanadisSyscall( output, link, process, func, event, "clone" ), m_os(os)
{
    m_output->verbose(CALL_INFO, 16, 0, "[syscall-clone] threadStackAddr=%#" PRIx64 " flags=%#" PRIx64 " parentTidAddr=%#" PRIx64 " tlsAddr=%#" PRIx64 " callStackAddr=%#" PRIx64 "\n", 
            event->getThreadStackAddr(), event->getFlags(), event->getParentTidAddr(), event->getTlsAddr(), event->getCallStackAddr() );

    // only honor PTHREAD clone config
    assert( event->getFlags() == 0x7d0f00 );
    m_threadID = m_os->allocHwThread();
    if ( nullptr == m_threadID ) {
        output->fatal(CALL_INFO, -1, "Error: clone, out of hardware threads for new thread\n");
    }

    m_newThread = new OS::ProcessInfo;

    // this make an exact copy of the process which implies pointers to classes are the same for both processes
    // this includes things like virtMemMap and fdTable
    *m_newThread = *process;

    // the new thread has the same pid as the parent but it has a unique tid
    m_newThread->settid( m_os->getNewTid() );
    m_newThread->incRefCnts();
    m_newThread->addThread( m_newThread );

    m_os->m_threadMap[m_newThread->gettid()] = m_newThread;

    //printf("pid=%d tid=%d ppid=%d numThreads=%d\n",m_newThread->getpid(), m_newThread->gettid(), m_newThread->getppid(), m_newThread->numThreads() );

    m_os->m_coreInfoMap.at(m_threadID->core).setProcess( m_threadID->hwThread, m_newThread );
    m_os->m_mmu->setCoreToPageTable( m_threadID->core, m_threadID->hwThread, m_newThread->getpid() );

    // we need to get clone() arguments that are not in registers, for clone we have one argument on the clone call stack 
    m_buffer.resize(sizeof(uint32_t));
    readMemory( event->getCallStackAddr()+16, m_buffer );
    m_state = ReadChildTidAddr;
}

void VanadisCloneSyscall::memReqIsDone() {

    switch( m_state ) {
      case ReadChildTidAddr:
        {
            uint64_t childTidAddr = *(uint32_t*)m_buffer.data();
            //printf("%s() childTldAddr=%#" PRIx64 "\n",__func__,childTidAddr);

            m_newThread->setTidAddress( childTidAddr );

            // the entry point for the thread is a fucntion, "int (*fn)(void * arg)", the address of which is passed to to the syscall located
            // in the thread stack, the arg is also located in the thread stack, we need to read it 
            m_buffer.resize( 2 * sizeof(uint32_t));
            readMemory( getEvent<VanadisSyscallCloneEvent*>()->getThreadStackAddr(), m_buffer );

            m_state = ReadThreadStack;
        } break;

      case ReadThreadStack:
        {
            m_threadStartAddr = *(uint32_t*)m_buffer.data();
            m_threadArgAddr = *((uint32_t*)m_buffer.data() + 1);

            //printf("%s() threadStartAddr=%#" PRIx64 " threadArgAddr=%#" PRIx64 "\n",__func__,m_threadStartAddr,m_threadArgAddr);

            m_buffer.resize(sizeof(uint32_t));
            *((uint32_t*)m_buffer.data()) = m_newThread->gettid(); 
            writeMemory( getEvent<VanadisSyscallCloneEvent*>()->getParentTidAddr(), m_buffer );

            m_state = WriteChildTid;
        } break;

      case WriteChildTid:
        {
#if 0
            printf("core=%d hwThead=%d tid=%d stackPtr=%#" PRIx64 " threadStart=%#" PRIx64 " threadArg=%#" PRIx64 "\n",
                m_threadCore, m_threadHwThread, m_process->getTid( m_threadCore, m_threadHwThread),
                getEvent<VanadisSyscallCloneEvent*>()->getThreadStack(), m_threadEntry,m_threadArg);
            printf("%s() %" PRIx64 "\n",__func__,getEvent<VanadisSyscallCloneEvent*>()->getTlsAddr() );
#endif

            m_os->core_links.at(m_threadID->core)->send( 
                new VanadisStartThreadReq(  m_threadID->hwThread, 
                                            getEvent<VanadisSyscallCloneEvent*>()->getThreadStackAddr(),
                                            m_threadStartAddr,
                                            m_threadArgAddr,
                                            getEvent<VanadisSyscallCloneEvent*>()->getTlsAddr() ) 
            );
            setReturnSuccess( m_newThread->gettid() );
        } break;
    }
}
