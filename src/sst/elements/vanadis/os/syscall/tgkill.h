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

#ifndef _H_VANADIS_OS_SYSCALL_TGKILL
#define _H_VANADIS_OS_SYSCALL_TGKILL

#include "os/syscall/syscall.h"
#include "os/callev/voscalltgkill.h"
#include "os/resp/vosexitresp.h"

namespace SST {
namespace Vanadis {

// Handled signals - any not listed has not been implemented yet
#define VANADIS_SIGNAL_SIGABRT 6

class VanadisTgKillSyscall : public VanadisSyscall {
public:
    VanadisTgKillSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallTgKillEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "tgkill" )
    {
        #ifdef VANADIS_BUILD_DEBUG
        m_output->verbose(CALL_INFO, 0, 0, "[syscall-tgkill] ---> pid=%d, tgid=%d tid=%d sig=%d\n", process->gettid(), event->getTgid(), event->getTid(), event->getSig() );
        #endif

        if ( event->getSig() == VANADIS_SIGNAL_SIGABRT ) {
            // Lookup tid in thread group and force it to exit

            auto threads = process->getThreadList(); // map tid -> thread info

            if ( process->gettid() == event->getTid() ) { // Terminating main thread
                #ifdef VANADIS_BUILD_DEBUG
                m_output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL, "[syscall-exit] core %d thread %d process %d\n",event->getCoreID(), event->getThreadID(), process->getpid() );
                #endif

                m_output->output("WARNING: Vanadis core=%d, thread=%d, pid=%d tid=%d has received a SIGABRT\n", event->getCoreID(), event->getThreadID(), process->getpid(), process->gettid());

                uint64_t tid_addr = process->getTidAddress();
                if ( tid_addr ) {
                    // this has to be the size of pid_t
                    buffer_.resize(sizeof(uint32_t));
                    *((uint32_t*)buffer_.data()) = 0;
                    writeMemory( tid_addr, buffer_ );
                } else {
                    memReqIsDone(true);
                }

            } else {
                Vanadis::OS::ProcessInfo* thr_process = threads.find(event->getTid())->second;
                unsigned core = thr_process->getCore();
                unsigned hw_thread = thr_process->getHwThread();

                // tell hwthread to stop
                coreLink->send( new VanadisExitResponse( -1, hw_thread) );

                // remove thread from OS
                os->removeThread( core, hw_thread, thr_process->gettid() );

                // delete thread
                delete thr_process;
            }

        } else {
            m_output->fatal(CALL_INFO, -1, "tgkill not implemented for signal [%d]", event->getSig());

        }
    }

private:
    void memReqIsDone(bool) {

        auto event = getEvent<VanadisSyscallTgKillEvent*>();

        auto syscall = m_process->findFutex( m_process->getTidAddress());
        if ( syscall ) {
            #ifdef VANADIS_BUILD_DEBUG
            m_output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL,
            "[syscall-exit] FUTEX_WAKE tid=%d addr=%#" PRIx64 " found waiter, wakeup tid=%d\n",
                m_process->gettid(), m_process->getTidAddress(), syscall->getTid());
            #endif
            dynamic_cast<VanadisFutexSyscall*>( syscall )->wakeup();
            delete syscall;
        }

        m_os->removeThread( event->getCoreID(),event->getThreadID(), m_process->gettid() );
        #ifdef VANADIS_BUILD_DEBUG
        m_output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL, "[syscall-tgkill] %s() called on thr=%d \n",__func__,m_process->gettid() );
        #endif

        setReturnExited();
    }

    std::vector<uint8_t> buffer_;
};

} // namespace Vanadis
} // namespace SST

#endif