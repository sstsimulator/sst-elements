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

#ifndef _H_VANADIS_MIPS_CPU_OS
#define _H_VANADIS_MIPS_CPU_OS

#include "os/callev/voscallall.h"
#include "os/resp/voscallresp.h"
#include "os/vstartthreadreq.h"
#include "os/vcpuos.h"
#include "os/voscallev.h"
#include <functional>
#include <sys/mman.h>

#include <fcntl.h>

#define MIPS_CONVERT( x ) \
    if ( flags & MIPS_O_##x ) {\
        flags &= ~MIPS_O_##x;\
        out |= O_##x;\
    }

#define MIPS_MMAP_CONVERT( x ) \
    if ( flags & MIPS_MAP_##x ) {\
        flags &= ~MIPS_MAP_##x;\
        out |= MAP_##x;\
    }

#define MIPS_MAP_FIXED   0x010

#define MIPS_O_RDONLY    0
#define MIPS_O_WRONLY    0x1
#define MIPS_O_RDWR      0x2
#define MIPS_O_APPEND    0x8
#define MIPS_O_ASYNC     0x1000
#define MIPS_O_CLOEXEC   0x80000
#define MIPS_O_CREAT     0x100
#define MIPS_O_DIRECTORY 0x10000
#define MIPS_O_DSYNC     0x10
#define MIPS_O_EXCL      0x400
#define MIPS_O_NOCTTY    0x800
#define MIPS_O_NOFOLLOW  0x20000
#define MIPS_O_SYNC      0x4010
#define MIPS_O_TRUNC     0x200
#define MIPS_O_NONBLOCK  0x80
#define MIPS_O_NDELAY    0x80

#ifndef SST_COMPILE_MACOSX
#define MIPS_O_DIRECT    0x8000
#define MIPS_O_LARGEFILE 0x2000
#define MIPS_O_NOATIME   0x40000
#define MIPS_O_PATH      0x200000
#define MIPS_O_TMPFILE   0x410000
#endif

#define MIPS_MAP_ANONYMOUS 0x800
#define MIPS_MAP_PRIVATE   0x2

#define VANADIS_SYSCALL_MIPS_EXIT 4001
#define VANADIS_SYSCALL_MIPS_FORK 4002
#define VANADIS_SYSCALL_MIPS_READ 4003
#define VANADIS_SYSCALL_MIPS_WRITE 4004
#define VANADIS_SYSCALL_MIPS_OPEN 4005
#define VANADIS_SYSCALL_MIPS_CLOSE 4006
#define VANADIS_SYSCALL_MIPS_UNLINK 4010
#define VANADIS_SYSCALL_MIPS_GETPID 4020
#define VANADIS_SYSCALL_MIPS_ACCESS 4033
#define VANADIS_SYSCALL_MIPS_KILL 4037
#define VANADIS_SYSCALL_MIPS_BRK 4045
#define VANADIS_SYSCALL_MIPS_IOCTL 4054
#define VANADIS_SYSCALL_MIPS_GETPPID 4064
#define VANADIS_SYSCALL_MIPS_READLINK 4085
#define VANADIS_SYSCALL_MIPS_MMAP 4090
#define VANADIS_SYSCALL_MIPS_UNMAP 4091
#define VANADIS_SYSCALL_MIPS_CLONE 4120
#define VANADIS_SYSCALL_MIPS_UNAME 4122
#define VANADIS_SYSCALL_MIPS_MPROTECT 4125
#define VANADIS_SYSCALL_MIPS_GETPGID 4132
#define VANADIS_SYSCALL_MIPS_READV 4145
#define VANADIS_SYSCALL_MIPS_WRITEV 4146
#define VANADIS_SYSCALL_MIPS_RT_SIGACTION 4194
#define VANADIS_SYSCALL_MIPS_RT_SIGPROCMASK 4195
#define VANADIS_SYSCALL_MIPS_MMAP2 4210
#define VANADIS_SYSCALL_MIPS_FSTAT 4215
#define VANADIS_SYSCALL_MIPS_MADVISE 4218
#define VANADIS_SYSCALL_MIPS_GETTID 4222
#define VANADIS_SYSCALL_MIPS_FUTEX 4238
#define VANADIS_SYSCALL_MIPS_SCHED_GETAFFINITY 4240
#define VANADIS_SYSCALL_MIPS_EXIT_GROUP 4246
#define VANADIS_SYSCALL_MIPS_SET_TID_ADDRESS 4252
#define VANADIS_SYSCALL_MIPS_SET_THREAD_AREA 4283
#define VANADIS_SYSCALL_MIPS_RM_INOTIFY 4286
#define VANADIS_SYSCALL_MIPS_OPENAT 4288
#define VANADIS_SYSCALL_MIPS_UNLINKAT 4294
#define VANADIS_SYSCALL_MIPS_STATX 4366
#define VANADIS_SYSCALL_MIPS_GETTIME64 4403

