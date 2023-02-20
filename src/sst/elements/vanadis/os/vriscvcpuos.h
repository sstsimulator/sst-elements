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
#define VANADIS_SYSCALL_RISCV_FSTAT 80
#define VANADIS_SYSCALL_RISCV_RT_SIGPROCMASK 135
#define VANADIS_SYSCALL_RISCV_RT_SIGACTION 134
#define VANADIS_SYSCALL_RISCV_MADVISE 233
#define VANADIS_SYSCALL_RISCV_FUTEX 98
#define VANADIS_SYSCALL_RISCV_SET_TID_ADDRESS 96
#define VANADIS_SYSCALL_RISCV_CLONE 220
#define VANADIS_SYSCALL_RISCV_EXIT 93
#define VANADIS_SYSCALL_RISCV_EXIT_GROUP 94
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

#define VANADIS_SYSCALL_RISCV_THREAD_REG 4 
#define VANADIS_SYSCALL_RISCV_RET_REG 10

namespace SST {
namespace Vanadis {

class VanadisRISCV64OSHandler : public VanadisCPUOSHandler {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(VanadisRISCV64OSHandler, "vanadis", "VanadisRISCV64OSHandler",
                                          SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                          "Provides SYSCALL handling for a RISCV-based decoding core",
                                          SST::Vanadis::VanadisCPUOSHandler)


    VanadisRISCV64OSHandler(ComponentId_t id, Params& params) : VanadisCPUOSHandler(id, params) {

    }

    virtual ~VanadisRISCV64OSHandler() {}

    virtual std::tuple<bool,bool> handleSysCall(VanadisSysCallInstruction* syscallIns) {
        uint64_t instPtr = syscallIns->getInstructionAddress();
        const uint16_t call_link_reg = isaTable->getIntPhysReg(31);
        uint64_t call_link_value = regFile->getIntReg<uint64_t>(call_link_reg);
        output->verbose(CALL_INFO, 8, 0, "System Call (syscall-ins: 0x%0llx, link-reg: 0x%llx)\n",
                        syscallIns->getInstructionAddress(), call_link_value);

        bool flushLSQ = false;
        const uint32_t hw_thr = syscallIns->getHWThread();

        // RISCV puts codes in GPR r2
        const uint16_t os_code_phys_reg = isaTable->getIntPhysReg(17);
        const uint64_t os_code = regFile->getIntReg<uint64_t>(os_code_phys_reg);

        output->verbose(CALL_INFO, 8, 0, "--> [SYSCALL-handler] syscall-ins: 0x%0llx / call-code: %" PRIu64 "\n",
                        syscallIns->getInstructionAddress(), os_code);
        VanadisSyscallEvent* call_ev = nullptr;

        switch (os_code) {

        case VANADIS_SYSCALL_RISCV_CLONE: {
            uint64_t flags = getRegister(10);
            uint64_t threadStack = getRegister(11);
            int64_t ptid = getRegister(12);
            int64_t tls = getRegister(13);
            int64_t ctid  = getRegister(14);

            if ( flags == RISCV_SIGCHLD ) {
                call_ev = new VanadisSyscallForkEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B);
            } else { 
                call_ev = new VanadisSyscallCloneEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, instPtr, threadStack, flags, ptid, tls, ctid );
            }
        } break;

        case VANADIS_SYSCALL_RISCV_MPROTECT: {
            uint64_t addr = getRegister(10);
            uint64_t len = getRegister(11);
            int64_t prot = getRegister(12);

            int myProt = 0;
            if ( prot & 0x1 ) {
                myProt |= PROT_READ;
            }
            if ( prot & 0x2 ) {
                myProt |= PROT_WRITE;
            }

            call_ev = new VanadisSyscallMprotectEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, addr, len, myProt );
        } break;

        case VANADIS_SYSCALL_RISCV_GETPID: {
            call_ev = new VanadisSyscallGetxEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B,SYSCALL_OP_GETPID);
        } break;

        case VANADIS_SYSCALL_RISCV_GETPGID: {
            call_ev = new VanadisSyscallGetxEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B,SYSCALL_OP_GETPGID);
        } break;
        case VANADIS_SYSCALL_RISCV_GETPPID: {
            call_ev = new VanadisSyscallGetxEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B,SYSCALL_OP_GETPPID);
        } break;

        case VANADIS_SYSCALL_RISCV_GETTID: {
            call_ev = new VanadisSyscallGetxEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B,SYSCALL_OP_GETTID);
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
            int64_t read_fd = getRegister(10);
            uint64_t read_buff_ptr = getRegister(11);
            int64_t read_count = getRegister(12);

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to read( %" PRId64 ", 0x%llx, %" PRIu64 " )\n", read_fd,
                            read_buff_ptr, read_count);

            call_ev = new VanadisSyscallReadEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, read_fd, read_buff_ptr, read_count);
        } break;

