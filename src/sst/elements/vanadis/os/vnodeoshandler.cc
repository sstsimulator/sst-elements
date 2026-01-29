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

#include "os/vnodeos.h"

#include "os/syscall/fork.h"
#include "os/syscall/clone.h"
#include "os/syscall/clone3.h"
#include "os/syscall/futex.h"
#include "os/syscall/exit.h"
#include "os/syscall/exitgroup.h"
#include "os/syscall/kill.h"
#include "os/syscall/tgkill.h"
#include "os/syscall/settidaddress.h"
#include "os/syscall/setrobustlist.h"
#include "os/syscall/mprotect.h"
#include "os/syscall/getcpu.h"
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
#include "os/syscall/lseek.h"
#include "os/syscall/checkpoint.h"
#include "os/syscall/brk.h"
#include "os/syscall/mmap.h"
#include "os/syscall/unmap.h"
#include "os/syscall/gettime.h"
#include "os/syscall/getrandom.h"
#include "os/syscall/access.h"
#include "os/syscall/readlink.h"
#include "os/syscall/readlinkat.h"
#include "os/syscall/read.h"
#include "os/syscall/readv.h"
#include "os/syscall/write.h"
#include "os/syscall/writev.h"
#include "os/syscall/ioctl.h"
#include "os/syscall/fstat.h"
#include "os/syscall/fstatat.h"
#include "os/syscall/statx.h"
#include "os/syscall/getaffinity.h"
#include "os/syscall/setaffinity.h"
#include "os/syscall/prlimit.h"
#include "os/syscall/schedyield.h"

using namespace SST::Vanadis;

static const char* SyscallName[] = {
    FOREACH_FUNCTION(GENERATE_STRING)
};

