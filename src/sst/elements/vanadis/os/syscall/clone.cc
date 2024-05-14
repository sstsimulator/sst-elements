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

#include <sst_config.h>

#include "os/syscall/clone.h"
#include "os/vnodeos.h"
#include "os/vgetthreadstate.h"

using namespace SST::Vanadis;

#define RISCV_SIGCHLD        17
#define RISCV_CSIGNAL       0x000000ff /* Signal mask to be sent at exit.  */
#define RISCV_CLONE_VM      0x00000100 /* Set if VM shared between processes.  */
#define RISCV_CLONE_FS      0x00000200 /* Set if fs info shared between processes.  */
#define RISCV_CLONE_FILES   0x00000400 /* Set if open files shared between processes.  */
#define RISCV_CLONE_SIGHAND 0x00000800 /* Set if signal handlers shared.  */

#define RISCV_CLONE_THREAD  0x00010000 /* Set to add to same thread group.  */
#define RISCV_CLONE_NEWNS   0x00020000 /* Set to create new namespace.  */
#define RISCV_CLONE_SYSVSEM 0x00040000 /* Set to shared SVID SEM_UNDO semantics.  */
#define RISCV_CLONE_SETTLS  0x00080000 /* Set TLS info.  */

#define RISCV_CLONE_PARENT_SETTID  0x00100000 /* Store TID in userlevel buffer before MM copy.  */
#define RISCV_CLONE_CHILD_CLEARTID 0x00200000 /* Register exit futex and memory location to clear.  */
#define RISVC_CLONE_DETACHED       0x00400000 /* Create clone detached.  */ 
#define RISCV_CLONE_CHILD_SETTID   0x01000000 /* Store TID in userlevel buffer in the child.  */

#define CLONE_FLAGS (   RISCV_CLONE_VM | \
                        RISCV_CLONE_FS | \
                        RISCV_CLONE_FILES | \
                        RISCV_CLONE_SIGHAND | \
                        RISCV_CLONE_THREAD | \
                        RISCV_CLONE_SYSVSEM | \
                        RISCV_CLONE_SETTLS | \
                        RISCV_CLONE_PARENT_SETTID | \
                        RISCV_CLONE_CHILD_CLEARTID \
                    )

VanadisCloneSyscall::VanadisCloneSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallCloneEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "clone" )
{
    m_output->debug(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL,
        "[syscall-clone] threadStackAddr=%#" PRIx64 " flags=%#" PRIx64 " parentTidAddr=%#" PRIx64 " tlsAddr=%#" PRIx64 " ctid=%#" PRIx64 " callStackAddr=%#" PRIx64 "\n", 
        event->getThreadStackAddr(), event->getFlags(), event->getParentTidAddr(), event->getTlsAddr(), event->getchildTidAddr(), event->getCallStackAddr() );

    // get a new hwThread to run the new process on
    m_threadID = m_os->allocHwThread();

    if ( nullptr == m_threadID ) {
        m_output->fatal(CALL_INFO, -1, "Error: clone, out of hardware threads for new thread\n");
    }

    if ( 0 == event->getCallStackAddr() ) { 
        m_childTidAddr = event->getchildTidAddr();
    }

    // SIGCHLD - fork()
    if ( (event->getFlags() & RISCV_CSIGNAL) == RISCV_SIGCHLD ) {

        // do this before we create the child becuase the child gets a copy of this the parents page table
        // and we want the child to fault on write as well
        m_os->getMMU()->removeWrite( process->getpid() );

        // create a new process/thread object with a new pid/tid
        m_newThread = new OS::ProcessInfo( *process, m_os->getNewTid() );

        //
        // flush the TLB for this hwThread, there shouldn't be any race conditions give the parent is blocked in the syscall 
        // Is this true? we are returning to the parent CPU before we know that the TLB is flushed
        //
        m_os->getMMU()->flushTlb( event->getCoreID(), event->getThreadID() );

        // create a page table for the child that is a copy of the parents
        m_os->getMMU()->dup( process->getpid(), m_newThread->getpid() );

    } else {
        // DETACHED is deprecated
        if ( ( event->getFlags() & ~RISVC_CLONE_DETACHED ) != CLONE_FLAGS ) {
            m_output->fatal(CALL_INFO, -1, "Error: clone, flags not supported %#lx\n",event->getFlags());
        }    
        m_newThread = new OS::ProcessInfo;

        // this make an exact copy of the process which implies pointers to classes are the same for both processes
        // this includes things like virtMemMap and fdTable
        *m_newThread = *process;

        // the new thread has the same pid as the parent but it has a unique tid
        m_newThread->settid( m_os->getNewTid() );
        m_newThread->incRefCnts();
        m_newThread->addThread( m_newThread );
    }

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
            m_buffer.resize(sizeof(uint32_t)); } else {
            m_buffer.resize(sizeof(uint64_t));
        }

        m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL,"read call stack %#" PRIx64 " \n",event->getCallStackAddr()+16);

        readMemory( event->getCallStackAddr()+16, m_buffer );

        m_state = ReadCallStack;
    } else {
        readThreadStack(event);
    }
}

