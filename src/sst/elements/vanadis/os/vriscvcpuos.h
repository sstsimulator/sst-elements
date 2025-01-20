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

#ifndef _H_VANADIS_RISCV_CPU_OS
#define _H_VANADIS_RISCV_CPU_OS

#define PRIuXX PRIu64
#define PRIdXX PRId64
#define PRIxXX PRIx64

#include "os/callev/voscallall.h"
#include "os/vcpuos2.h"
#include "os/voscallev.h"
#include <functional>

#include <fcntl.h>

#define RISC_CONVERT( x ) \
    if ( flags & RISCV_O_##x ) {\
        flags &= ~RISCV_O_##x;\
        out |= O_##x;\
    }

// This was generated on Ubuntu compiled with GCC.
// Does this need to generated with the cross compiler?
#define RISCV_O_RDONLY      0
#define RISCV_O_WRONLY      1
#define RISCV_O_RDWR        2
#define RISCV_O_APPEND      02000 
#define RISCV_O_ASYNC       020000 
#define RISCV_O_CLOEXEC     02000000 
#define RISCV_O_CREAT       0100 
#define RISCV_O_DIRECTORY   0200000
#define RISCV_O_DSYNC       010000 
#define RISCV_O_EXCL        0200  
#define RISCV_O_NOCTTY      0400  
#define RISCV_O_NOFOLLOW    0400000
#define RISCV_O_SYNC        04010000
#define RISCV_O_TRUNC       01000
#define RISCV_O_NONBLOCK    04000 
#define RISCV_O_NDELAY      RISCV_O_NONBLOCK 
#define RISCV_O_LARGEFILE   0100000 

#ifndef SST_COMPILE_MACOSX
#define RISCV_O_DIRECT      040000
#define RISCV_O_NOATIME     01000000  
#define RISCV_O_PATH        010000000 
#define RISCV_O_TMPFILE     020200000 
#endif

#define RISCV_MAP_STACK 0x20000
#define RISCV_MAP_ANONYMOUS 0x20
#define RISCV_MAP_PRIVATE 0x2
#define RISCV_MAP_FIXED 0x10

#define RISCV_SIGCHLD 17

#define VANADIS_SYSCALL_RISCV64_IOCTL 29
#define VANADIS_SYSCALL_RISCV64_RM_INOTIFY 28
#define VANADIS_SYSCALL_RISCV64_UNLINKAT 35 
#define VANADIS_SYSCALL_RISCV64_OPENAT 56
#define VANADIS_SYSCALL_RISCV64_CLOSE 57
#define VANADIS_SYSCALL_RISCV64_LSEEK 62
#define VANADIS_SYSCALL_RISCV64_READ 63
#define VANADIS_SYSCALL_RISCV64_WRITE 64
#define VANADIS_SYSCALL_RISCV64_READV 65
#define VANADIS_SYSCALL_RISCV64_WRITEV 66
#define VANADIS_SYSCALL_RISCV64_SPLICE 76
#define VANADIS_SYSCALL_RISCV64_READLINKAT 78 
#define VANADIS_SYSCALL_RISCV64_FSTATAT 79
#define VANADIS_SYSCALL_RISCV64_FSTAT 80
#define VANADIS_SYSCALL_RISCV64_EXIT 93
#define VANADIS_SYSCALL_RISCV64_EXIT_GROUP 94
#define VANADIS_SYSCALL_RISCV64_SET_TID_ADDRESS 96
#define VANADIS_SYSCALL_RISCV64_FUTEX 98
#define VANADIS_SYSCALL_RISCV64_SET_ROBUST_LIST 99
#define VANADIS_SYSCALL_RISCV64_GET_RLIST 100
#define VANADIS_SYSCALL_RISCV64_CLOCK_GETTIME 113
#define VANADIS_SYSCALL_RISCV64_SCHED_GETAFFINITY 123 
#define VANADIS_SYSCALL_RISCV64_KILL 129
#define VANADIS_SYSCALL_RISCV64_RT_SIGACTION 134
#define VANADIS_SYSCALL_RISCV64_RT_SIGPROCMASK 135
#define VANADIS_SYSCALL_RISCV64_GETPGID 155
#define VANADIS_SYSCALL_RISCV64_UNAME 160
#define VANADIS_SYSCALL_RISCV64_GETPID 172 
#define VANADIS_SYSCALL_RISCV64_GETPPID 173
#define VANADIS_SYSCALL_RISCV64_GETTID 178 
#define VANADIS_SYSCALL_RISCV64_BRK 214
#define VANADIS_SYSCALL_RISCV64_UNMAP 215
#define VANADIS_SYSCALL_RISCV64_CLONE 220
#define VANADIS_SYSCALL_RISCV64_MMAP 222
#define VANADIS_SYSCALL_RISCV64_MPROTECT 226 
#define VANADIS_SYSCALL_RISCV64_MADVISE 233
#define VANADIS_SYSCALL_RISCV64_PRLIMIT 261
#define VANADIS_SYSCALL_RISCV64_GETRANDOM 278
#define VANADIS_SYSCALL_RISCV64_CHECKPOINT 500 

