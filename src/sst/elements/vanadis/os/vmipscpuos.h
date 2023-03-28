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
#define MIPS_MAP_FIXED   0x010
#define MIPS_MAP_PRIVATE   0x2

#define MIPS_AT_FDCWD -100

#define VANADIS_SYSCALL_MIPS_CLONE 4120
#define VANADIS_SYSCALL_MIPS_MPROTECT 4125
#define VANADIS_SYSCALL_MIPS_EXIT 4001
#define VANADIS_SYSCALL_MIPS_FORK 4002

#define VANADIS_SYSCALL_MIPS_GETTID 4222
#define VANADIS_SYSCALL_MIPS_GETPID 4020
#define VANADIS_SYSCALL_MIPS_GETPGID 4132
#define VANADIS_SYSCALL_MIPS_GETPPID 4064
#define VANADIS_SYSCALL_MIPS_READ 4003
#define VANADIS_SYSCALL_MIPS_OPEN 4005
#define VANADIS_SYSCALL_MIPS_UNLINK 4010
#define VANADIS_SYSCALL_MIPS_UNLINKAT 4294
#define VANADIS_SYSCALL_MIPS_CLOSE 4006
#define VANADIS_SYSCALL_MIPS_WRITE 4004
#define VANADIS_SYSCALL_MIPS_ACCESS 4033
#define VANADIS_SYSCALL_MIPS_BRK 4045
#define VANADIS_SYSCALL_MIPS_IOCTL 4054
#define VANADIS_SYSCALL_MIPS_READLINK 4085
#define VANADIS_SYSCALL_MIPS_MMAP 4090
#define VANADIS_SYSCALL_MIPS_UNMAP 4091
#define VANADIS_SYSCALL_MIPS_UNAME 4122
#define VANADIS_SYSCALL_MIPS_READV 4145
#define VANADIS_SYSCALL_MIPS_WRITEV 4146
#define VANADIS_SYSCALL_MIPS_RT_SIGACTION 4194
#define VANADIS_SYSCALL_MIPS_RT_SETSIGMASK 4195
#define VANADIS_SYSCALL_MIPS_MMAP2 4210

#define VANADIS_SYSCALL_MIPS_STATX 4366
#define VANADIS_SYSCALL_MIPS_FSTAT 4215
#define VANADIS_SYSCALL_MIPS_MADVISE 4218
#define VANADIS_SYSCALL_MIPS_FUTEX 4238
#define VANADIS_SYSCALL_MIPS_SET_TID_ADDRESS 4252
#define VANADIS_SYSCALL_MIPS_EXIT_GROUP 4246
#define VANADIS_SYSCALL_MIPS_SET_THREAD_AREA 4283
#define VANADIS_SYSCALL_MIPS_RM_INOTIFY 4286
#define VANADIS_SYSCALL_MIPS_OPENAT 4288
#define VANADIS_SYSCALL_MIPS_GETTIME64 4403
#define VANADIS_SYSCALL_MIPS_SCHED_GETAFFINITY 4240

namespace SST {
namespace Vanadis {

class VanadisMIPSOSHandler : public VanadisCPUOSHandler {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(VanadisMIPSOSHandler, "vanadis", "VanadisMIPSOSHandler",
                                          SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                          "Provides SYSCALL handling for a MIPS-based decoding core",
                                          SST::Vanadis::VanadisCPUOSHandler)

    VanadisMIPSOSHandler(ComponentId_t id, Params& params) : VanadisCPUOSHandler(id, params) { }

    virtual ~VanadisMIPSOSHandler() {}

    virtual std::tuple<bool,bool> handleSysCall(VanadisSysCallInstruction* syscallIns) {
        uint32_t instPtr = syscallIns->getInstructionAddress();
        uint32_t call_link_value = getLinkReg();

        bool flushLSQ = false;
        const uint32_t hw_thr = syscallIns->getHWThread();
        const uint32_t os_code = getOsCode();

        output->verbose(CALL_INFO, 8, 0, "core=%d hw_thr=%d syscall-ins: 0x%0llx, link-reg: 0x%llx, os_code=%d\n",
                        core_id, hw_thr, syscallIns->getInstructionAddress(), call_link_value, os_code);
        VanadisSyscallEvent* call_ev = nullptr;

        switch (os_code) {
        case VANADIS_SYSCALL_MIPS_CLONE: {
            uint32_t flags      = getRegister(0);
            uint32_t threadStack =  getRegister(1);
            int32_t ptid        = getRegister(2);
            int32_t tls         = getRegister(3);
            uint32_t stackPtr   = getSP();

            assert( stackPtr );

            output->verbose(CALL_INFO, 8, 0,
                "clone( %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 " )\n",
                flags, threadStack, ptid, tls, stackPtr );

            call_ev = new VanadisSyscallCloneEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, instPtr, threadStack, flags, ptid, tls, 0, stackPtr );
        } break;