#if 0
        case VANADIS_SYSCALL_RISCV_ACCESS: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t path_ptr = regFile->getIntReg<uint64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t access_mode = regFile->getIntReg<uint64_t>(phys_reg_5);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to access( 0x%llx, %" PRIu64 " )\n",
                            path_ptr, access_mode);
            call_ev = new VanadisSyscallAccessEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, path_ptr, access_mode);
        } break;
#endif

        case VANADIS_SYSCALL_RISCV_BRK: {
            const uint64_t phys_reg_10 = isaTable->getIntPhysReg(10);
            uint64_t newBrk = regFile->getIntReg<uint64_t>(phys_reg_10);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to brk( value: %" PRIu64 " / 0x%llx )\n",
                            newBrk, newBrk);
            call_ev = new VanadisSyscallBRKEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, newBrk);
        } break;

#if 0
        case VANADIS_SYSCALL_RISCV_SET_THREAD_AREA: {
            const uint64_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t thread_area_ptr = regFile->getIntReg<uint64_t>(phys_reg_4);

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to set_thread_area( value: %" PRIu64 " / 0x%llx )\n",
                            thread_area_ptr, thread_area_ptr);

            if (tls_address != nullptr) {
                (*tls_address) = thread_area_ptr;
            }

            call_ev = new VanadisSyscallSetThreadAreaEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, thread_area_ptr);
        } break;
#endif

        case VANADIS_SYSCALL_RISCV_RM_INOTIFY: {
    assert(0);
            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to inotify_rm_watch(), "
                            "by-passing and removing.\n");
            const uint16_t rc_reg = isaTable->getIntPhysReg(VANADIS_SYSCALL_RISCV_RET_REG);
            regFile->setIntReg(rc_reg, (uint64_t)0);

#if 0
            for (int i = 0; i < returnCallbacks.size(); ++i) {
                returnCallbacks[i](hw_thr);
            }