#define InstallMIPSFuncPtr( funcName ) Install_ISA_FuncPtr( MIPS, funcName )

namespace SST {
namespace Vanadis {

template <typename T1, VanadisOSBitType BitType, int RegZero, int OsCodeReg, int LinkReg >
class VanadisMIPSOSHandler2 : public VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg > {

    AddUsing;

public:
    VanadisMIPSOSHandler2(ComponentId_t id, Params& params) :
        VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >(id, params, "MIPS" ) 
    {
        InstallCommonFuncs( MIPS )

        InstallMIPSFuncPtr( READLINK );
        InstallMIPSFuncPtr( UNLINK );
        InstallMIPSFuncPtr( ACCESS );
        InstallMIPSFuncPtr( OPEN );
        InstallMIPSFuncPtr( CLONE );
        InstallMIPSFuncPtr( FORK );
        InstallMIPSFuncPtr( FUTEX );
        InstallMIPSFuncPtr( GETTIME64 );
        InstallMIPSFuncPtr( MMAP2 );
        InstallMIPSFuncPtr( SET_THREAD_AREA );
        InstallMIPSFuncPtr( STATX );
    }

    virtual ~VanadisMIPSOSHandler2() {}

    VanadisSyscallEvent* READLINK( int hw_thr ) {
        T1 path       = getArgRegister(0);
        T1 buf_ptr    = getArgRegister(1);
        T1 size        = getArgRegister(2);

        output->verbose(CALL_INFO, 8, 0, "readLink( %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ")\n",path,buf_ptr,size);
        return new VanadisSyscallReadLinkEvent(core_id, hw_thr, BitType, path, buf_ptr, size);
    }


    VanadisSyscallEvent* UNLINK( int hw_thr ) {
        T1 path_addr = getArgRegister( 0 );

        output->verbose(CALL_INFO, 8, 0, "unlink( %" PRId32 " )\n",path_addr);

        return new VanadisSyscallUnlinkEvent(core_id, hw_thr, BitType, path_addr);
    }

    VanadisSyscallEvent* OPEN( int hw_thr ) {
        T1 path_ptr   = getArgRegister(0);
        T1 flags      = getArgRegister(1);
        T1 mode       = getArgRegister(2);

        output->verbose(CALL_INFO, 8, 0, "open( %#" PRIx64 ", %" PRIu64 ", %" PRIu64 " )\n", path_ptr, flags, mode);

        return new VanadisSyscallOpenEvent(core_id, hw_thr, BitType, path_ptr, convertFlags(flags), mode);
    }

    VanadisSyscallEvent* ACCESS( int hw_thr ) {
        T1 path_ptr       = getArgRegister(0);
        T1 access_mode    = getArgRegister(1);

        output->verbose(CALL_INFO, 8, 0, "access( 0x%llx, %" PRIu64 " )\n", path_ptr, access_mode);
        return new VanadisSyscallAccessEvent(core_id, hw_thr, BitType, path_ptr, access_mode);
    }

    VanadisSyscallEvent* CLONE( int hw_thr ) {
        uint32_t flags       = getArgRegister(0);
        uint32_t threadStack = getArgRegister(1);
        int32_t  ptid        = getArgRegister(2);
        int32_t  tls         = getArgRegister(3);
        uint32_t stackPtr    = getSP();

        assert( stackPtr );

        output->verbose(CALL_INFO, 8, 0,
                "clone( %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 " )\n",
                flags, threadStack, ptid, tls, stackPtr );

        return new VanadisSyscallCloneEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, instPtr, threadStack, flags, ptid, tls, 0, stackPtr );
    }

