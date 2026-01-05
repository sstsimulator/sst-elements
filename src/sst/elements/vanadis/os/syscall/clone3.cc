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

#include "os/syscall/clone3.h"
#include "os/syscall/syscall_defines.h"
#include "os/vnodeos.h"
#include "os/vgetthreadstate.h"

using namespace SST::Vanadis;

#define CLONE3_INVALID_FLAGS     ( VANADIS_LINUX_CSIGNAL | VANADIS_LINUX_CLONE_DETACHED )

// Corresponds to flags = 0x000f3d00
#define CLONE3_VALID_FLAGS ( VANADIS_LINUX_CLONE_VM | \
                        VANADIS_LINUX_CLONE_FS | \
                        VANADIS_LINUX_CLONE_FILES | \
                        VANADIS_LINUX_CLONE_SIGHAND | \
                        VANADIS_LINUX_CLONE_THREAD | \
                        VANADIS_LINUX_CLONE_SYSVSEM | \
                        VANADIS_LINUX_CLONE_SETTLS | \
                        VANADIS_LINUX_CLONE_PARENT_SETTID | \
                        VANADIS_LINUX_CLONE_CHILD_CLEARTID \
                    )

VanadisClone3Syscall::VanadisClone3Syscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallClone3Event* event )
        : VanadisSyscall( os, coreLink, process, event, "clone3" )
{
    #ifdef VANADIS_BUILD_DEBUG
    m_output->debug(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL,
        "[syscall-clone3] cloneArgsAddr=%#" PRIx64 " size=%" PRIu64 "\n",
        event->getCloneArgsAddr(), event->getCloneArgsSize());
    #endif
    if ( event->getOSBitType() == VanadisOSBitType::VANADIS_OS_32B ) {
        m_output->fatal(CALL_INFO, -1, "Error: clone3 is not support for 32b systems\n");
    }

    // get a new hwThread to run the new process on
    hw_thread_id_ = m_os->allocHwThread();

    if ( nullptr == hw_thread_id_ ) {
        m_output->fatal(CALL_INFO, -1, "Error: clone3, out of hardware threads for new thread\n");
    }

    // Read the cl_args structure
    buffer_.resize( event->getCloneArgsSize(), 0 );
    readMemory( event->getCloneArgsAddr(), buffer_ );
    state_ = State::ReadCloneArgs;
}

// we have the instPtr and registers from the parent core/hwThread,
void VanadisClone3Syscall::handleEvent( VanadisCoreEvent* ev )
{
    auto event = getEvent<VanadisSyscallClone3Event*>();
    auto resp = dynamic_cast<VanadisGetThreadStateResp*>( ev );
    assert(resp);

    #ifdef VANADIS_BUILD_DEBUG
    m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL, "[syscall-clone3] got thread state response\n");
    #endif
    _VanadisStartThreadBaseReq* req;

    if ( exit_signal_ == VANADIS_LINUX_SIGCHLD ) {
        req = new VanadisStartThreadForkReq( hw_thread_id_->hwThread, resp->getInstPtr(), resp->getTlsPtr() );
    } else {
        req = new VanadisStartThreadClone3Req( hw_thread_id_->hwThread, resp->getInstPtr(), stack_, tls_);
    }

    req->setIntRegs( resp->intRegs );
    req->setFpRegs( resp->fpRegs );

    #ifdef VANADIS_BUILD_DEBUG
    m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL, "[syscall-clone3] core=%d thread=%d tid=%d instPtr=%" PRI_ADDR "\n",
                hw_thread_id_->core, hw_thread_id_->hwThread, new_thread_->gettid(), resp->getInstPtr() );
    #endif

    m_os->sendEvent( hw_thread_id_->core, req );

    setReturnSuccess( new_thread_->gettid() );

    delete resp;
}

