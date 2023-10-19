// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
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
#include "os/vgetthreadstate.h"

using namespace SST::Vanadis;

VanadisCloneSyscall::VanadisCloneSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallCloneEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "clone" )
{
    m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL,
        "[syscall-clone] threadStackAddr=%#" PRIx64 " flags=%#" PRIx64 " parentTidAddr=%#" PRIx64 " tlsAddr=%#" PRIx64 " callStackAddr=%#" PRIx64 "\n", 
        event->getThreadStackAddr(), event->getFlags(), event->getParentTidAddr(), event->getTlsAddr(), event->getCallStackAddr() );

    // only honor PTHREAD clone config
    assert( event->getFlags() == 0x7d0f00 );
    m_threadID = m_os->allocHwThread();
    if ( nullptr == m_threadID ) {
        m_output->fatal(CALL_INFO, -1, "Error: clone, out of hardware threads for new thread\n");
    }

    m_newThread = new OS::ProcessInfo;

    // this make an exact copy of the process which implies pointers to classes are the same for both processes
    // this includes things like virtMemMap and fdTable
    *m_newThread = *process;

    // the new thread has the same pid as the parent but it has a unique tid
    m_newThread->settid( m_os->getNewTid() );
    m_newThread->incRefCnts();
    m_newThread->addThread( m_newThread );

    m_newThread->setHwThread( *m_threadID );

    m_os->setThread( m_newThread->gettid(), m_newThread );

    m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL, "[syscall-clone] newthread pid=%d tid=%d ppid=%d numThreads=%d\n",
            m_newThread->getpid(), m_newThread->gettid(), m_newThread->getppid(), m_newThread->numThreads() );

    m_os->setProcess( m_threadID->core, m_threadID->hwThread, m_newThread ); 
    m_os->getMMU()->setCoreToPageTable( m_threadID->core, m_threadID->hwThread, m_newThread->getpid() );

    // if that stackAddr is valid  we need to get clone() arguments that are not in registers, for clone we have one argument on the clone call stack 
    if ( event->getCallStackAddr() ) {
        // this has to be the size of pointer to pid_t
        if ( event->getOSBitType() == VanadisOSBitType::VANADIS_OS_32B ) {
            m_buffer.resize(sizeof(uint32_t));
        } else {
            m_buffer.resize(sizeof(uint64_t));
        }

        readMemory( event->getCallStackAddr()+16, m_buffer );

        m_state = ReadChildTidAddr;
    } else {
        m_newThread->setTidAddress( event->getchildTidAddr());

        m_state = ReadThreadStack;
        // this has to be the size of function pointer and stack pointer
        if ( event->getOSBitType() == VanadisOSBitType::VANADIS_OS_32B ) {
            m_buffer.resize( 2 * sizeof(uint32_t));
        } else {
            m_buffer.resize( 2 * sizeof(uint64_t));
        }
        readMemory( event->getThreadStackAddr(), m_buffer );
    }
}

// we have the instPtr and registers from the parent core/hwThread,
void VanadisCloneSyscall::handleEvent( VanadisCoreEvent* ev )
{
    auto resp = dynamic_cast<VanadisGetThreadStateResp*>( ev );
    assert(resp);

    m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL, "[syscall-clone] got thread state response\n");

    VanadisStartThreadCloneReq* req =  new VanadisStartThreadCloneReq( m_threadID->hwThread, 
                                    m_threadStartAddr,
                                    getEvent<VanadisSyscallCloneEvent*>()->getThreadStackAddr(),
                                    getEvent<VanadisSyscallCloneEvent*>()->getTlsAddr(),
                                    m_threadArgAddr );

    req->setIntRegs( resp->intRegs );
    req->setFpRegs( resp->fpRegs );

     m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL, "[syscall-clone] core=%d thread=%d tid=%d instPtr=%" PRI_ADDR "\n",
                 m_threadID->core, m_threadID->hwThread, m_newThread->gettid(), resp->getInstPtr() );
#if 0 //debug
    for ( int i = 0; i < req->getIntRegs().size(); i++ ) {
        printf("int r%d %" PRIx64 "\n",i,req->getIntRegs()[i]);
    }
    printf("%s() %lx\n", __func__,m_newThread->getTidAddress());
#endif

    m_os->sendEvent( m_threadID->core, req ); 

    setReturnSuccess( m_newThread->gettid() );

    delete resp;
}

void VanadisCloneSyscall::memReqIsDone(bool) {

    switch( m_state ) {
      case ReadChildTidAddr:
        {
            uint64_t childTidAddr = *(uint32_t*)m_buffer.data();

            m_newThread->setTidAddress( childTidAddr );

            // the entry point for the thread is a fucntion, "int (*fn)(void * arg)", the address of which is passed to to the syscall located
            // in the thread stack, the arg is also located in the thread stack, we need to read it 
            if ( getEvent<VanadisSyscallCloneEvent*>()->getOSBitType() == VanadisOSBitType::VANADIS_OS_32B ) {
                m_buffer.resize( 2 * sizeof(uint32_t) );
            } else {    
                assert(0);
            }
            readMemory( getEvent<VanadisSyscallCloneEvent*>()->getThreadStackAddr(), m_buffer );

            m_state = ReadThreadStack;
        } break;

      case ReadThreadStack:
        {
            // this has to be the size of function pointer and stack pointer
            if ( getEvent<VanadisSyscallCloneEvent*>()->getOSBitType() == VanadisOSBitType::VANADIS_OS_32B ) {
                m_threadStartAddr = *(uint32_t*)m_buffer.data();
                m_threadArgAddr = *((uint32_t*)m_buffer.data() + 1);
            } else {
                m_threadStartAddr = *(uint64_t*)m_buffer.data();
                m_threadArgAddr = *((uint64_t*)m_buffer.data() + 1);
            } 

             m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL,
                "[syscall-clone] threadStartAddr=%#" PRIx64 " threadArgAddr=%#" PRIx64 "\n", m_threadStartAddr,m_threadArgAddr);

            // this has to be size of pid_t
            if ( getEvent<VanadisSyscallCloneEvent*>()->getOSBitType() == VanadisOSBitType::VANADIS_OS_32B ) {
                m_buffer.resize(sizeof(uint32_t));
                *((uint32_t*)m_buffer.data()) = m_newThread->gettid(); 
            } else {
                m_buffer.resize(sizeof(uint32_t));
                *((uint32_t*)m_buffer.data()) = m_newThread->gettid(); 
            }
            writeMemory( getEvent<VanadisSyscallCloneEvent*>()->getParentTidAddr(), m_buffer );

            m_state = WriteChildTid;
        } break;

      case WriteChildTid:
        {
            sendNeedThreadState();
        } break;
    }
}
