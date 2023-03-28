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

#ifndef _H_VANADIS_RISCV_CPU_OS
#define _H_VANADIS_RISCV_CPU_OS

#include "os/callev/voscallall.h"
#include "os/vcpuos.h"
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

#define RISCV_MAP_ANONYMOUS 0x20
#define RISCV_MAP_PRIVATE 0x2
#define RISCV_MAP_FIXED 0x10

#define RISCV_AT_FDCWD     -100

#define RISCV_SIGCHLD 17

#define VANADIS_SYSCALL_RISCV_UNLINKAT 35 
#define VANADIS_SYSCALL_RISCV_READ 63
#define VANADIS_SYSCALL_RISCV_CLOSE 57
#define VANADIS_SYSCALL_RISCV_WRITE 64
#define VANADIS_SYSCALL_RISCV_BRK 214
#define VANADIS_SYSCALL_RISCV_IOCTL 29
#define VANADIS_SYSCALL_RISCV_MMAP 222
#define VANADIS_SYSCALL_RISCV_UNMAP 215
#define VANADIS_SYSCALL_RISCV_UNAME 160
#define VANADIS_SYSCALL_RISCV_READV 65
#define VANADIS_SYSCALL_RISCV_WRITEV 66
#define VANADIS_SYSCALL_RISCV_FSTATAT 79
#define VANADIS_SYSCALL_RISCV_FSTAT 80
#define VANADIS_SYSCALL_RISCV_RT_SIGPROCMASK 135
#define VANADIS_SYSCALL_RISCV_RT_SIGACTION 134
#define VANADIS_SYSCALL_RISCV_MADVISE 233
#define VANADIS_SYSCALL_RISCV_FUTEX 98
#define VANADIS_SYSCALL_RISCV_SET_TID_ADDRESS 96
#define VANADIS_SYSCALL_RISCV_CLONE 220
#define VANADIS_SYSCALL_RISCV_EXIT 93
#define VANADIS_SYSCALL_RISCV_EXIT_GROUP 94

#define VANADIS_SYSCALL_RISCV_SET_ROBUST_LIST 99
#define VANADIS_SYSCALL_RISCV_RM_INOTIFY 28
#define VANADIS_SYSCALL_RISCV_OPENAT 56
#define VANADIS_SYSCALL_RISCV_SET_RLIST 99
#define VANADIS_SYSCALL_RISCV_GET_RLIST 100
#define VANADIS_SYSCALL_RISCV_CLOCK_GETTIME 113
#define VANADIS_SYSCALL_RISCV_GETPGID 155
#define VANADIS_SYSCALL_RISCV_GETPID 172 
#define VANADIS_SYSCALL_RISCV_GETPPID 173
#define VANADIS_SYSCALL_RISCV_GETTID 178 
#define VANADIS_SYSCALL_RISCV_MPROTECT 226 
#define VANADIS_SYSCALL_RISCV_KILL 129
#define VANADIS_SYSCALL_RISCV_PRLIMIT 261
#define VANADIS_SYSCALL_RISCV_SPLICE 76
#define VANADIS_SYSCALL_RISCV_READLINKAT 78 
#define VANADIS_SYSCALL_RISCV_SCHED_GETAFFINITY 123 
#define VANADIS_SYSCALL_RISCV_GETRANDOM 278

#define VANADIS_SYSCALL_RISCV_THREAD_REG 4 
#define VANADIS_SYSCALL_RISCV_RET_REG 10

namespace SST {
namespace Vanadis {

template <typename T1, VanadisOSBitType BitType, int RegZero, int OsCodeReg, int LinkReg >
class VanadisRISCV64OSHandler2 : public VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg > {

    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::output;
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::regFile;
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::core_id;
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::isaTable;
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::sendSyscallEvent;
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::getRegister;
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::getLinkReg;
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::getOsCode;
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::uname;

public:
    VanadisRISCV64OSHandler2(ComponentId_t id, Params& params) : 
        VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >(id, params) {}