void VanadisCloneSyscall::readThreadStack(VanadisSyscallCloneEvent* event)
{
    if ( event->getThreadStackAddr() ) {
        // the entry point for the thread is a fucntion, "int (*fn)(void * arg)", the address of which is passed to to the syscall located
        // in the thread stack, the arg is also located in the thread stack, we need to read it 
        
        // this has to be the size of function pointer and stack pointer
        if ( event->getOSBitType() == VanadisOSBitType::VANADIS_OS_32B ) {
            m_buffer.resize( 2 * sizeof(uint32_t));
        } else {
            m_buffer.resize( 2 * sizeof(uint64_t));
        }

        m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL,"read thread stack %#" PRIx64 " \n",event->getThreadStackAddr());

        readMemory( event->getThreadStackAddr(), m_buffer );

        m_state = ReadThreadStack;
    } else { 
        setTid(event);
    }
}

void VanadisCloneSyscall::setTid(VanadisSyscallCloneEvent* event)
{
    m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL,"setTidAddress %#" PRIx64 " \n",m_childTidAddr);
    m_newThread->setTidAddress( m_childTidAddr );

    // because we are lazy don't allow both of these at once, If this can happen we will need to add another state
    if ( event->getFlags() & RISCV_CLONE_PARENT_SETTID && event->getFlags() & RISCV_CLONE_CHILD_SETTID ) {
        assert(0);
    }

    // this code could be collapsed
    if ( event->getFlags() & RISCV_CLONE_PARENT_SETTID ) {
        assert( event->getParentTidAddr() );

        // this has to be size of pid_t
        if ( getEvent<VanadisSyscallCloneEvent*>()->getOSBitType() == VanadisOSBitType::VANADIS_OS_32B ) {
            m_buffer.resize(sizeof(uint32_t));
            *((uint32_t*)m_buffer.data()) = m_newThread->gettid(); 
        } else {
            m_buffer.resize(sizeof(uint32_t));
            *((uint32_t*)m_buffer.data()) = m_newThread->gettid(); 
        }

        m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL,"PARENT_SETTID addr %#" PRIx64 " tid=%d\n",event->getParentTidAddr(),  m_newThread->gettid());

        writeMemory( event->getParentTidAddr(), m_buffer );
        m_state = ChildSetTid;

    } else if ( event->getFlags() & RISCV_CLONE_CHILD_SETTID ) {
        assert( m_childTidAddr );

        // this has to be size of pid_t
        if ( getEvent<VanadisSyscallCloneEvent*>()->getOSBitType() == VanadisOSBitType::VANADIS_OS_32B ) {
            m_buffer.resize(sizeof(uint32_t));
            *((uint32_t*)m_buffer.data()) = m_newThread->gettid(); 
        } else {
            m_buffer.resize(sizeof(uint32_t));
            *((uint32_t*)m_buffer.data()) = m_newThread->gettid(); 
        }

        m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL,"CHILD_SETTID addr %#" PRIx64 " tid=%d\n",m_childTidAddr,  m_newThread->gettid());

        writeMemory( m_childTidAddr, m_buffer, m_newThread );

        m_state = ChildSetTid;
    } else {
        finish(event);
    }
}

void VanadisCloneSyscall::finish(VanadisSyscallCloneEvent* event)
{
    m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL, "\n");
    sendNeedThreadState();
}

// we have the instPtr and registers from the parent core/hwThread,
void VanadisCloneSyscall::handleEvent( VanadisCoreEvent* ev )
{
    auto event = getEvent<VanadisSyscallCloneEvent*>();
    auto resp = dynamic_cast<VanadisGetThreadStateResp*>( ev );
    assert(resp);

    m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL, "[syscall-clone] got thread state response\n");

    _VanadisStartThreadBaseReq* req;

    if ( (event->getFlags() & RISCV_CSIGNAL) == RISCV_SIGCHLD ) {
        req = new VanadisStartThreadForkReq( m_threadID->hwThread, resp->getInstPtr(), resp->getTlsPtr() );
    } else {
        req = new VanadisStartThreadCloneReq( m_threadID->hwThread, m_threadStartAddr, event->getThreadStackAddr(),
                                    event->getTlsAddr(), m_threadArgAddr );
    }

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

void VanadisCloneSyscall::memReqIsDone(bool) 
{
    auto event = getEvent<VanadisSyscallCloneEvent*>();

    switch( m_state ) {
      case ReadCallStack:

        m_childTidAddr = *(uint32_t*)m_buffer.data();
        m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL,"read childTidAddr=%#" PRIx64 " \n",m_childTidAddr);

        readThreadStack(event);
        break;

      case ReadThreadStack:
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

        setTid( event );
        break;

      case ChildSetTid:
        finish(event);
        break;
    }
}
