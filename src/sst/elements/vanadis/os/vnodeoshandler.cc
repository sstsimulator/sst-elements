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
#include "os/syscall/fstat.h"
#include "os/syscall/statx.h"

using namespace SST::Vanadis;

static const char* SyscallName[] = {
    FOREACH_FUNCTION(GENERATE_STRING)
};

VanadisSyscall* VanadisNodeOSComponent::handleIncomingSyscall( OS::ProcessInfo* process, VanadisSyscallEvent* sys_ev, SST::Link* coreLink ) 
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
            syscall = new VanadisSetTidAddressSyscall( this, coreLink, process, convertEvent<VanadisSyscallSetTidAddressEvent*>( "set_tid_address", sys_ev ) );
        } break;
        case SYSCALL_OP_MPROTECT: {
            syscall = new VanadisMprotectSyscall( this, coreLink, process, convertEvent<VanadisSyscallMprotectEvent*>( "mprotect", sys_ev ) );
        } break;
        case SYSCALL_OP_FORK: {
            syscall = new VanadisForkSyscall( this, coreLink, process, convertEvent<VanadisSyscallForkEvent*>( "fork", sys_ev ) );
        } break;
        case SYSCALL_OP_CLONE: {
            syscall = new VanadisCloneSyscall( this, coreLink, process, convertEvent<VanadisSyscallCloneEvent*>( "clone", sys_ev ) );
        } break;
        case SYSCALL_OP_FUTEX: {
            syscall = new VanadisFutexSyscall( this, coreLink, process, convertEvent<VanadisSyscallFutexEvent*>( "futex", sys_ev ) );
        } break;
        case SYSCALL_OP_EXIT: {
            syscall = new VanadisExitSyscall( this, coreLink, process, convertEvent<VanadisSyscallExitEvent*>( "exit", sys_ev ) );
        } break;
        case SYSCALL_OP_EXIT_GROUP: {
            syscall = new VanadisExitGroupSyscall( this, coreLink, process, convertEvent<VanadisSyscallExitGroupEvent*>( "exitgroup", sys_ev ) );
        } break;
        case SYSCALL_OP_GETPID: {
            syscall = new VanadisGetpidSyscall( this, coreLink, process, convertEvent<VanadisSyscallGetxEvent*>( "getpid", sys_ev ) );
        } break;
        case SYSCALL_OP_GETTID: {
            syscall = new VanadisGettidSyscall( this, coreLink, process, convertEvent<VanadisSyscallGetxEvent*>( "gettid", sys_ev ) );
        } break;
        case SYSCALL_OP_GETPPID: {
            syscall = new VanadisGetppidSyscall( this, coreLink, process, convertEvent<VanadisSyscallGetxEvent*>( "getppid", sys_ev ) );
        } break;
        case SYSCALL_OP_GETPGID: {
            syscall = new VanadisGetpgidSyscall( this, coreLink, process, convertEvent<VanadisSyscallGetxEvent*>( "getpgid", sys_ev ) );
        } break;
        case SYSCALL_OP_UNAME: {
            syscall = new VanadisUnameSyscall( this, coreLink, process, convertEvent<VanadisSyscallUnameEvent*>( "uname", sys_ev ) );
        } break;
        case SYSCALL_OP_UNLINK: {
            syscall = new VanadisUnlinkSyscall( this, coreLink, process, convertEvent<VanadisSyscallUnlinkEvent*>( "unlink", sys_ev ) );
        } break;
        case SYSCALL_OP_UNLINKAT: {
            syscall = new VanadisUnlinkatSyscall( this, coreLink, process, convertEvent<VanadisSyscallUnlinkatEvent*>( "unlinkat", sys_ev ) );
        } break;
        case SYSCALL_OP_OPEN: {
            syscall = new VanadisOpenSyscall( this, coreLink, process, convertEvent<VanadisSyscallOpenEvent*>( "open", sys_ev ) );
        } break;
        case SYSCALL_OP_OPENAT: {
            syscall = new VanadisOpenatSyscall( this, coreLink, process, convertEvent<VanadisSyscallOpenatEvent*>( "openat", sys_ev ) );
        } break;
        case SYSCALL_OP_CLOSE: {
            syscall = new VanadisCloseSyscall( this, coreLink, process, convertEvent<VanadisSyscallCloseEvent*>( "close", sys_ev ) );
        } break;
        case SYSCALL_OP_BRK: {
            syscall = new VanadisBrkSyscall( this, coreLink, process, convertEvent<VanadisSyscallBRKEvent*>( "brk", sys_ev ) );
        } break;
        case SYSCALL_OP_MMAP: {
            syscall = new VanadisMmapSyscall( this, coreLink, process, convertEvent<VanadisSyscallMemoryMapEvent*>( "mmap", sys_ev ) );
        } break;
        case SYSCALL_OP_UNMAP: {
            syscall = new VanadisUnmapSyscall( this, coreLink, process, m_physMemMgr, convertEvent<VanadisSyscallMemoryUnMapEvent*>( "unmap", sys_ev ) );
        } break;
        case SYSCALL_OP_GETTIME64: {
            syscall = new VanadisGettime64Syscall( this, coreLink, process, getSimNanoSeconds(), convertEvent<VanadisSyscallGetTime64Event*>( "gettime64", sys_ev ) );
        } break;
        case SYSCALL_OP_ACCESS: {
            syscall = new VanadisAccessSyscall( this, coreLink, process, convertEvent<VanadisSyscallAccessEvent*>( "access", sys_ev ) );
        } break;
        case SYSCALL_OP_READLINK: {
            syscall = new VanadisReadlinkSyscall( this, coreLink, process, convertEvent<VanadisSyscallReadLinkEvent*>( "readlink", sys_ev ) );
        } break;
        case SYSCALL_OP_READ: {
            syscall = new VanadisReadSyscall( this, coreLink, process, convertEvent<VanadisSyscallReadEvent*>( "read", sys_ev ) );
        } break;
        case SYSCALL_OP_READV: {
            syscall = new VanadisReadvSyscall( this, coreLink, process, convertEvent<VanadisSyscallReadvEvent*>( "readv", sys_ev ) );
        } break;
        case SYSCALL_OP_WRITE: {
            syscall = new VanadisWriteSyscall( this, coreLink, process, convertEvent<VanadisSyscallWriteEvent*>( "write", sys_ev ) );
        } break;
        case SYSCALL_OP_WRITEV: {
            syscall = new VanadisWritevSyscall( this, coreLink, process, convertEvent<VanadisSyscallWritevEvent*>( "writev", sys_ev ) );
        } break;
        case SYSCALL_OP_IOCTL: {
            syscall = new VanadisIoctlSyscall( this, coreLink, process, convertEvent<VanadisSyscallIoctlEvent*>( "ioctl", sys_ev ) );
        } break;
        case SYSCALL_OP_FSTAT: {
            syscall = new VanadisFstatSyscall( this, coreLink, process, convertEvent<VanadisSyscallFstatEvent*>( "fstat", sys_ev ) );
        } break;
        case SYSCALL_OP_STATX: {
            syscall = new VanadisStatxSyscall( this, coreLink, process, convertEvent<VanadisSyscallStatxEvent*>( "statx", sys_ev ) );
        } break;

        default: {
            output->fatal(CALL_INFO, -1,"Error - unknown operating system call, op %s, instPtr=%#" PRIx64 "\n",
                            SyscallName[sys_ev->getOperation()], sys_ev->getInstPtr() );
        } break;
    }

    return syscall;
}