    VanadisSyscallEvent* FORK( int hw_thr ) {
        output->verbose(CALL_INFO, 8, 0, "fork()\n");
        return new VanadisSyscallForkEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B);
    }

    VanadisSyscallEvent* SET_THREAD_AREA( int hw_thr ) {
        T1 ptr = getArgRegister(0);

        output->verbose(CALL_INFO, 8, 0, "set_thread_area( %#" PRIx64 ")\n", ptr);

        if (tls_address != nullptr) {
            (*tls_address) = ptr;
        } else {
            assert(0);
        }
        recvSyscallResp(new VanadisSyscallResponse(0));
        return nullptr;
    }

    VanadisSyscallEvent* STATX( int hw_thr ) {
        int32_t  dirfd      = getArgRegister(0);
        uint32_t pathname   = getArgRegister(1);
        int32_t  flags      = getArgRegister(2);
        uint32_t mask       = getArgRegister(3);
        uint32_t stackPtr   = getSP();

#ifdef SST_COMPILE_MACOSX
       if (  VANADIS_AT_FDCWD == dirfd ) {
           dirfd = AT_FDCWD;
       }
#endif

        output->verbose(CALL_INFO, 8, 0, "statx( %" PRId32 ", %#" PRIx64 ", %#" PRIx32 ", %#" PRIx32 ", %#" PRIx64 " )\n",
                        dirfd, pathname, flags, mask, stackPtr );

        return new VanadisSyscallStatxEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, dirfd, pathname, flags, mask, stackPtr);
    }

    VanadisSyscallEvent* FUTEX( int hw_thr ) {
        uint32_t addr           = getArgRegister(0);
        int32_t  op             = getArgRegister(1);
        uint32_t val            = getArgRegister(2);
        uint32_t timeout_addr   = getArgRegister(3);
        uint32_t stack_ptr      = getSP();

        output->verbose(CALL_INFO, 8, 0, "futex( 0x%llx, %" PRId32 ", %" PRIu32 ", %" PRIu64 ", sp: 0x%llx (arg-count is greater than 4))\n",
                            addr, op, val, timeout_addr, stack_ptr);

        return new VanadisSyscallFutexEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, addr, op, val, timeout_addr, stack_ptr );
    }

    VanadisSyscallEvent* MMAP2( int hw_thr ) {
        uint32_t addr       = getArgRegister(0);
        uint32_t len        = getArgRegister(1);
        int32_t  prot       = getArgRegister(2);
        int32_t  flags      = getArgRegister(3);
        uint32_t stack_ptr  = getSP();

        output->verbose(CALL_INFO, 8, 0, "mmap2( 0x%llx, %" PRIu64 ", %" PRId32 ", %" PRId32 ", sp: 0x%llx (> 4 arguments) )\n",
                            addr, len, prot, flags, stack_ptr);

        if ( flags & MIPS_MAP_FIXED ) {
            output->verbose(CALL_INFO, 8, 0,"mmap2() we don't support MAP_FIXED return error EEXIST\n");

            recvSyscallResp(new VanadisSyscallResponse(-EEXIST));
            return nullptr;
        } else {

            return new VanadisSyscallMemoryMapEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B,
                addr, len, prot, mmapConvertFlags(flags), /*fd*/ 0, /*offset*/ 0, stack_ptr, 4096);
        }
    }

    VanadisSyscallEvent* GETTIME64( int hw_thr) {
        int32_t  clk_type   = getArgRegister(0);
        uint32_t time_addr  = getArgRegister(1);

        output->verbose(CALL_INFO, 8, 0, "clock_gettime64( %" PRId64 ", 0x%llx )\n", clk_type, time_addr);

        return new VanadisSyscallGetTime64Event(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, clk_type, time_addr);
    }

#if 0
        case VANADIS_SYSCALL_MIPS_RM_INOTIFY: {
            output->verbose(CALL_INFO, 8, 0, "inotify_rm_watch(), by-passing and removing.\n");
            const uint16_t rc_reg = isaTable->getIntPhysReg(7);
            regFile->setIntReg(rc_reg, (uint32_t)0);

            writeSyscallResult(true);
        } break;