#define VANADIS_SYSCALL_RISCV_RET_REG 10

#define InstallRISCV64FuncPtr( funcName ) Install_ISA_FuncPtr( RISCV64, funcName )

namespace SST {
namespace Vanadis {

template <typename T1, VanadisOSBitType BitType, int RegZero, int OsCodeReg, int LinkReg >
class VanadisRISCV64OSHandler2 : public VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg > {

    AddUsing;

public:
    VanadisRISCV64OSHandler2(ComponentId_t id, Params& params) : 
        VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >(id, params, "RISCV" ) 
    {
        InstallCommonFuncs( RISCV64 )

        InstallRISCV64FuncPtr( MMAP );
        InstallRISCV64FuncPtr( CLONE );
        InstallRISCV64FuncPtr( FUTEX );
        InstallRISCV64FuncPtr( CLOCK_GETTIME );
        InstallRISCV64FuncPtr( SET_ROBUST_LIST );
        InstallRISCV64FuncPtr( PRLIMIT );
        InstallRISCV64FuncPtr( READLINKAT );
        InstallRISCV64FuncPtr( GETRANDOM );
        InstallRISCV64FuncPtr( FSTATAT );
        InstallRISCV64FuncPtr( LSEEK );
        InstallRISCV64FuncPtr( CHECKPOINT );
    }

    virtual ~VanadisRISCV64OSHandler2() {}

    VanadisSyscallEvent* CHECKPOINT( int hw_thr ) {
        output->verbose(CALL_INFO, 8, 0, "checkpoint()\n");
        return new VanadisSyscallCheckpointEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B );
    }