    virtual ~VanadisRISCV64OSHandler2() {}

    virtual std::tuple<bool,bool> handleSysCall(VanadisSysCallInstruction* syscallIns) {
        uint64_t instPtr = syscallIns->getInstructionAddress();
        uint64_t call_link_value = getLinkReg();

        bool flushLSQ = false;
        const uint32_t hw_thr = syscallIns->getHWThread();

        const uint64_t os_code = getOsCode();

        output->verbose(CALL_INFO, 8, 0, "core=%d hw_thr=%d syscall-ins: 0x%0llx, link-reg: 0x%llx, os_code=%d\n",
                        core_id, hw_thr, syscallIns->getInstructionAddress(), call_link_value, os_code);
        VanadisSyscallEvent* call_ev = nullptr;

        switch (os_code) {

        case VANADIS_SYSCALL_RISCV_CLONE: {
            uint64_t flags          = getRegister(0);
            uint64_t threadStack    = getRegister(1);
            int64_t  ptid           = getRegister(2);
            int64_t  tls            = getRegister(3);
            int64_t  ctid           = getRegister(4);

            if ( flags == RISCV_SIGCHLD ) {
                output->verbose(CALL_INFO, 8, 0,"clone( flags = RISCV_SIGCHLD )\n");
                call_ev = new VanadisSyscallForkEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B);
            } else { 
                output->verbose(CALL_INFO, 8, 0,
                    "clone( %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ")\n",
                    instPtr,threadStack,flags,ptid,tls,ctid);
                call_ev = new VanadisSyscallCloneEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, instPtr,
                    threadStack, flags, ptid, tls, ctid );
            }
        } break;

        case VANADIS_SYSCALL_RISCV_SCHED_GETAFFINITY: {
            uint64_t pid        = getRegister(0);
            uint64_t cpusetsize = getRegister(1);
            int64_t maskAddr    = getRegister(2);

            output->verbose(CALL_INFO, 8, 0,
                            "sched_getaffinity( %" PRId64 ", %" PRId64", %#" PRIx64 " )\n",
                                pid, cpusetsize, maskAddr );

            call_ev = new VanadisSyscallGetaffinityEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, pid, cpusetsize, maskAddr );

        } break;

        case VANADIS_SYSCALL_RISCV_MPROTECT: {
            uint64_t addr   = getRegister(0);
            uint64_t len    = getRegister(1);
            int64_t  prot   = getRegister(2);

            int myProt = 0;
            if ( prot & 0x1 ) {
                myProt |= PROT_READ;
            }
            if ( prot & 0x2 ) {
                myProt |= PROT_WRITE;
            }

            output->verbose(CALL_INFO, 8, 0,"mprotect( %#" PRIx64 ", %zu, %#" PRIx64 ")\n",addr,len,myProt);
            call_ev = new VanadisSyscallMprotectEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, addr, len, myProt );
        } break;

        case VANADIS_SYSCALL_RISCV_GETPID: {
            output->verbose(CALL_INFO, 8, 0,"getgid()\n");
            call_ev = new VanadisSyscallGetxEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B,SYSCALL_OP_GETPID);
        } break;

        case VANADIS_SYSCALL_RISCV_GETPGID: {
            output->verbose(CALL_INFO, 8, 0,"getpgid()\n");
            call_ev = new VanadisSyscallGetxEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B,SYSCALL_OP_GETPGID);
        } break;

        case VANADIS_SYSCALL_RISCV_GETPPID: {
            output->verbose(CALL_INFO, 8, 0,"getpid()\n");
            call_ev = new VanadisSyscallGetxEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B,SYSCALL_OP_GETPPID);
        } break;

        case VANADIS_SYSCALL_RISCV_GETTID: {
            output->verbose(CALL_INFO, 8, 0,"gettid()\n");
            call_ev = new VanadisSyscallGetxEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B,SYSCALL_OP_GETTID);
        } break;

        case VANADIS_SYSCALL_RISCV_GETRANDOM: {
            uint64_t buf    = getRegister( 0 );
            uint64_t buflen = getRegister( 1 );
            uint64_t flags  = getRegister( 2 );

            output->verbose(CALL_INFO, 8, 0, "getrandom( %#" PRIu64 ", %" PRIu64 ", %#" PRIx64 ")\n",
                    buf, buflen, flags);

            call_ev = new VanadisSyscallGetrandomEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, buf, buflen, flags );
        } break;

        case VANADIS_SYSCALL_RISCV_READLINKAT: {
            uint64_t dirfd      = getRegister( 0 );
            uint64_t pathname   = getRegister( 1 );
            uint64_t buf        = getRegister( 2 );
            uint64_t bufsize    = getRegister( 3 );

#ifdef SST_COMPILE_MACOSX
            if (  RISCV_AT_FDCWD == dirfd ) {
                dirfd = AT_FDCWD;
            }
#endif

            output->verbose(CALL_INFO, 8, 0, "readlinkat( %" PRIu64 ", %#" PRIx64 ", %#" PRIx64 ", %#" PRIu64 ")\n",
                    dirfd, pathname, buf, bufsize);

            call_ev = new VanadisSyscallReadLinkAtEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, dirfd, pathname, buf, bufsize);
        } break;
