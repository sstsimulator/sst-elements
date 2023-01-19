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

#include "os/syscall/futex.h"
#include "os/vnodeos.h"

#define FUTEX_WAIT      0
#define FUTEX_WAKE      1
#define FUTEX_FD        2
#define FUTEX_REQUEUE       3
#define FUTEX_CMP_REQUEUE   4
#define FUTEX_WAKE_OP       5
#define FUTEX_LOCK_PI       6
#define FUTEX_UNLOCK_PI     7
#define FUTEX_TRYLOCK_PI    8
#define FUTEX_WAIT_BITSET   9
#define FUTEX_WAKE_BITSET   10
#define FUTEX_WAIT_REQUEUE_PI   11
#define FUTEX_CMP_REQUEUE_PI    12

#define FUTEX_PRIVATE_FLAG  128
#define FUTEX_CLOCK_REALTIME    256
#define FUTEX_CMD_MASK      ~(FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME)

#define FUTEX_WAIT_PRIVATE  (FUTEX_WAIT | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_PRIVATE  (FUTEX_WAKE | FUTEX_PRIVATE_FLAG)
#define FUTEX_REQUEUE_PRIVATE   (FUTEX_REQUEUE | FUTEX_PRIVATE_FLAG)
#define FUTEX_CMP_REQUEUE_PRIVATE (FUTEX_CMP_REQUEUE | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_OP_PRIVATE   (FUTEX_WAKE_OP | FUTEX_PRIVATE_FLAG)
#define FUTEX_LOCK_PI_PRIVATE   (FUTEX_LOCK_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_UNLOCK_PI_PRIVATE (FUTEX_UNLOCK_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_TRYLOCK_PI_PRIVATE (FUTEX_TRYLOCK_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAIT_BITSET_PRIVATE   (FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_BITSET_PRIVATE   (FUTEX_WAKE_BITSET | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAIT_REQUEUE_PI_PRIVATE   (FUTEX_WAIT_REQUEUE_PI | \
                     FUTEX_PRIVATE_FLAG)
#define FUTEX_CMP_REQUEUE_PI_PRIVATE    (FUTEX_CMP_REQUEUE_PI | \
                     FUTEX_PRIVATE_FLAG)

using namespace SST::Vanadis;

VanadisFutexSyscall::VanadisFutexSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallFutexEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "futex" ), m_state(ReadAddr), m_numWokeup(0)
{
    m_output->verbose(CALL_INFO, 16, 0, "[syscall-futex] addr=%#" PRIx64 " op=%#x val=%#" PRIx32 " timeAddr=%#" PRIx64 " callStackAddr=%#" PRIx64 
            " val2=%d, addr2=%#" PRIx64 " val2=%d\n", 
            event->getAddr(), event->getOp(), event->getVal(), event->getTimeAddr(), event->getStackPtr(), event->getVal2(), event->getAddr2(), event->getVal3() );

    m_op = event->getOp();
    if( m_op & ~0xff ) assert(0);

    if ( m_op & FUTEX_PRIVATE_FLAG ) {
        m_op -= FUTEX_PRIVATE_FLAG;
    }

    switch ( m_op ) {
      case FUTEX_WAKE:
        {
            m_output->verbose(CALL_INFO, 16, 0, "[syscall-futex]  FUTEX_WAKE tid=%d addr=%#" PRIx64 "\n", process->gettid(), event->getAddr());
            VanadisSyscall* syscall = m_process->findFutex( event->getAddr() );
            if ( syscall ) {
                m_output->verbose(CALL_INFO, 16, 0, "[syscall-futex] FUTEX_WAKE tid=%d addr=%#" PRIx64 " found waiter\n",process->gettid(), event->getAddr());
                dynamic_cast<VanadisFutexSyscall*>( syscall )->wakeup();
                delete syscall;
                setReturnSuccess(1);
            } else {
                m_output->verbose(CALL_INFO, 16, 0, "[syscall-futex] FUTEX_WAKE tid=%d addr=%#" PRIx64 " no waiter\n", process->gettid(), event->getAddr());
                setReturnSuccess(0);
            }
        } break;

      case FUTEX_WAIT:
        {
            m_output->verbose(CALL_INFO, 16, 0, "[syscall-futex] FUTEX_WAIT tid=%d addr=%#" PRIx64 "\n", process->gettid(), event->getAddr());
            m_buffer.resize(sizeof(uint32_t));
            readMemory( event->getAddr(), m_buffer );
        } break;
      case FUTEX_REQUEUE:
        {
            m_output->verbose(CALL_INFO, 16, 0, "[syscall-futex] FUTEX_REQUEUE tid=%d addr=%#" PRIx64 "\n", process->gettid(), event->getAddr());
            m_buffer.resize(sizeof(uint32_t));
            readMemory( event->getAddr(), m_buffer );
        } break;

      case FUTEX_CMP_REQUEUE:
        assert(0);
      case FUTEX_FD:
        assert(0);
      case FUTEX_WAKE_OP:
        assert(0);
      case FUTEX_LOCK_PI:
        assert(0);
      case FUTEX_UNLOCK_PI:
        assert(0);
      case FUTEX_TRYLOCK_PI:
        assert(0);
      case FUTEX_WAIT_BITSET:
        assert(0);
      case FUTEX_WAKE_BITSET:
        assert(0);
      case FUTEX_WAIT_REQUEUE_PI:
        assert(0);
      case FUTEX_CMP_REQUEUE_PI:
      default:
        assert(0);
    }
}

