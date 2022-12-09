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

#include "os/vnodeos.h"
#include "os/vdumpregsreq.h"

#include "os/syscall/fork.h"
#include "os/syscall/clone.h"
#include "os/syscall/futex.h"
#include "os/syscall/exit.h"
#include "os/syscall/exitgroup.h"
#include "os/syscall/settidaddress.h"
#include "os/syscall/mprotect.h"
#include "os/syscall/getpid.h"
#include "os/syscall/gettid.h"
#include "os/syscall/getppid.h"
#include "os/syscall/getpgid.h"
#include "os/syscall/uname.h"
#include "os/syscall/unlink.h"
#include "os/syscall/unlinkat.h"
#include "os/syscall/open.h"
#include "os/syscall/openat.h"
#include "os/syscall/close.h"
#include "os/syscall/brk.h"
#include "os/syscall/mmap.h"
#include "os/syscall/unmap.h"
#include "os/syscall/gettime.h"
#include "os/syscall/access.h"
#include "os/syscall/readlink.h"
#include "os/syscall/read.h"
#include "os/syscall/readv.h"
#include "os/syscall/write.h"
#include "os/syscall/writev.h"
#include "os/syscall/ioctl.h"

using namespace SST::Vanadis;

static const char* SyscallName[] = {
    FOREACH_FUNCTION(GENERATE_STRING)
};