VanadisSyscall* VanadisNodeOSComponent::handleIncomingSyscall( OS::ProcessInfo* process, VanadisSyscallEvent* sys_ev, SST::Link* core_link )
{
    #ifdef VANADIS_BUILD_DEBUG
    if ( sys_ev->getOperation() < NUM_SYSCALLS  ) {
        output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_SYSCALL, "from core=%" PRIu32 " thr=%" PRIu32 " syscall=%s\n",
                    sys_ev->getCoreID(), sys_ev->getThreadID(),SyscallName[sys_ev->getOperation()]);
    }
    #endif

    VanadisSyscall* syscall=NULL;

    // **********************************
    // NOTE that we ended up having to pass in "this" to some syscalls.
    // If we modify all syscalls to accpt "this" argument we would not need to pass in member functions of "this" for all system calls
    // for now we will leave this inconsistency
    // ***********************************
    switch (sys_ev->getOperation()) {
        case SYSCALL_OP_CHECKPOINT: {
            assert ( CHECKPOINT_SAVE == enable_checkpoint_ );
            syscall = new VanadisCheckpointSyscall( this, core_link, process, convertEvent<VanadisSyscallCheckpointEvent*>( "checkpoint", sys_ev ) );
        } break;
        case SYSCALL_OP_SET_ROBUST_LIST: {
            syscall = new VanadisSetRobustListSyscall( this, core_link, process, convertEvent<VanadisSyscallSetRobustListEvent*>( "set_robust_list", sys_ev ) );
        } break;
        case SYSCALL_OP_PRLIMIT: {
            syscall = new VanadisPrlimitSyscall( this, core_link, process, convertEvent<VanadisSyscallPrlimitEvent*>( "prlimit", sys_ev ) );
        } break;
        case SYSCALL_OP_SET_TID_ADDRESS: {
            syscall = new VanadisSetTidAddressSyscall( this, core_link, process, convertEvent<VanadisSyscallSetTidAddressEvent*>( "set_tid_address", sys_ev ) );
        } break;
        case SYSCALL_OP_MPROTECT: {
            syscall = new VanadisMprotectSyscall( this, core_link, process, convertEvent<VanadisSyscallMprotectEvent*>( "mprotect", sys_ev ) );
        } break;
        case SYSCALL_OP_SCHED_GETAFFINITY: {
            syscall = new VanadisGetaffinitySyscall( this, core_link, process, convertEvent<VanadisSyscallGetaffinityEvent*>( "getaffinity", sys_ev ) );
        } break;
        case SYSCALL_OP_SCHED_SETAFFINITY: {
            syscall = new VanadisSetaffinitySyscall( this, core_link, process, convertEvent<VanadisSyscallSetaffinityEvent*>( "setaffinity", sys_ev) );
        } break;
        case SYSCALL_OP_SCHED_YIELD: {
            syscall = new VanadisSchedYieldSyscall( this, core_link, process, convertEvent<VanadisSyscallSchedYieldEvent*>( "sched_yield", sys_ev) );
        } break;
        case SYSCALL_OP_FORK: {
            syscall = new VanadisForkSyscall( this, core_link, process, convertEvent<VanadisSyscallForkEvent*>( "fork", sys_ev ) );
        } break;
        case SYSCALL_OP_CLONE: {
            syscall = new VanadisCloneSyscall( this, core_link, process, convertEvent<VanadisSyscallCloneEvent*>( "clone", sys_ev ) );
        } break;
        case SYSCALL_OP_CLONE3: {
            syscall = new VanadisClone3Syscall( this, core_link, process, convertEvent<VanadisSyscallClone3Event*>( "clone3", sys_ev ) );
        } break;
        case SYSCALL_OP_FUTEX: {
            syscall = new VanadisFutexSyscall( this, core_link, process, convertEvent<VanadisSyscallFutexEvent*>( "futex", sys_ev ) );
        } break;
        case SYSCALL_OP_KILL: {
            syscall = new VanadisKillSyscall( this, core_link, process, convertEvent<VanadisSyscallKillEvent*>( "kill", sys_ev ) );
        } break;
        case SYSCALL_OP_TGKILL: {
            syscall = new VanadisTgKillSyscall( this, core_link, process, convertEvent<VanadisSyscallTgKillEvent*>( "tgkill", sys_ev ) );
        } break;
        case SYSCALL_OP_EXIT: {
            syscall = new VanadisExitSyscall( this, core_link, process, convertEvent<VanadisSyscallExitEvent*>( "exit", sys_ev ) );
        } break;
        case SYSCALL_OP_EXIT_GROUP: {
            syscall = new VanadisExitGroupSyscall( this, core_link, process, convertEvent<VanadisSyscallExitGroupEvent*>( "exitgroup", sys_ev ) );
        } break;
        case SYSCALL_OP_GETCPU: {
            syscall = new VanadisGetcpuSyscall( this, core_link, process, convertEvent<VanadisSyscallGetxEvent*>( "getcpu", sys_ev ) );
        } break;
        case SYSCALL_OP_GETPID: {
            syscall = new VanadisGetpidSyscall( this, core_link, process, convertEvent<VanadisSyscallGetxEvent*>( "getpid", sys_ev ) );
        } break;
        case SYSCALL_OP_GETTID: {
            syscall = new VanadisGettidSyscall( this, core_link, process, convertEvent<VanadisSyscallGetxEvent*>( "gettid", sys_ev ) );
        } break;
        case SYSCALL_OP_GETPPID: {
            syscall = new VanadisGetppidSyscall( this, core_link, process, convertEvent<VanadisSyscallGetxEvent*>( "getppid", sys_ev ) );
        } break;
        case SYSCALL_OP_GETPGID: {
            syscall = new VanadisGetpgidSyscall( this, core_link, process, convertEvent<VanadisSyscallGetxEvent*>( "getpgid", sys_ev ) );
        } break;
        case SYSCALL_OP_UNAME: {
            syscall = new VanadisUnameSyscall( this, core_link, process, convertEvent<VanadisSyscallUnameEvent*>( "uname", sys_ev ) );
        } break;
        case SYSCALL_OP_UNLINK: {
            syscall = new VanadisUnlinkSyscall( this, core_link, process, convertEvent<VanadisSyscallUnlinkEvent*>( "unlink", sys_ev ) );
        } break;
        case SYSCALL_OP_UNLINKAT: {
            syscall = new VanadisUnlinkatSyscall( this, core_link, process, convertEvent<VanadisSyscallUnlinkatEvent*>( "unlinkat", sys_ev ) );
        } break;
        case SYSCALL_OP_OPEN: {
            syscall = new VanadisOpenSyscall( this, core_link, process, convertEvent<VanadisSyscallOpenEvent*>( "open", sys_ev ) );
        } break;
        case SYSCALL_OP_OPENAT: {
            syscall = new VanadisOpenatSyscall( this, core_link, process, convertEvent<VanadisSyscallOpenatEvent*>( "openat", sys_ev ) );
        } break;
        case SYSCALL_OP_CLOSE: {
            syscall = new VanadisCloseSyscall( this, core_link, process, convertEvent<VanadisSyscallCloseEvent*>( "close", sys_ev ) );
        } break;
        case SYSCALL_OP_LSEEK: {
            syscall = new VanadisLseekSyscall( this, core_link, process, convertEvent<VanadisSyscallLseekEvent*>( "lseek", sys_ev ) );
        } break;
        case SYSCALL_OP_BRK: {
            syscall = new VanadisBrkSyscall( this, core_link, process, convertEvent<VanadisSyscallBRKEvent*>( "brk", sys_ev ) );
        } break;
        case SYSCALL_OP_MMAP: {
            syscall = new VanadisMmapSyscall( this, core_link, process, convertEvent<VanadisSyscallMemoryMapEvent*>( "mmap", sys_ev ) );
        } break;
        case SYSCALL_OP_UNMAP: {
            syscall = new VanadisUnmapSyscall( this, core_link, process, phys_mem_mgr_, convertEvent<VanadisSyscallMemoryUnMapEvent*>( "unmap", sys_ev ) );
        } break;
        case SYSCALL_OP_GETTIME64: {
            syscall = new VanadisGettime64Syscall( this, core_link, process, getNanoSeconds(), convertEvent<VanadisSyscallGetTime64Event*>( "gettime64", sys_ev ) );
        } break;
        case SYSCALL_OP_GETRANDOM: {
            syscall = new VanadisGetrandomSyscall( this, core_link, process, convertEvent<VanadisSyscallGetrandomEvent*>( "getrandom", sys_ev ) );
        } break;
        case SYSCALL_OP_ACCESS: {
            syscall = new VanadisAccessSyscall( this, core_link, process, convertEvent<VanadisSyscallAccessEvent*>( "access", sys_ev ) );
        } break;
        case SYSCALL_OP_READLINK: {
            syscall = new VanadisReadlinkSyscall( this, core_link, process, convertEvent<VanadisSyscallReadLinkEvent*>( "readlink", sys_ev ) );
        } break;
        case SYSCALL_OP_READLINKAT: {
            syscall = new VanadisReadlinkatSyscall( this, core_link, process, convertEvent<VanadisSyscallReadLinkAtEvent*>( "readlinkat", sys_ev ) );
        } break;
        case SYSCALL_OP_READ: {
            syscall = new VanadisReadSyscall( this, core_link, process, convertEvent<VanadisSyscallReadEvent*>( "read", sys_ev ) );
        } break;
        case SYSCALL_OP_READV: {
            syscall = new VanadisReadvSyscall( this, core_link, process, convertEvent<VanadisSyscallReadvEvent*>( "readv", sys_ev ) );
        } break;
        case SYSCALL_OP_WRITE: {
            syscall = new VanadisWriteSyscall( this, core_link, process, convertEvent<VanadisSyscallWriteEvent*>( "write", sys_ev ) );
        } break;
        case SYSCALL_OP_WRITEV: {
            syscall = new VanadisWritevSyscall( this, core_link, process, convertEvent<VanadisSyscallWritevEvent*>( "writev", sys_ev ) );
        } break;
        case SYSCALL_OP_IOCTL: {
            syscall = new VanadisIoctlSyscall( this, core_link, process, convertEvent<VanadisSyscallIoctlEvent*>( "ioctl", sys_ev ) );
        } break;
        case SYSCALL_OP_FSTAT: {
            syscall = new VanadisFstatSyscall( this, core_link, process, convertEvent<VanadisSyscallFstatEvent*>( "fstat", sys_ev ) );
        } break;
        case SYSCALL_OP_FSTATAT: {
            syscall = new VanadisFstatatSyscall( this, core_link, process, convertEvent<VanadisSyscallFstatAtEvent*>( "fstatat", sys_ev ) );
        } break;
        case SYSCALL_OP_STATX: {
            syscall = new VanadisStatxSyscall( this, core_link, process, convertEvent<VanadisSyscallStatxEvent*>( "statx", sys_ev ) );
        } break;

        default: {
            output_->fatal(CALL_INFO, -1,"Error - unknown operating system call, op %s, instPtr=%#" PRIx64 "\n",
                            SyscallName[sys_ev->getOperation()], sys_ev->getInstPtr() );
        } break;
    }

    return syscall;
}