    VanadisSyscallEvent* CLONE( int hw_thr ) {
        uint64_t flags          = getArgRegister(0);
        uint64_t threadStack    = getArgRegister(1);
        int64_t  ptid           = getArgRegister(2);
        int64_t  tls            = getArgRegister(3);
        int64_t  ctid           = getArgRegister(4);

        output->verbose(CALL_INFO, 8, 0,
                    "clone( %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ")\n",
                    instPtr,threadStack,flags,ptid,tls,ctid);
        return new VanadisSyscallCloneEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, instPtr,
                    threadStack, flags, ptid, tls, ctid );
    }

    VanadisSyscallEvent* GETRANDOM( int hw_thr ) {
        uint64_t buf    = getArgRegister( 0 );
        uint64_t buflen = getArgRegister( 1 );
        uint64_t flags  = getArgRegister( 2 );

        output->verbose(CALL_INFO, 8, 0, "getrandom( %" PRIu64 ", %" PRIu64 ", %#" PRIx64 ")\n", buf, buflen, flags);

        return new VanadisSyscallGetrandomEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, buf, buflen, flags );
    }

    VanadisSyscallEvent* READLINKAT( int hw_thr ) {
        uint64_t dirfd      = getArgRegister( 0 );
        uint64_t pathname   = getArgRegister( 1 );
        uint64_t buf        = getArgRegister( 2 );
        uint64_t bufsize    = getArgRegister( 3 );

#ifdef SST_COMPILE_MACOSX
        if (  VANADIS_AT_FDCWD == dirfd ) {
            dirfd = AT_FDCWD;
        }
#endif

        output->verbose(CALL_INFO, 8, 0, "readlinkat( %" PRIu64 ", %#" PRIx64 ", %#" PRIx64 ", %" PRIu64 ")\n",
                    dirfd, pathname, buf, bufsize);

        return new VanadisSyscallReadLinkAtEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, dirfd, pathname, buf, bufsize);
    }

    VanadisSyscallEvent* FSTATAT( int hw_thr ) {
        int32_t  dirfd      = getArgRegister(0);
        uint64_t pathname   = getArgRegister(1);
        uint64_t statbuf    = getArgRegister(2);
        uint64_t flags      = getArgRegister(3);

#ifdef SST_COMPILE_MACOSX
        if (  VANADIS_AT_FDCWD == dirfd ) {
            dirfd = AT_FDCWD;
        }
#endif

        output->verbose(CALL_INFO, 8, 0, "fstat( %d, %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ")\n",dirfd,pathname,statbuf,flags);

        return new VanadisSyscallFstatAtEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, dirfd, pathname, statbuf, flags );
    }

    VanadisSyscallEvent* FUTEX( int hw_thr ) {
        uint64_t addr           = getArgRegister(0);
        int32_t  op             = getArgRegister(1);
        uint32_t val            = getArgRegister(2);
        uint64_t timeout_addr   = getArgRegister(3);
        uint64_t addr2          = getArgRegister(4);
        uint32_t val3           = getArgRegister(5);

        output->verbose(CALL_INFO, 8, 0,
                            "futex( %#" PRIx64 ", %" PRIx32 ", %" PRIx32 ", %" PRIx64 ", %" PRIx64 ", %" PRIu32 " )\n",
                            addr, op, val, timeout_addr, addr2, val3);

        return new VanadisSyscallFutexEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, addr, op, val, timeout_addr, addr2, val3 );
    }

    VanadisSyscallEvent* SET_ROBUST_LIST( int hw_thr ) {
        uint64_t  head  = getArgRegister( 0 );
        uint64_t  len   = getArgRegister( 1 );

        output->verbose(CALL_INFO, 8, 0, "set_robust_list(  %#" PRIx64 ", %" PRIu64" )\n",head,len);

        return new VanadisSyscallSetRobustListEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, head, len );
    }

    VanadisSyscallEvent* MMAP( int hw_thr ) {
        uint64_t map_addr   = getArgRegister( 0 );
        uint64_t map_len    = getArgRegister( 1 );
        int32_t  map_prot   = getArgRegister( 2 );
        int32_t  map_flags  = getArgRegister( 3 );
        int32_t  fd         = getArgRegister( 4 );
        int64_t  offset     = getArgRegister( 5 );

        int32_t hostFlags = 0;

        if ( map_flags & RISCV_MAP_FIXED ) {
            output->verbose(CALL_INFO, 8, 0,"mmap() we don't support MAP_FIXED return error EEXIST\n");

            recvSyscallResp(new VanadisSyscallResponse(-EEXIST));
            return nullptr;
        } else {

            // ignore this flag
            if ( map_flags & RISCV_MAP_STACK ) {
                map_flags &= ~RISCV_MAP_STACK;
            }
            if ( map_flags & RISCV_MAP_ANONYMOUS ) {
                hostFlags |= MAP_ANONYMOUS; 
                map_flags &= ~RISCV_MAP_ANONYMOUS;
            }
            if ( map_flags & RISCV_MAP_PRIVATE ) {
                hostFlags |= MAP_PRIVATE; 
                map_flags &= ~RISCV_MAP_PRIVATE;
            }
            assert( map_flags == 0 );

            output->verbose(CALL_INFO, 8, 0,
                        "mmap2( 0x%" PRI_ADDR ", %" PRIu64 ", %" PRId32 ", %" PRId32 ", %d, %" PRIu64 ")\n",
                        map_addr, map_len, map_prot, map_flags, fd, offset );

            return new VanadisSyscallMemoryMapEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B,
                    map_addr, map_len, map_prot, hostFlags, fd, offset, 0, 0);
        }
    }

    VanadisSyscallEvent* CLOCK_GETTIME( int hw_thr ) {
        int64_t  clk_type   = getArgRegister( 0 );
        uint64_t time_addr  = getArgRegister( 1 );

        output->verbose(CALL_INFO, 8, 0,
                            "clock_gettime64( %" PRId64 ", 0x%" PRI_ADDR " )\n", clk_type, time_addr);

        return new VanadisSyscallGetTime64Event(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, clk_type, time_addr);
    }

    VanadisSyscallEvent* RT_SIGPROCMASK( int hw_thr ) {
        int32_t  how                = getArgRegister( 0 );
        uint64_t signal_set_in      = getArgRegister( 1 );
        uint64_t signal_set_out     = getArgRegister( 2 );
        int32_t  signal_set_size    = getArgRegister( 3 );

        output->verbose(CALL_INFO, 8, 0,
                            "rt_sigprocmask( %" PRId32 ", 0x%" PRI_ADDR ", 0x%" PRI_ADDR ", %" PRId32 ")\n",
                            how, signal_set_in, signal_set_out, signal_set_size);

        printf("Warning: VANADIS_SYSCALL_RISCV_RT_SIGPROCMASK not implemented return success\n");

        recvSyscallResp(new VanadisSyscallResponse(0));
        return nullptr;
    }

    VanadisSyscallEvent* PRLIMIT( int hw_thr ) {
        uint64_t pid        = getArgRegister( 0 );
        uint64_t resource   = getArgRegister( 1 );
        uint64_t new_limit  = getArgRegister( 2 );
        uint64_t old_limit  = getArgRegister( 3 );

        output->verbose(CALL_INFO, 8, 0,
                            "prlimit( %" PRIu64 ", %" PRIu64 ",  %#" PRIx64 ", %#" PRIx64 ")\n", pid, resource, new_limit, old_limit );

        return new VanadisSyscallPrlimitEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, pid, resource, new_limit, old_limit);
    }

    VanadisSyscallEvent* LSEEK( int hw_thr ) {
        int32_t fd       = getArgRegister(0);
        uint64_t offset   = getArgRegister(1);
        int32_t whence   = getArgRegister(2);

        output->verbose(CALL_INFO, 8, 0, "lseek( %" PRId32 ", %" PRIu64 ", %" PRId32 " )\n", fd, offset, whence);
        return new VanadisSyscallLseekEvent(core_id, hw_thr, BitType, fd, offset, whence);
    }


    void recvSyscallResp( VanadisSyscallResponse* os_resp ) {
        output->verbose(CALL_INFO, 8, 0, "return-code: %" PRId64 " (success: %3s)\n",
                            os_resp->getReturnCode(), os_resp->isSuccessful() ? "yes" : "no");
        output->verbose(CALL_INFO, 8, 0, "issuing call-backs to clear syscall ROB stops...\n");

        // Set up the return code (according to ABI, this goes in r10)
        const uint16_t rc_reg = isaTable->getIntPhysReg( VANADIS_SYSCALL_RISCV_RET_REG );
        const int64_t rc_val = (int64_t)os_resp->getReturnCode();
        regFile->setIntReg(rc_reg, rc_val);

        delete os_resp;
    }