void VanadisNodeOSComponent::handleIncomingSyscall( OS::ProcessInfo* process, VanadisSyscallEvent* sys_ev, SST::Link* core_link) 
{
    if ( sys_ev->getOperation() < NUM_SYSCALLS  ) {
       output->verbose(CALL_INFO, 16, 0, "from core=%" PRIu32 " thr=%" PRIu32 " syscall=%s\n",
                    sys_ev->getCoreID(), sys_ev->getThreadID(),SyscallName[sys_ev->getOperation()]);
    }

    VanadisSyscall* syscall=NULL;

    // **********************************
    // NOTE that we ended up having to pass in "this" to some syscalls.
    // If we modify all syscalls to accpt "this" argument we would not need to pass in member functions of "this" for all system calls
    // for now we will leave this inconsistency
    // ***********************************
    switch (sys_ev->getOperation()) {
        case SYSCALL_OP_SET_TID_ADDRESS: {
            syscall = new VanadisSetTidAddressSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallSetTidAddressEvent*>( "set_tid_address", sys_ev ), this );
        } break;
        case SYSCALL_OP_MPROTECT: {
            syscall = new VanadisMprotectSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallMprotectEvent*>( "mprotect", sys_ev ), this );
        } break;
        case SYSCALL_OP_FORK: {
            syscall = new VanadisForkSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallForkEvent*>( "fork", sys_ev ), this );
        } break;
        case SYSCALL_OP_CLONE: {
            syscall = new VanadisCloneSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallCloneEvent*>( "clone", sys_ev ), this );
        } break;
        case SYSCALL_OP_FUTEX: {
            syscall = new VanadisFutexSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallFutexEvent*>( "futex", sys_ev ), this );
        } break;
        case SYSCALL_OP_EXIT: {
            syscall = new VanadisExitSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallExitEvent*>( "exit", sys_ev ), this );
        } break;
        case SYSCALL_OP_EXIT_GROUP: {
            syscall = new VanadisExitGroupSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallExitGroupEvent*>( "exitgroup", sys_ev ), this );
        } break;
        case SYSCALL_OP_GETPID: {
            syscall = new VanadisGetpidSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallGetxEvent*>( "getpid", sys_ev ) );
        } break;
        case SYSCALL_OP_GETTID: {
            syscall = new VanadisGettidSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallGetxEvent*>( "gettid", sys_ev ) );
        } break;
        case SYSCALL_OP_GETPPID: {
            syscall = new VanadisGetppidSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallGetxEvent*>( "getppid", sys_ev ) );
        } break;
        case SYSCALL_OP_GETPGID: {
            syscall = new VanadisGetpgidSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallGetxEvent*>( "getpgid", sys_ev ) );
        } break;
        case SYSCALL_OP_UNAME: {
            syscall = new VanadisUnameSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallUnameEvent*>( "uname", sys_ev ) );
        } break;
        case SYSCALL_OP_UNLINK: {
            syscall = new VanadisUnlinkSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallUnlinkEvent*>( "unlink", sys_ev ) );
        } break;
        case SYSCALL_OP_UNLINKAT: {
            syscall = new VanadisUnlinkatSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallUnlinkatEvent*>( "unlinkat", sys_ev ) );
        } break;
        case SYSCALL_OP_OPEN: {
            syscall = new VanadisOpenSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallOpenEvent*>( "open", sys_ev ) );
        } break;
        case SYSCALL_OP_OPENAT: {
            syscall = new VanadisOpenatSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallOpenatEvent*>( "openat", sys_ev ) );
        } break;
        case SYSCALL_OP_CLOSE: {
            syscall = new VanadisCloseSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallCloseEvent*>( "close", sys_ev ) );
        } break;
        case SYSCALL_OP_BRK: {
            syscall = new VanadisBrkSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallBRKEvent*>( "brk", sys_ev ) );
        } break;
        case SYSCALL_OP_MMAP: {
            syscall = new VanadisMmapSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallMemoryMapEvent*>( "mmap", sys_ev ) );
        } break;
        case SYSCALL_OP_UNMAP: {
            syscall = new VanadisUnmapSyscall( output, core_link, process, m_physMemMgr, &m_sendMemReqFunc, convertEvent<VanadisSyscallMemoryUnMapEvent*>( "unmap", sys_ev ) );
        } break;
        case SYSCALL_OP_GETTIME64: {
            syscall = new VanadisGettime64Syscall( output, core_link, process, getSimNanoSeconds(), &m_sendMemReqFunc,
                                    convertEvent<VanadisSyscallGetTime64Event*>( "gettime64", sys_ev ) );
        } break;
        case SYSCALL_OP_ACCESS: {
            syscall = new VanadisAccessSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallAccessEvent*>( "access", sys_ev ) );
        } break;
        case SYSCALL_OP_READLINK: {
            syscall = new VanadisReadlinkSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallReadLinkEvent*>( "readlink", sys_ev ) );
        } break;
        case SYSCALL_OP_READ: {
            syscall = new VanadisReadSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallReadEvent*>( "read", sys_ev ) );
        } break;
        case SYSCALL_OP_READV: {
            syscall = new VanadisReadvSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallReadvEvent*>( "readv", sys_ev ) );
        } break;
        case SYSCALL_OP_WRITE: {
            syscall = new VanadisWriteSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallWriteEvent*>( "write", sys_ev ) );
        } break;
        case SYSCALL_OP_WRITEV: {
            syscall = new VanadisWritevSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallWritevEvent*>( "writev", sys_ev ) );
        } break;
        case SYSCALL_OP_IOCTL: {
            syscall = new VanadisIoctlSyscall( output, core_link, process, &m_sendMemReqFunc, convertEvent<VanadisSyscallIoctlEvent*>( "ioctl", sys_ev ) );
        } break;

        default: {
            output->verbose(CALL_INFO, 0, 0, "Error - unknown operating system call, op %s, instPtr=%#" PRIx64 "\n",
                            SyscallName[sys_ev->getOperation()], sys_ev->getInstPtr() );

            core_link->send( new VanadisDumpRegsReq( sys_ev->getThreadID() ) );

        } break;
    }

    if ( syscall ) {
        if ( syscall->isComplete() ) {
            output->verbose(CALL_INFO, 16, 0,"delete syscall\n");
            delete syscall;
        } else {
            output->verbose(CALL_INFO, 16, 0,"syscall is pending memory accesses\n");
        }
    }
}