#if 0
        case VANADIS_SYSCALL_RISCV_READLINK: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t readlink_path = regFile->getIntReg<uint64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t readlink_buff_ptr = regFile->getIntReg<uint64_t>(phys_reg_5);

            const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
            int64_t readlink_size = regFile->getIntReg<uint64_t>(phys_reg_6);

            call_ev = new VanadisSyscallReadLinkEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, readlink_path, readlink_buff_ptr, readlink_size);
        } break;
#endif

        case VANADIS_SYSCALL_RISCV_READ: {
            int64_t  read_fd        = getRegister(0);
            uint64_t read_buff_ptr  = getRegister(1);
            int64_t  read_count     = getRegister(2);

            output->verbose(CALL_INFO, 8, 0,
                            "read( %" PRId64 ", 0x%llx, %" PRIu64 " )\n", read_fd, read_buff_ptr, read_count);

            call_ev = new VanadisSyscallReadEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, read_fd, read_buff_ptr, read_count);
        } break;

#if 0
        case VANADIS_SYSCALL_RISCV_ACCESS: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t path_ptr = regFile->getIntReg<uint64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t access_mode = regFile->getIntReg<uint64_t>(phys_reg_5);

            output->verbose(CALL_INFO, 8, 0, "access( 0x%llx, %" PRIu64 " )\n", path_ptr, access_mode);
            call_ev = new VanadisSyscallAccessEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, path_ptr, access_mode);
        } break;
#endif

        case VANADIS_SYSCALL_RISCV_BRK: {
            uint64_t newBrk = getRegister(0);

            output->verbose(CALL_INFO, 8, 0, "brk( value: %" PRIu64 " / 0x%llx )\n", newBrk, newBrk);
            call_ev = new VanadisSyscallBRKEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, newBrk);
        } break;

#if 0
        case VANADIS_SYSCALL_RISCV_SET_THREAD_AREA: {
            const uint64_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t thread_area_ptr = regFile->getIntReg<uint64_t>(phys_reg_4);

            output->verbose(CALL_INFO, 8, 0,
                            "set_thread_area( value: %" PRIu64 " / 0x%llx )\n", thread_area_ptr, thread_area_ptr);

            if (tls_address != nullptr) {
                (*tls_address) = thread_area_ptr;
            }

            call_ev = new VanadisSyscallSetThreadAreaEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, thread_area_ptr);
        } break;