// Available 64B only
void VanadisClone3Syscall::memReqIsDone(bool)
{
    auto event = getEvent<VanadisSyscallClone3Event*>();

    switch ( state_ ) {
        case State::ReadCloneArgs:
            flags_ = *(uint64_t*)buffer_.data();
            pidfd_ = *((uint64_t*)buffer_.data() + 1);
            child_tid_ = *((uint64_t*)buffer_.data() + 2);
            parent_tid_ = *((uint64_t*)buffer_.data() + 3);
            exit_signal_ = *((uint64_t*)buffer_.data() + 4);
            stack_ = *((uint64_t*)buffer_.data() + 5);
            stack_size_ = *((uint64_t*)buffer_.data() + 6);
            tls_ = *((uint64_t*)buffer_.data() + 7);
            if ( event->getCloneArgsSize() > 64 ) {
                set_tid_ = *((uint64_t*)buffer_.data() + 8);
                set_tid_size_ = *((uint64_t*)buffer_.data() + 9);
            }
            if ( event->getCloneArgsSize() > 80 ) {
                cgroup_ = *((uint64_t*)buffer_.data() + 10);
            }
            stack_ = stack_ + stack_size_;  // stack_ is passed as lowest byte mem address,
                                            // stack grows down so swap to highest
            parseCloneArgs(event);
            break;

        case State::ChildSetTid:
            finish(event);
            break;
    }
}

// State 1: ParseCloneArgs - after reading cl_args from memory
void VanadisClone3Syscall::parseCloneArgs(VanadisSyscallClone3Event* event)
{
    // Only exit_signal handled right now...
    if ( exit_signal_ == VANADIS_LINUX_SIGCHLD ) {
        // do this before we create the child becuase the child gets a copy of the parents page table
        // and we want the child to fault on write as well
        m_os->getMMU()->removeWrite( m_process->getpid() );

        // create a new process/thread object with a new pid/tid
        new_thread_ = new OS::ProcessInfo( *m_process, m_os->getNewTid() );

        // flush the TLB for this hwThread, there shouldn't be any race conditions given the parent is blocked in the syscall
        // Is this true? we are returning to the parent CPU before we know that the TLB is flushed
        m_os->getMMU()->flushTlb( event->getCoreID(), event->getThreadID() );

        // create a page table for the child that is a copy of the parents
        m_os->getMMU()->dup( m_process->getpid(), new_thread_->getpid() );

    } else {
        // We only handle the common combination of flags for now (ignoring irrelevant ones)
        if ( ( flags_ & ~CLONE3_INVALID_FLAGS ) != CLONE3_VALID_FLAGS ) {
            m_output->fatal(CALL_INFO, -1,
                "Error: clone3, syscall implementation missing support for flags: %#" PRIx64 "\n", flags_);
        }
        // Create a new thread context
        new_thread_ = new OS::ProcessInfo;

        // Make an exact copy of the process which implies pointers to classes are the same for both processes
        // this includes things like virtMemMap and fdTable
        *new_thread_ = *m_process;

        // the new thread has the same pid as the parent but it has a unique tid
        new_thread_->settid( m_os->getNewTid() );
        new_thread_->incRefCnts();
        new_thread_->addThread( new_thread_ );
    }

    new_thread_->setHwThread( *hw_thread_id_ );
    m_os->setThread( new_thread_->gettid(), new_thread_ );

    #ifdef VANADIS_BUILD_DEBUG
    m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL, "[syscall-clone3] newthread pid=%d tid=%d ppid=%d numThreads=%d\n",
            new_thread_->getpid(), new_thread_->gettid(), new_thread_->getppid(), new_thread_->numThreads() );
    #endif

    m_os->setProcess( hw_thread_id_->core, hw_thread_id_->hwThread, new_thread_ );
    m_os->getMMU()->setCoreToPageTable( hw_thread_id_->core, hw_thread_id_->hwThread, new_thread_->getpid() );

    setTidAtParent(event);
}

// State 2: Handle the VANADIS_LINUX_CLONE_PARENT_SETTID flag
void VanadisClone3Syscall::setTidAtParent(VanadisSyscallClone3Event* event)
{
    #ifdef VANADIS_BUILD_DEBUG
    m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL,"setTidAddress %#" PRIx64 " \n", child_tid_);
    #endif
    new_thread_->setTidAddress( child_tid_ );

    // this code could be collapsed
    assert( parent_tid_ );

    // this has to be size of pid_t
    buffer_.resize(sizeof(uint32_t));
    *((uint32_t*)buffer_.data()) = new_thread_->gettid();

    #ifdef VANADIS_BUILD_DEBUG
    m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL,"PARENT_SETTID addr %#" PRIx64 " tid=%d\n",
        parent_tid_,  new_thread_->gettid());
    #endif

    writeMemory( parent_tid_, buffer_ );
    state_ = State::ChildSetTid;
}

// State 3: Finish up and return
void VanadisClone3Syscall::finish(VanadisSyscallClone3Event* event)
{
    #ifdef VANADIS_BUILD_DEBUG
    m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL, "\n");
    #endif
    sendNeedThreadState();
}