#endif

    void recvSyscallResp( VanadisSyscallResponse* os_resp ) {
        output->verbose(CALL_INFO, 8, 0, "syscall return-code: %" PRId64 " (success: %3s)\n",
                            os_resp->getReturnCode(), os_resp->isSuccessful() ? "yes" : "no");
        output->verbose(CALL_INFO, 8, 0, "-> issuing call-backs to clear syscall ROB stops...\n");

        // Set up the return code (according to ABI, this goes in r2)
        const uint16_t rc_reg = isaTable->getIntPhysReg(2);
        const int32_t rc_val = (int32_t)os_resp->getReturnCode();
        regFile->setIntReg(rc_reg, rc_val);

        if (os_resp->isSuccessful()) {
            if (rc_val < 0) {
                writeSyscallResult(false);
            } else {
                // Generate correct markers for OS return code checks
                writeSyscallResult(os_resp->isSuccessful());
            }
        } else {
            writeSyscallResult(false);
        }
        delete os_resp;
    }

private:
    void writeSyscallResult(const bool success) {
        const uint32_t os_success = 0;
        const uint32_t os_failed = 1;
        const uint16_t succ_reg = isaTable->getIntPhysReg(7);

        if (success) {
            output->verbose(CALL_INFO, 8, 0,
                            "syscall - generating successful (v: 0) result (isa-reg: "
                            "7, phys-reg: %" PRIu16 ")\n",
                            succ_reg);

            regFile->setIntReg(succ_reg, os_success);
        } else {
            output->verbose(CALL_INFO, 8, 0,
                            "syscall - generating failed (v: 1) result (isa-reg: 7, "
                            "phys-reg: %" PRIu16 ")\n",
                            succ_reg);

            regFile->setIntReg(succ_reg, os_failed);
        }
    }

    uint32_t mmapConvertFlags( uint32_t flags ) {
        uint32_t out = 0;
        MIPS_MMAP_CONVERT( ANONYMOUS );
        MIPS_MMAP_CONVERT( PRIVATE );
        MIPS_MMAP_CONVERT( FIXED );
        assert( 0 == flags );
        return out;
    }

	uint32_t convertFlags( uint32_t flags ) {
		uint32_t out = 0;

		MIPS_CONVERT( RDONLY );
		MIPS_CONVERT( WRONLY );
		MIPS_CONVERT( RDWR );
		MIPS_CONVERT( APPEND );
		MIPS_CONVERT( ASYNC );
		MIPS_CONVERT( CLOEXEC );
		MIPS_CONVERT( CREAT );
		MIPS_CONVERT( DIRECTORY );
		MIPS_CONVERT( DSYNC );
		MIPS_CONVERT( EXCL );
		MIPS_CONVERT( NOCTTY );
		MIPS_CONVERT( NOFOLLOW );
		MIPS_CONVERT( SYNC );
		MIPS_CONVERT( TRUNC );
		MIPS_CONVERT( NONBLOCK );
		MIPS_CONVERT( NDELAY );

#ifndef SST_COMPILE_MACOSX
		MIPS_CONVERT( DIRECT );
		MIPS_CONVERT( LARGEFILE );
		MIPS_CONVERT( NOATIME );
		MIPS_CONVERT( PATH );
		MIPS_CONVERT( TMPFILE );
#else
        // remove O_LARGEFILE 
        flags &= ~0x2000;
#endif

        assert( 0 == flags );

		return out;
	}
    uint32_t getSP( ) {
        return regFile->template getIntReg<uint32_t>( isaTable->getIntPhysReg( 29 ) );
    }
};

#define MIPS_ARG_REG_ZERO 4 
#define MIPS_OS_CODE_REG 2 
#define MIPS_LINK_REG 31

class VanadisMIPSOSHandler : public VanadisMIPSOSHandler2< uint32_t, VanadisOSBitType::VANADIS_OS_32B, MIPS_ARG_REG_ZERO, MIPS_OS_CODE_REG, MIPS_LINK_REG > {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(VanadisMIPSOSHandler, "vanadis",
                                            "VanadisMIPSOSHandler",
                                            SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                            "Provides SYSCALL handling for a MIPS-based decoding core",
                                            SST::Vanadis::VanadisCPUOSHandler)

    VanadisMIPSOSHandler(ComponentId_t id, Params& params) : 
        VanadisMIPSOSHandler2<uint32_t, VanadisOSBitType::VANADIS_OS_32B, MIPS_ARG_REG_ZERO, MIPS_OS_CODE_REG, MIPS_LINK_REG >(id, params) { }
};

} // namespace Vanadis
} // namespace SST

#endif