#endif

        case VANADIS_SYSCALL_RISCV_RM_INOTIFY: {
    assert(0);
            output->verbose(CALL_INFO, 8, 0,
                            "to inotify_rm_watch(), by-passing and removing.\n");
            const uint16_t rc_reg = isaTable->getIntPhysReg(VANADIS_SYSCALL_RISCV_RET_REG);
            regFile->setIntReg(rc_reg, (uint64_t)0);

#if 0
            for (int i = 0; i < returnCallbacks.size(); ++i) {
                returnCallbacks[i](hw_thr);
            }
#endif
        } break;

        case VANADIS_SYSCALL_RISCV_UNAME: {
            call_ev = uname();
        } break;

        case VANADIS_SYSCALL_RISCV_FSTAT: {
            int32_t  file_handle = getRegister(0);
            uint64_t fstat_addr  = getRegister(1);

            output->verbose(CALL_INFO, 8, 0, "fstat( %" PRId32 ", %" PRIu64 " )\n", file_handle, fstat_addr);

            call_ev = new VanadisSyscallFstatEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, file_handle, fstat_addr);
        } break;

        case VANADIS_SYSCALL_RISCV_FSTATAT: {
            int32_t  dirfd      = getRegister(0);
            uint64_t pathname   = getRegister(1);
            uint64_t statbuf    = getRegister(2);
            uint64_t flags      = getRegister(3);

#ifdef SST_COMPILE_MACOSX
            if (  RISCV_AT_FDCWD == dirfd ) {
                dirfd = AT_FDCWD;
            }
#endif

            output->verbose(CALL_INFO, 8, 0, "fstat( %d, %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ")\n",dirfd,pathname,statbuf,flags);

            call_ev = new VanadisSyscallFstatAtEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, dirfd, pathname, statbuf, flags );
        } break;

        case VANADIS_SYSCALL_RISCV_UNLINKAT: {
            int64_t dirFd       = getRegister( 0 );
            int64_t path_addr   = getRegister( 1 );
            int64_t flags       = getRegister( 2 );

#ifdef SST_COMPILE_MACOSX
            if (  RISCV_AT_FDCWD == dirFd ) {
                dirFd = AT_FDCWD;                
            } 
#endif

            output->verbose(CALL_INFO, 8, 0, "unlinkat( %" PRIu64 ", %" PRIu64 ", %#" PRIx64" )\n",dirFd,path_addr,flags);

            call_ev = new VanadisSyscallUnlinkatEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, dirFd,path_addr,flags);
        } break;


        case VANADIS_SYSCALL_RISCV_CLOSE: {
            const uint64_t close_file = getRegister( 0 );

            output->verbose(CALL_INFO, 8, 0, "close( %" PRIu64 " )\n", close_file);

            call_ev = new VanadisSyscallCloseEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, close_file);
        } break;

        case VANADIS_SYSCALL_RISCV_OPENAT: {
            uint64_t dirfd      = getRegister( 0 );
            uint64_t path_ptr   = getRegister( 1 );
            uint64_t flags      = getRegister( 2 );
            uint64_t mode       = getRegister( 3 );

#ifdef SST_COMPILE_MACOSX
            if (  RISCV_AT_FDCWD == dirfd ) {
                dirfd = AT_FDCWD;                
            } 
#endif

            output->verbose(CALL_INFO, 8, 0, "openat( %" PRIu64 ", %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ")\n",
                    dirfd, path_ptr, flags, mode);

            call_ev = new VanadisSyscallOpenatEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, dirfd, path_ptr, convertFlags(flags), mode);
        } break;

        case VANADIS_SYSCALL_RISCV_READV: {
            int64_t  readv_fd           = getRegister( 0 );
            uint64_t readv_iovec_ptr    = getRegister( 1 );
            int64_t  readv_iovec_count  = getRegister( 2 );

            output->verbose(CALL_INFO, 8, 0,
                            "readv( %" PRId64 ", 0x%llx, %" PRId64 " )\n", readv_fd, readv_iovec_ptr, readv_iovec_count);
            call_ev = new VanadisSyscallReadvEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, readv_fd, readv_iovec_ptr, readv_iovec_count);
        } break;

        case VANADIS_SYSCALL_RISCV_WRITEV: {
            int64_t  writev_fd          = getRegister( 0 );
            uint64_t writev_iovec_ptr   = getRegister( 1 );
            int64_t  writev_iovec_count = getRegister( 2 );

            output->verbose(CALL_INFO, 8, 0,
                            "writev( %" PRId64 ", 0x%llx, %" PRId64 " )\n", writev_fd, writev_iovec_ptr, writev_iovec_count);
            call_ev = new VanadisSyscallWritevEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, writev_fd, writev_iovec_ptr, writev_iovec_count);
        } break;

        case VANADIS_SYSCALL_RISCV_EXIT: {
            int64_t exit_code = getRegister(0);

            output->verbose(CALL_INFO, 8, 0, "exit( %" PRId64 " )\n", exit_code);
            call_ev = new VanadisSyscallExitEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, exit_code);
            flushLSQ=true;
        } break;

        case VANADIS_SYSCALL_RISCV_EXIT_GROUP: {
            int64_t exit_code = getRegister( 0 );

            output->verbose(CALL_INFO, 8, 0, "exit_group( %" PRId64 " )\n", exit_code);
            call_ev = new VanadisSyscallExitGroupEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, exit_code);
            flushLSQ=true;
        } break;

        case VANADIS_SYSCALL_RISCV_WRITE: {
            int64_t  write_fd       = getRegister( 0 );
            uint64_t write_buff     = getRegister( 1 );
            uint64_t write_count    = getRegister( 2 );

            output->verbose(CALL_INFO, 8, 0,
                            "write( %" PRId64 ", 0x%llx, %" PRIu64 " )\n", write_fd,  write_buff, write_count);
            call_ev = new VanadisSyscallWriteEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, write_fd, write_buff, write_count);
        } break;

        case VANADIS_SYSCALL_RISCV_SET_TID_ADDRESS: {
            uint64_t addr = getRegister( 0 );

            output->verbose(CALL_INFO, 8, 0, "set_tid_address( %#" PRIx64 " )\n", addr);
            call_ev = new VanadisSyscallSetTidAddressEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, addr);

        } break;

        case VANADIS_SYSCALL_RISCV_MADVISE: {
            uint64_t advise_addr    = getRegister( 0 );
            uint64_t advise_len     = getRegister( 1 );
            uint64_t advise_advice  = getRegister( 2 );

            output->verbose(CALL_INFO, 8, 0,
                            "madvise( 0x%llx, %" PRIu64 ", %" PRIu64 " )\n", advise_addr, advise_len, advise_advice);

            printf("Warning: VANADIS_SYSCALL_RISCV_MADVISE not implmented return success\n");
            recvSyscallResp(new VanadisSyscallResponse(0));
        } break;

        case VANADIS_SYSCALL_RISCV_FUTEX: {
            uint64_t addr           = getRegister(0);
            int32_t  op             = getRegister(1);
            uint32_t val            = getRegister(2);
            uint64_t timeout_addr   = getRegister(3);
            uint64_t val2           = getRegister(4);
            uint64_t addr2          = getRegister(5);
            uint64_t val3           = getRegister(6);

            output->verbose(CALL_INFO, 8, 0,
                            "futex( %#" PRIx64 ", %" PRId32 ", %" PRIu64 ", %#" PRIu64 ", %" PRIu32 " %#" PRIu64 " %" PRIu64 " )\n",
                            addr, op, val, timeout_addr, val2, addr2, val3);

            call_ev = new VanadisSyscallFutexEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, addr, op, val, timeout_addr, val2, addr2, val3 );

        } break;

        case VANADIS_SYSCALL_RISCV_IOCTL: {
            int64_t  fd     = getRegister(0);
            uint64_t io_req = getRegister(1);
            uint64_t ptr    = getRegister(2);

            uint64_t access_type = (io_req & 0xE0000000) >> 29;

            bool is_read = (access_type & 0x1) != 0;
            bool is_write = (access_type & 0x2) != 0;

            uint64_t data_size = ((io_req)&0x1FFF0000) >> 16;
            uint64_t io_op = ((io_req)&0xFF);

            uint64_t io_driver = ((io_req)&0xFF00) >> 8;

            output->verbose(CALL_INFO, 8, 0,
                            "ioctl( %" PRId64 ", %" PRIu64 " / 0x%llx, %" PRIu64 " / 0x%llx )\n",
                            fd, io_req, io_req, ptr, ptr);
            output->verbose(CALL_INFO, 9, 0,
                            "-> R: %c W: %c / size: %" PRIu64 " / op: %" PRIu64 " / drv: %" PRIu64 "\n",
                            is_read ? 'y' : 'n', is_write ? 'y' : 'n', data_size, io_op, io_driver);

            call_ev = new VanadisSyscallIoctlEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, fd, is_read, is_write, io_op, io_driver, ptr,
                                                   data_size);
        } break;

        case VANADIS_SYSCALL_RISCV_UNMAP: {
            uint64_t unmap_addr = getRegister( 0 );
            uint64_t unmap_len  = getRegister( 1 );

            output->verbose(CALL_INFO, 8, 0, "unmap( 0x%llx, %" PRIu64 " )\n", unmap_addr, unmap_len);

            if ((0 == unmap_addr)) {
                recvSyscallResp(new VanadisSyscallResponse(-22));
            } else {
                call_ev = new VanadisSyscallMemoryUnMapEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, unmap_addr, unmap_len);
            }
        } break;

        case VANADIS_SYSCALL_RISCV_KILL: {
            uint64_t  pid = getRegister( 0 );
            uint64_t  sig = getRegister( 1 );

            output->verbose(CALL_INFO, 8, 0, "kill( %d, %d  )\n",pid,sig);

            call_ev = new VanadisSyscallKillEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, pid, sig );
        } break;

        case VANADIS_SYSCALL_RISCV_SET_ROBUST_LIST: {
            uint64_t  head  = getRegister( 0 );
            uint64_t  len   = getRegister( 1 );

            output->verbose(CALL_INFO, 8, 0, "set_robust_list(  %#" PRIx64 ", %#" PRIu64" )\n",head,len);

            call_ev = new VanadisSyscallSetRobustListEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, head, len );
        } break;

        case VANADIS_SYSCALL_RISCV_MMAP: {
            uint64_t map_addr   = getRegister( 0 );
            uint64_t map_len    = getRegister( 1 );
            int32_t  map_prot   = getRegister( 2 );
            int32_t  map_flags  = getRegister( 3 );
            int32_t  fd         = getRegister( 4 );
            int64_t  offset     = getRegister( 5 );

            int32_t hostFlags = 0;

            if ( map_flags & MIPS_MAP_FIXED ) {
                output->verbose(CALL_INFO, 8, 0,"mmap() we don't support MAP_FIXED return error EEXIST\n");

                recvSyscallResp(new VanadisSyscallResponse(-EEXIST));
            } else {

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
                            "mmap2( 0x%llx, %" PRIu64 ", %" PRId32 ", %" PRId32 ", %d, %" PRIu64 ")\n",
                            map_addr, map_len, map_prot, map_flags, fd, offset );

                call_ev = new VanadisSyscallMemoryMapEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B,
                        map_addr, map_len, map_prot, hostFlags, fd, offset, 0, 0);
            }
        } break;

        case VANADIS_SYSCALL_RISCV_CLOCK_GETTIME: {
            int64_t  clk_type   = getRegister( 0 );
            uint64_t time_addr  = getRegister( 1 );

            output->verbose(CALL_INFO, 8, 0,
                            "clock_gettime64( %" PRId64 ", 0x%llx )\n", clk_type, time_addr);

            call_ev = new VanadisSyscallGetTime64Event(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, clk_type, time_addr);
        } break;

        case VANADIS_SYSCALL_RISCV_RT_SIGACTION: {
            int64_t signum  = getRegister( 0 );
            int64_t act     = getRegister( 1 );
            int64_t oldact  = getRegister( 2 );

            output->verbose(CALL_INFO, 8, 0, "rt_sigaction( %" PRId64 ", 0#" PRIx64 ", %#" PRIx64 ")\n",signum,act,oldact);

            printf("Warning: VANADIS_SYSCALL_RISCV_RT_SIGACTION not implmented return success\n");
            recvSyscallResp(new VanadisSyscallResponse(0));
        } break;

        case VANADIS_SYSCALL_RISCV_RT_SIGPROCMASK: {
            int32_t  how                = getRegister( 0 );
            uint64_t signal_set_in      = getRegister( 1 );
            uint64_t signal_set_out     = getRegister( 2 );
            int32_t  signal_set_size    = getRegister( 3 );

            output->verbose(CALL_INFO, 8, 0,
                            "rt_sigprocmask( %" PRId32 ", 0x%llx, 0x%llx, %" PRId32 ")\n",
                            how, signal_set_in, signal_set_out, signal_set_size);

            printf("Warning: VANADIS_SYSCALL_RISCV_RT_SETSIGMASK not implmented return success\n");

            recvSyscallResp(new VanadisSyscallResponse(0));
        } break;

        case VANADIS_SYSCALL_RISCV_PRLIMIT: {
            uint64_t pid        = getRegister( 0 );
            uint64_t resource   = getRegister( 1 );
            uint64_t new_limit  = getRegister( 2 );
            uint64_t old_limit  = getRegister( 3 );

            output->verbose(CALL_INFO, 8, 0,
                            "prlimit( %" PRIu64 ", %" PRIu64 ",  %#" PRIx64 ", %#" PRIx64 ")\n", pid, new_limit, old_limit );

            call_ev = new VanadisSyscallPrlimitEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, pid, resource, new_limit, old_limit);
        } break;