private:

    uint64_t convertFlags( uint64_t flags ) {
        uint64_t out = 0;

        RISC_CONVERT( RDONLY );
        RISC_CONVERT( WRONLY );
        RISC_CONVERT( RDWR );
        RISC_CONVERT( APPEND );
        RISC_CONVERT( ASYNC );
        RISC_CONVERT( CLOEXEC );
        RISC_CONVERT( CREAT );
        RISC_CONVERT( DIRECTORY );
        RISC_CONVERT( DSYNC );
        RISC_CONVERT( EXCL );
        RISC_CONVERT( NOCTTY );
        RISC_CONVERT( NOFOLLOW );
        RISC_CONVERT( SYNC );
        RISC_CONVERT( TRUNC );
        RISC_CONVERT( NONBLOCK );
        RISC_CONVERT( NDELAY );

#ifdef SST_COMPILE_MACOSX
        flags &= ~RISCV_O_LARGEFILE; 
#else
        RISC_CONVERT( DIRECT );
        RISC_CONVERT( LARGEFILE );
        RISC_CONVERT( NOATIME );
        RISC_CONVERT( PATH );
        RISC_CONVERT( TMPFILE );
#endif
        if ( flags ) {
            printf("%s() all flags have not been converted %#" PRIx64 "\n",__func__,flags);
            assert( 0 );
        }

        return out;
    }
};

#define RISCV_ARG_REG_ZERO 10
#define RISCV_OS_CODE_REG 17
#define RISCV_LINK_REG 31

class VanadisRISCV64OSHandler :
    public VanadisRISCV64OSHandler2< uint64_t, VanadisOSBitType::VANADIS_OS_64B, RISCV_ARG_REG_ZERO, RISCV_OS_CODE_REG, RISCV_LINK_REG > {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(VanadisRISCV64OSHandler,
                                            "vanadis",
                                            "VanadisRISCV64OSHandler",
                                            SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                            "Provides SYSCALL handling for a RISCV-based decoding core",
                                            SST::Vanadis::VanadisCPUOSHandler)

    VanadisRISCV64OSHandler(ComponentId_t id, Params& params) : 
        VanadisRISCV64OSHandler2<uint64_t, VanadisOSBitType::VANADIS_OS_64B, RISCV_ARG_REG_ZERO, RISCV_OS_CODE_REG, RISCV_LINK_REG >(id, params) { }
};

} // namespace Vanadis
} // namespace SST

#endif