void VanadisFutexSyscall::wakeup() 
{
    m_output->verbose(CALL_INFO, 16, 0, "[syscall-futex] FUTEX_WAIT wakeup \n");
    setReturnSuccess(0);
}

void VanadisFutexSyscall::memReqIsDone() 
{
    switch( m_op ) {
      case FUTEX_WAIT: 
        {
            uint32_t val = *(uint32_t*)m_buffer.data();

            if ( val == getEvent<VanadisSyscallFutexEvent*>()->getVal() ) {
                m_output->verbose(CALL_INFO, 16, 0, "[syscall-futex] FUTEX_WAIT tid=%d addr=%#" PRIx64 " vals match go to sleep\n",
                        m_process->gettid(), getEvent<VanadisSyscallFutexEvent*>()->getAddr());

                m_process->addFutexWait( getEvent<VanadisSyscallFutexEvent*>()->getAddr(), this );

            } else { 
                m_output->verbose(CALL_INFO, 16, 0, "[syscall-futex] FUTEX_WAIT tid=%d addr=%#" PRIx64 " vals dont match return\n",
                         m_process->gettid(), getEvent<VanadisSyscallFutexEvent*>()->getAddr());
                setReturnFail(-EINVAL);
            }
        } break;
      case FUTEX_REQUEUE:
        {
            if ( ReadAddr == m_state ) { 
                uint32_t val = *(uint32_t*)m_buffer.data();
                m_output->verbose(CALL_INFO, 16, 0, "[syscall-futex] FUTEX_REQUEUE wakeup %d waiter\n",val);

                int numWaiters = m_process->futexGetNumWaiters( getEvent<VanadisSyscallFutexEvent*>()->getAddr() );
                m_output->verbose(CALL_INFO, 16, 0, "[syscall-futex] FUTEX_REQUEUE numWaiters %d\n",numWaiters);

                // wakeup at most val number of waiters
                for ( int i = 0; i < val && i < numWaiters; i++ ) { 
                    VanadisSyscall* syscall = m_process->findFutex( getEvent<VanadisSyscallFutexEvent*>()->getAddr() );
                    if ( syscall ) {
                        ++m_numWokeup;
                        m_output->verbose(CALL_INFO, 16, 0, "[syscall-futex] FUTEX_REQUEUE tid=%d addr=%#" PRIx64 " wakeup\n",
                                    m_process->gettid(), getEvent<VanadisSyscallFutexEvent*>()->getAddr());
                        dynamic_cast<VanadisFutexSyscall*>( syscall )->wakeup();
                        delete syscall;
                    }
                }
                m_output->verbose(CALL_INFO, 16, 0, "[syscall-futex] FUTEX_REQUEUE numWokeup %d\n",m_numWokeup);
                
                // if there are no more waiters, we are done
                // else read val2 and addr2 so we can move val2 waiters to addr2
                if ( 0 == numWaiters - m_numWokeup ) {
                    setReturnSuccess( m_numWokeup );
                } else {
                    if ( getEvent<VanadisSyscallFutexEvent*>()->getStackPtr() ) {
                        // read val2 and uaddr2
                        m_buffer.resize(sizeof(uint32_t)*2);
                        readMemory( getEvent<VanadisSyscallFutexEvent*>()->getStackPtr(), m_buffer );
                        m_state = ReadArgs;
                    } else {
                        finish( getEvent<VanadisSyscallFutexEvent*>()->getVal2(), getEvent<VanadisSyscallFutexEvent*>()->getAddr2() );
                    }
                }

            } else if ( ReadArgs == m_state ) {
                finish( ((uint32_t*)m_buffer.data())[0], ((uint32_t*)m_buffer.data())[1] );

            } else {
                assert(0);
            }
        } break;
    }
}

void VanadisFutexSyscall::finish( uint32_t val2, uint64_t addr2 ) 
{
    m_output->verbose(CALL_INFO, 16, 0, "[syscall-futex] FUTEX_REQUEUE read val2=%d addr2=%#" PRIx64 "\n",val2,addr2);

    int numWaiters = m_process->futexGetNumWaiters( getEvent<VanadisSyscallFutexEvent*>()->getAddr() );
    m_output->verbose(CALL_INFO, 16, 0, "[syscall-futex] FUTEX_REQUEUE numWaiters %d\n",numWaiters);

    for ( int i = 0; i < val2 && i < numWaiters; i++ ) { 
        VanadisSyscall* syscall = m_process->findFutex( getEvent<VanadisSyscallFutexEvent*>()->getAddr() );
        if ( syscall ) {
            m_output->verbose(CALL_INFO, 16, 0, "[syscall-futex] FUTEX_REQUEUE tid=%d addr=%#" PRIx64 " move to %#" PRIx64 "\n",m_process->gettid(),
                getEvent<VanadisSyscallFutexEvent*>()->getAddr(),addr2); m_process->addFutexWait( addr2, syscall );
        }
    }

    setReturnSuccess( m_numWokeup );
}