       case VANADIS_SYSCALL_MIPS_SCHED_GETAFFINITY: {
            uint32_t pid        = getRegister(0);
            uint32_t cpusetsize = getRegister(1);
            int32_t  maskAddr   = getRegister(2);

            output->verbose(CALL_INFO, 8, 0,
                            "sched_getaffinity( %" PRId64 ", %" PRId64", %#" PRIx64 " )\n", pid, cpusetsize, maskAddr );

            call_ev = new VanadisSyscallGetaffinityEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, pid, cpusetsize, maskAddr );

        } break;

        case VANADIS_SYSCALL_MIPS_MPROTECT: {
            uint32_t addr   = getRegister(0);
            uint32_t len    = getRegister(1);
            int32_t prot    = getRegister(2);

            int myProt = 0;
            if ( prot & 0x1 ) {
                myProt |= PROT_READ;
            }
            if ( prot & 0x2 ) {
                myProt |= PROT_WRITE;
            }

            call_ev = new VanadisSyscallMprotectEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, addr, len, myProt );
        } break;

        case VANADIS_SYSCALL_MIPS_FORK: {
            output->verbose(CALL_INFO, 8, 0, "fork()\n");
            call_ev = new VanadisSyscallForkEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B);
        } break;

        case VANADIS_SYSCALL_MIPS_GETPID: {
            output->verbose(CALL_INFO, 8, 0, "getpid()\n");
            call_ev = new VanadisSyscallGetxEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B,SYSCALL_OP_GETPID);
        } break;

        case VANADIS_SYSCALL_MIPS_GETPGID: {
            output->verbose(CALL_INFO, 8, 0, "getpgid()\n");
            call_ev = new VanadisSyscallGetxEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B,SYSCALL_OP_GETPGID);
        } break;
        case VANADIS_SYSCALL_MIPS_GETPPID: {
            output->verbose(CALL_INFO, 8, 0, "getppid()\n");
            call_ev = new VanadisSyscallGetxEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B,SYSCALL_OP_GETPPID);
        } break;

        case VANADIS_SYSCALL_MIPS_GETTID: {
            output->verbose(CALL_INFO, 8, 0, "gettid()\n");
            call_ev = new VanadisSyscallGetxEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B,SYSCALL_OP_GETTID);
        } break;

        case VANADIS_SYSCALL_MIPS_READLINK: {
            uint32_t path       = getRegister(0);
            uint32_t buf_ptr    = getRegister(1);
            int32_t size        = getRegister(2);

            output->verbose(CALL_INFO, 8, 0, "readLink( %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ")\n",path,buf_ptr,size);
            call_ev = new VanadisSyscallReadLinkEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, path, buf_ptr, size);
        } break;

        case VANADIS_SYSCALL_MIPS_READ: {
            int32_t fd          = getRegister(0);
            uint32_t buff_ptr   = getRegister(1);
            int32_t count       = getRegister(2);

            output->verbose(CALL_INFO, 8, 0, "read( %#" PRId64 ", %#" PRIx64 ", %#" PRIx64 ")\n", fd, buff_ptr, count );
            call_ev = new VanadisSyscallReadEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, fd, buff_ptr, count);
        } break;

        case VANADIS_SYSCALL_MIPS_ACCESS: {
            uint32_t path_ptr       = getRegister(0);
            uint32_t access_mode    = getRegister(1);

            output->verbose(CALL_INFO, 8, 0, "access( 0x%llx, %" PRIu64 " )\n", path_ptr, access_mode);
            call_ev = new VanadisSyscallAccessEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, path_ptr, access_mode);
        } break;

        case VANADIS_SYSCALL_MIPS_BRK: {
            uint32_t newBrk = getRegister(0);

            output->verbose(CALL_INFO, 8, 0, "brk( value: %" PRIu64 " )\n", newBrk);
            call_ev = new VanadisSyscallBRKEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, newBrk );
        } break;

        case VANADIS_SYSCALL_MIPS_SET_THREAD_AREA: {
            uint32_t ptr = getRegister(0);

            output->verbose(CALL_INFO, 8, 0, "set_thread_area( %#" PRIx64 ")\n", ptr);

            if (tls_address != nullptr) {
                (*tls_address) = ptr;
            } else {
                assert(0);
            }
            recvSyscallResp(new VanadisSyscallResponse(0));

//            call_ev = new VanadisSyscallSetThreadAreaEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, thread_area_ptr);
        } break;

        case VANADIS_SYSCALL_MIPS_RM_INOTIFY: {
            output->verbose(CALL_INFO, 8, 0, "inotify_rm_watch(), by-passing and removing.\n");
            const uint16_t rc_reg = isaTable->getIntPhysReg(7);
            regFile->setIntReg(rc_reg, (uint32_t)0);

            writeSyscallResult(true);
        } break;

        case VANADIS_SYSCALL_MIPS_UNAME: {
            uint32_t addr = getRegister(0);

            output->verbose(CALL_INFO, 8, 0, "uname( %#" PRIx64 ")\n",addr);

            call_ev = new VanadisSyscallUnameEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, addr);
        } break;

        case VANADIS_SYSCALL_MIPS_STATX: {
            int32_t  dirfd      = getRegister(0);
            uint32_t pathname   = getRegister(1);
            int32_t  flags      = getRegister(2);
            uint32_t mask       = getRegister(3);
            uint32_t stackPtr   = getSP();

#ifdef SST_COMPILE_MACOSX
            if (  MIPS_AT_FDCWD == dirfd ) {
                dirfd = AT_FDCWD;
            }
#endif
            output->verbose(CALL_INFO, 8, 0, "statx( %" PRId32 ", %#" PRIx64 ", %#" PRIx32 ", %#" PRIx32 ", %#" PRIx64 " )\n",
                            dirfd, pathname, flags, mask, stackPtr );

            call_ev = new VanadisSyscallStatxEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, dirfd, pathname, flags, mask, stackPtr);
         } break;

        case VANADIS_SYSCALL_MIPS_FSTAT: {
            int32_t  file_handle    = getRegister(0);
            uint32_t addr           = getRegister(1);

            output->verbose(CALL_INFO, 8, 0, "fstat( %" PRId32 ", %" PRIu64 " )\n", file_handle, addr);

            call_ev = new VanadisSyscallFstatEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, file_handle, addr);
        } break;

        case VANADIS_SYSCALL_MIPS_UNLINK: {
            int32_t path_addr = getRegister( 0 );

            output->verbose(CALL_INFO, 8, 0, "unlink( %" PRId32 " )\n",path_addr);

            call_ev = new VanadisSyscallUnlinkEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, path_addr);
        } break;

        case VANADIS_SYSCALL_MIPS_UNLINKAT: {
            int32_t dirFd       = getRegister( 0 );
            int32_t path_addr   = getRegister( 1 );
            int32_t flags       = getRegister( 2 );

#ifdef SST_COMPILE_MACOSX
            if (  MIPS_AT_FDCWD == dirFd ) {
                dirFd = AT_FDCWD;
            }
#endif
            output->verbose(CALL_INFO, 8, 0, "unlinkat( %d, %" PRId32 ", %#" PRIx32" )\n",dirFd,path_addr,flags);

            call_ev = new VanadisSyscallUnlinkatEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, dirFd,path_addr,flags);
        } break;

        case VANADIS_SYSCALL_MIPS_CLOSE: {
            uint32_t file = getRegister(0);

            output->verbose(CALL_INFO, 8, 0, "close( %" PRIu32 " )\n", file);

            call_ev = new VanadisSyscallCloseEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, file);
        } break;

        case VANADIS_SYSCALL_MIPS_OPEN: {
            uint32_t path_ptr   = getRegister(0);
            uint32_t flags      = getRegister(1);
            uint32_t mode       = getRegister(2);

            output->verbose(CALL_INFO, 8, 0, "open( %#" PRIx64 ", %" PRIu64 ", %" PRIu64 " )\n", path_ptr, flags, mode);

            call_ev = new VanadisSyscallOpenEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, path_ptr, convertFlags(flags), mode);
        } break;

        case VANADIS_SYSCALL_MIPS_OPENAT: {
            uint32_t dirfd      = getRegister(0);
            uint32_t path_ptr   = getRegister(1);
            uint32_t flags      = getRegister(2);
            uint32_t mode       = getRegister(3);

#ifdef SST_COMPILE_MACOSX
            if (  MIPS_AT_FDCWD == dirfd ) {
                dirfd = AT_FDCWD;
            }
#endif
            output->verbose(CALL_INFO, 8, 0, "openat( %" PRIu64 ", %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ")\n", dirfd, path_ptr, flags, mode );
            call_ev = new VanadisSyscallOpenatEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, dirfd, path_ptr, convertFlags(flags), mode);
        } break;

        case VANADIS_SYSCALL_MIPS_READV: {
            int32_t  fd             = getRegister(0);
            uint32_t iovec_ptr      = getRegister(1);
            int32_t  iovec_count    = getRegister(2);

            output->verbose(CALL_INFO, 8, 0, "readv( %" PRId64 ", 0x%llx, %" PRId64 " )\n", fd, iovec_ptr, iovec_count);
            call_ev = new VanadisSyscallReadvEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, fd, iovec_ptr, iovec_count);
        } break;


        case VANADIS_SYSCALL_MIPS_WRITEV: {
            int32_t  fd             = getRegister(0);
            uint32_t iovec_ptr      = getRegister(1);
            int32_t  iovec_count    = getRegister(2);

            output->verbose(CALL_INFO, 8, 0, "writev( %" PRId64 ", 0x%llx, %" PRId64 " )\n", fd, iovec_ptr, iovec_count);
            call_ev = new VanadisSyscallWritevEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, fd, iovec_ptr, iovec_count);
        } break;

        case VANADIS_SYSCALL_MIPS_EXIT:{
            int32_t code = getRegister(0);

            output->verbose(CALL_INFO, 8, 0, "exit( %" PRId64 " )\n", code);
            call_ev = new VanadisSyscallExitEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, code);
            flushLSQ = true;
        } break;

        case VANADIS_SYSCALL_MIPS_EXIT_GROUP: {
            int32_t code = getRegister(0);

            output->verbose(CALL_INFO, 8, 0, "exit_group( %" PRId64 " )\n", code);
            call_ev = new VanadisSyscallExitGroupEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, code);
            flushLSQ = true;
        } break;

        case VANADIS_SYSCALL_MIPS_WRITE: {
            int32_t  fd     = getRegister(0);
            uint32_t buff   = getRegister(1);
            uint32_t count  = getRegister(2);

            output->verbose(CALL_INFO, 8, 0, "write( %" PRId64 ", 0x%llx, %" PRIu64 " )\n", fd, buff, count);
            call_ev = new VanadisSyscallWriteEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, fd, buff, count);
        } break;

        case VANADIS_SYSCALL_MIPS_SET_TID_ADDRESS: {
            int32_t addr = getRegister(0);

            output->verbose(CALL_INFO, 8, 0, "set_tid_address( %#" PRIx64 " )\n", addr);
            call_ev = new VanadisSyscallSetTidAddressEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, addr);
        } break;

        case VANADIS_SYSCALL_MIPS_MADVISE: {
            uint32_t addr   = getRegister(0);
            uint32_t len    = getRegister(1);
            uint32_t advice = getRegister(2);

            output->verbose(CALL_INFO, 8, 0, "madvise( 0x%llx, %" PRIu64 ", %" PRIu64 " )\n", addr, len, advice);

            // output->fatal(CALL_INFO, -1, "STOP\n");
            recvSyscallResp(new VanadisSyscallResponse(0));
        } break;

        case VANADIS_SYSCALL_MIPS_FUTEX: {
            uint32_t addr           = getRegister(0);
            int32_t  op             = getRegister(1);
            uint32_t val            = getRegister(2);
            uint32_t timeout_addr   = getRegister(3);
            uint32_t stack_ptr      = getSP();

            output->verbose(CALL_INFO, 8, 0, "futex( 0x%llx, %" PRId32 ", %" PRIu32 ", %" PRIu64 ", sp: 0x%llx (arg-count is greater than 4))\n",
                            addr, op, val, timeout_addr, stack_ptr);

            call_ev = new VanadisSyscallFutexEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, addr, op, val, timeout_addr, stack_ptr );
        } break;

        case VANADIS_SYSCALL_MIPS_IOCTL: {
            int32_t  fd     = getRegister(0);
            uint32_t io_req = getRegister(1);
            uint32_t ptr    = getRegister(2);

            uint32_t access_type = (io_req & 0xE0000000) >> 29;

            bool is_read = (access_type & 0x1) != 0;
            bool is_write = (access_type & 0x2) != 0;

            uint32_t data_size = ((io_req)&0x1FFF0000) >> 16;
            uint32_t io_op = ((io_req)&0xFF);

            uint32_t io_driver = ((io_req)&0xFF00) >> 8;

            output->verbose(CALL_INFO, 8, 0,
                            "ioctl( %" PRId64 ", %" PRIu64 " / 0x%llx, %" PRIu64 " / 0x%llx )\n", fd, io_req, io_req, ptr, ptr);
            output->verbose(CALL_INFO, 8, 0,
                            "ioctl -> R: %c W: %c / size: %" PRIu64 " / op: %" PRIu64 " / drv: %" PRIu64 "\n",
                            is_read ? 'y' : 'n', is_write ? 'y' : 'n', data_size, io_op, io_driver);

            call_ev = new VanadisSyscallIoctlEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, fd, is_read, is_write, io_op, io_driver, ptr,
                                                   data_size);
        } break;

        case VANADIS_SYSCALL_MIPS_UNMAP: {
            uint32_t addr   = getRegister(0);
            uint32_t len    = getRegister(1);

            output->verbose(CALL_INFO, 8, 0, "unmap( 0x%llx, %" PRIu64 " )\n", addr, len);

            if ((0 == addr)) {
                recvSyscallResp(new VanadisSyscallResponse(-22));
            } else {
                call_ev = new VanadisSyscallMemoryUnMapEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, addr, len);
            }
        } break;

        case VANADIS_SYSCALL_MIPS_MMAP: {
            assert(0);
        } break;

        case VANADIS_SYSCALL_MIPS_MMAP2: {
            uint32_t addr       = getRegister(0);
            uint32_t len        = getRegister(1);
            int32_t  prot       = getRegister(2);
            int32_t  flags      = getRegister(3);
            uint32_t stack_ptr  = getSP();

            output->verbose(CALL_INFO, 8, 0, "mmap2( 0x%llx, %" PRIu64 ", %" PRId32 ", %" PRId32 ", sp: 0x%llx (> 4 arguments) )\n",
                            addr, len, prot, flags, stack_ptr);

            if ( flags & MIPS_MAP_FIXED ) {
                output->verbose(CALL_INFO, 8, 0,"mmap2() we don't support MAP_FIXED return error EEXIST\n");

                recvSyscallResp(new VanadisSyscallResponse(-EEXIST));
            } else {

                call_ev = new VanadisSyscallMemoryMapEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B,
                    addr, len, prot, mmapConvertFlags(flags), /*fd*/ 0, /*offset*/ 0, stack_ptr, 4096);
            }
        } break;

        case VANADIS_SYSCALL_MIPS_GETTIME64: {
            int32_t  clk_type   = getRegister(0);
            uint32_t time_addr  = getRegister(1);

            output->verbose(CALL_INFO, 8, 0, "clock_gettime64( %" PRId64 ", 0x%llx )\n", clk_type, time_addr);

            call_ev = new VanadisSyscallGetTime64Event(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, clk_type, time_addr);
        } break;

        case VANADIS_SYSCALL_MIPS_RT_SIGACTION: {
             printf("Warning: VANADIS_SYSCALL_MIPS_RT_SIGACTION not implmented return success\n");
             recvSyscallResp(new VanadisSyscallResponse(0));
        } break;

        case VANADIS_SYSCALL_MIPS_RT_SETSIGMASK: {
            int32_t  how        = getRegister(0);
            uint32_t set_in     = getRegister(1);
            uint32_t set_out    = getRegister(2);
            int32_t  set_size   = getRegister(3);

            output->verbose(CALL_INFO, 8, 0, "rt_sigprocmask( %" PRId32 ", 0x%llx, 0x%llx, %" PRId32 ")\n", how, set_in, set_out, set_size);

            recvSyscallResp(new VanadisSyscallResponse(0));
        } break;

        default: {
            uint32_t link_reg = getLinkReg();

            output->fatal(CALL_INFO, -1, "Error: unknown code %" PRIu64 " (ins: 0x%llx, link-reg: 0x%llx)\n",
                          os_code, syscallIns->getInstructionAddress(), link_reg);
        } break;
        }

        if (nullptr != call_ev) {
            output->verbose(CALL_INFO, 8, 0, "Sending event to operating system...\n");
            sendSyscallEvent(call_ev);
            return std::make_tuple(false,flushLSQ);
        } else {
            return std::make_tuple(true,flushLSQ);
        }
    }

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

protected:
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
    uint32_t getOsCode() {
        // MIPS puts codes in GPR r2
        return regFile->getIntReg<uint32_t>( isaTable->getIntPhysReg(2) );
    }
    uint32_t getLinkReg() {
        return regFile->getIntReg<uint32_t>( isaTable->getIntPhysReg(31) );
    }
    uint32_t getSP( ) {
        return regFile->getIntReg<uint32_t>( isaTable->getIntPhysReg( 29 ) );
    }
    uint32_t getRegister( int reg ) {
        return regFile->getIntReg<uint32_t>( isaTable->getIntPhysReg( reg + 4 ) );
    }

    bool brk_zero_memory;
};

} // namespace Vanadis
} // namespace SST

#endif