#if 0
        case VANADIS_SYSCALL_RISCV_SPLICE: {
            recvSyscallResp(new VanadisSyscallResponse(0));
        } break;
#endif

        default: {
            uint64_t link_reg = getLinkReg();

            output->fatal(CALL_INFO, -1,
                          "Error: unknown code %" PRIu64 " (ins: 0x%llx, link-reg: 0x%llx)\n",
                          os_code, syscallIns->getInstructionAddress(), link_reg);
        } break;
        }

        if (nullptr != call_ev) {
            output->verbose(CALL_INFO, 9, 0, "Sending event to operating system...\n");
            sendSyscallEvent(call_ev);
            return std::make_tuple(false,flushLSQ);
        } else {
            return std::make_tuple(true,flushLSQ);
        }
    }

    void recvSyscallResp( VanadisSyscallResponse* os_resp ) {
        output->verbose(CALL_INFO, 8, 0, "return-code: %" PRId64 " (success: %3s)\n",
                            os_resp->getReturnCode(), os_resp->isSuccessful() ? "yes" : "no");
        output->verbose(CALL_INFO, 9, 0, "issuing call-backs to clear syscall ROB stops...\n");

        // Set up the return code (according to ABI, this goes in r10)
        const uint16_t rc_reg = isaTable->getIntPhysReg(VANADIS_SYSCALL_RISCV_RET_REG);
        const int64_t rc_val = (int64_t)os_resp->getReturnCode();
        regFile->setIntReg(rc_reg, rc_val);

        delete os_resp;
    }

protected:

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
            printf("%s() all flags have not been converted %#x\n",__func__,flags);
            assert( 0 );
        }

        return out;
    }
};

#define RISCV_ARG_REG_ZERO 10
#define RISCV_OS_CODE_REG 17
#define RISCV_LINK_REG 31

class VanadisRISCV64OSHandler : public VanadisRISCV64OSHandler2< uint64_t, VanadisOSBitType::VANADIS_OS_64B, RISCV_ARG_REG_ZERO, RISCV_OS_CODE_REG, RISCV_LINK_REG > {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(VanadisRISCV64OSHandler,
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