#endif
        } break;

        case VANADIS_SYSCALL_RISCV_UNAME: {
    assert(0);
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t uname_addr = regFile->getIntReg<uint64_t>(phys_reg_4);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to uname()\n");

            call_ev = new VanadisSyscallUnameEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, uname_addr);
        } break;

        case VANADIS_SYSCALL_RISCV_FSTAT: {
            int32_t file_handle = getRegister(10);
            uint64_t fstat_addr = getRegister(11);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to fstat( %" PRId32 ", %" PRIu64 " )\n",
                            file_handle, fstat_addr);

            call_ev = new VanadisSyscallFstatEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, file_handle, fstat_addr);
        } break;

        case VANADIS_SYSCALL_RISCV_UNLINKAT: {
            int64_t dirFd = getRegister( 10 );
            int64_t path_addr = getRegister( 11 );
            int64_t flags = getRegister( 12 );

#ifdef SST_COMPILE_MACOSX
            if (  RISCV_AT_FDCWD == dirFd ) {
                dirFd = AT_FDCWD;                
            } 
#endif

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to unlinkat( %" PRIu64 ", %" PRIu64 ", %#" PRIx64" )\n",dirFd,path_addr,flags);

            call_ev = new VanadisSyscallUnlinkatEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, dirFd,path_addr,flags);
        } break;


        case VANADIS_SYSCALL_RISCV_CLOSE: {
            const uint64_t close_file = getRegister( 10 );

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to close( %" PRIu64 " )\n", close_file);

            call_ev = new VanadisSyscallCloseEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, close_file);
        } break;

        case VANADIS_SYSCALL_RISCV_OPENAT: {
            uint64_t dirfd = getRegister( 10 );
            uint64_t path_ptr = getRegister( 11 );
            uint64_t flags = getRegister( 12 );
            uint64_t mode = getRegister(13);

#ifdef SST_COMPILE_MACOSX
            if (  RISCV_AT_FDCWD == dirfd ) {
                dirfd = AT_FDCWD;                
            } 
#endif

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to openat( %" PRIu64 ", %#" PRIx64 ", %#" PRIx64 ", %#" PRIx64 ")\n",
                    dirfd, path_ptr, flags, mode);

            call_ev = new VanadisSyscallOpenatEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, dirfd, path_ptr, convertFlags(flags), mode);
        } break;

        case VANADIS_SYSCALL_RISCV_READV: {
            int64_t readv_fd = getRegister( 10 );
            uint64_t readv_iovec_ptr = getRegister( 11 );
            int64_t readv_iovec_count = getRegister( 12 );

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to readv( %" PRId64 ", 0x%llx, %" PRId64 " )\n", readv_fd, readv_iovec_ptr, readv_iovec_count);
            call_ev = new VanadisSyscallReadvEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, readv_fd, readv_iovec_ptr, readv_iovec_count);
        } break;

        case VANADIS_SYSCALL_RISCV_WRITEV: {
            int64_t writev_fd = getRegister( 10 );
            uint64_t writev_iovec_ptr = getRegister( 11 );
            int64_t writev_iovec_count = getRegister( 12 );

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to writev( %" PRId64 ", 0x%llx, %" PRId64 " )\n", writev_fd,
                            writev_iovec_ptr, writev_iovec_count);
            call_ev = new VanadisSyscallWritevEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, writev_fd, writev_iovec_ptr, writev_iovec_count);
        } break;

        case VANADIS_SYSCALL_RISCV_EXIT: {
            const uint16_t phys_reg_10 = isaTable->getIntPhysReg(10);
            int64_t exit_code = regFile->getIntReg<int64_t>(phys_reg_10);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to exit( %" PRId64 " )\n",
                            exit_code);
            call_ev = new VanadisSyscallExitEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, exit_code);
            flushLSQ=true;
        } break;

        case VANADIS_SYSCALL_RISCV_EXIT_GROUP: {
            const uint16_t phys_reg_10 = isaTable->getIntPhysReg(10);
            int64_t exit_code = regFile->getIntReg<int64_t>(phys_reg_10);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to exit_group( %" PRId64 " )\n",
                            exit_code);
            call_ev = new VanadisSyscallExitGroupEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, exit_code);
            flushLSQ=true;
        } break;

        case VANADIS_SYSCALL_RISCV_WRITE: {
            int64_t write_fd = getRegister( 10 );
            uint64_t write_buff = getRegister( 11 );
            uint64_t write_count = getRegister( 12 );

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to write( %" PRId64 ", 0x%llx, %" PRIu64 " )\n", write_fd,
                            write_buff, write_count);
            call_ev = new VanadisSyscallWriteEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, write_fd, write_buff, write_count);
        } break;

        case VANADIS_SYSCALL_RISCV_SET_TID_ADDRESS: {
            uint64_t addr = getRegister(10);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] set_tid_address( %#" PRIx64 " )\n", addr);
            call_ev = new VanadisSyscallSetTidAddressEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, addr);

        } break;

        case VANADIS_SYSCALL_RISCV_MADVISE: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t advise_addr = regFile->getIntReg<int64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t advise_len = regFile->getIntReg<int64_t>(phys_reg_5);

            const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
            uint64_t advise_advice = regFile->getIntReg<int64_t>(phys_reg_6);

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found call to madvise( 0x%llx, %" PRIu64 ", %" PRIu64 " )\n",
                            advise_addr, advise_len, advise_advice);

            printf("Warning: VANADIS_SYSCALL_RISCV_MADVISE not implmented return success\n");
            recvSyscallResp(new VanadisSyscallResponse(0));
        } break;

        case VANADIS_SYSCALL_RISCV_FUTEX: {
            uint64_t addr = getRegister(10);
            int32_t op = getRegister(11);
            uint32_t val = getRegister(12);
            uint64_t timeout_addr = getRegister(13);
            uint64_t val2 = getRegister(14);
            uint64_t addr2 = getRegister(15);
            uint64_t val3 = getRegister(16);

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] futex( %#" PRIx64 ", %" PRId32 ", %" PRIu64 ", %" PRIu64 ", %" PRIu32 " %" PRIu64 " %" PRIu64 " )\n",
                            addr, op, val, timeout_addr, val2, addr2, val3);

            call_ev = new VanadisSyscallFutexEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, addr, op, val, timeout_addr, val2, addr2, val3 );

        } break;

        case VANADIS_SYSCALL_RISCV_IOCTL: {
            const uint16_t phys_reg_10 = isaTable->getIntPhysReg(10);
            int64_t fd = regFile->getIntReg<int64_t>(phys_reg_10);

            const uint16_t phys_reg_11 = isaTable->getIntPhysReg(11);
            uint64_t io_req = regFile->getIntReg<uint64_t>(phys_reg_11);

            const uint16_t phys_reg_12 = isaTable->getIntPhysReg(12);
            uint64_t ptr = regFile->getIntReg<uint64_t>(phys_reg_12);

            uint64_t access_type = (io_req & 0xE0000000) >> 29;

            bool is_read = (access_type & 0x1) != 0;
            bool is_write = (access_type & 0x2) != 0;

            uint64_t data_size = ((io_req)&0x1FFF0000) >> 16;
            uint64_t io_op = ((io_req)&0xFF);

            uint64_t io_driver = ((io_req)&0xFF00) >> 8;

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to ioctl( %" PRId64 ", %" PRIu64 " / 0x%llx, %" PRIu64
                            " / 0x%llx )\n",
                            fd, io_req, io_req, ptr, ptr);
            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] -> R: %c W: %c / size: %" PRIu64 " / op: %" PRIu64 " / drv: %" PRIu64
                            "\n",
                            is_read ? 'y' : 'n', is_write ? 'y' : 'n', data_size, io_op, io_driver);

            call_ev = new VanadisSyscallIoctlEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, fd, is_read, is_write, io_op, io_driver, ptr,
                                                   data_size);
        } break;

        case VANADIS_SYSCALL_RISCV_UNMAP: {
            uint64_t unmap_addr = getRegister( 10 );
            uint64_t unmap_len = getRegister( 11 );

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to unmap( 0x%llx, %" PRIu64 " )\n",
                            unmap_addr, unmap_len);

            if ((0 == unmap_addr)) {
                recvSyscallResp(new VanadisSyscallResponse(-22));
            } else {
                call_ev = new VanadisSyscallMemoryUnMapEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, unmap_addr, unmap_len);
            }
        } break;

        case VANADIS_SYSCALL_RISCV_MMAP: {
            uint64_t map_addr = getRegister( 10 );
            uint64_t map_len = getRegister( 11 );
            int32_t map_prot = getRegister( 12 );
            int32_t map_flags = getRegister( 13 );
            int32_t fd = getRegister( 14 );
            int64_t offset = getRegister( 15 );

            int32_t hostFlags = 0;

            if ( map_flags & MIPS_MAP_FIXED ) {
                output->verbose(CALL_INFO, 8, 0,"[syscall-handler] mmap() we don't support MAP_FIXED return error EEXIST\n");

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
                            "[syscall-handler] found a call to mmap2( 0x%llx, %" PRIu64 ", %" PRId32 ", %" PRId32
                            ", %d, %" PRIu64 ")\n",
                            map_addr, map_len, map_prot, map_flags, fd, offset );

                call_ev = new VanadisSyscallMemoryMapEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B,
                        map_addr, map_len, map_prot, hostFlags, fd, offset, 0, 0);
            }
        } break;

        case VANADIS_SYSCALL_RISCV_CLOCK_GETTIME: {
            int64_t clk_type = getRegister( 10 );
            uint64_t time_addr = getRegister( 11 );

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to clock_gettime64( %" PRId64 ", 0x%llx )\n", clk_type,
                            time_addr);

            call_ev = new VanadisSyscallGetTime64Event(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, clk_type, time_addr);
        } break;

        case VANADIS_SYSCALL_RISCV_RT_SIGACTION: {
            printf("Warning: VANADIS_SYSCALL_RISCV_RT_SIGACTION not implmented return success\n");
            recvSyscallResp(new VanadisSyscallResponse(0));
        } break;

        case VANADIS_SYSCALL_RISCV_RT_SIGPROCMASK: {
            int32_t how = getRegister( 10 );
            uint64_t signal_set_in = getRegister( 11 );
            uint64_t signal_set_out = getRegister( 12 );
            int32_t signal_set_size = getRegister( 13 );

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to rt_sigprocmask( %" PRId32 ", 0x%llx, 0x%llx, %" PRId32
                            ")\n",
                            how, signal_set_in, signal_set_out, signal_set_size);

            printf("Warning: VANADIS_SYSCALL_RISCV_RT_SETSIGMASK not implmented return success\n");

            recvSyscallResp(new VanadisSyscallResponse(0));
        } break;

        default: {
            const uint16_t phys_reg_31 = isaTable->getIntPhysReg(31);
            uint64_t link_reg = regFile->getIntReg<int32_t>(phys_reg_31);

            output->fatal(CALL_INFO, -1,
                          "[syscall-handler] Error: unknown code %" PRIu64 " (ins: 0x%llx, link-reg: 0x%llx)\n",
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

    uint64_t getRegister( int reg ) {
        return regFile->getIntReg<uint64_t>( isaTable->getIntPhysReg( reg ) );
    }

    bool brk_zero_memory;
};

} // namespace Vanadis
} // namespace SST

#endif
