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
#include "os/vstartthreadreq.h"
#include "os/vcpuos.h"
#include "os/voscallev.h"
#include <functional>

#include <fcntl.h>
#include <sys/mman.h>

#define MIPS_CONVERT( x ) \
    if ( flags & MIPS_O_##x ) {\
        flags &= ~MIPS_O_##x;\
        out |= O_##x;\
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
#define MIPS_O_LARGEFILE 0x2000

#ifndef SST_COMPILE_MACOSX
#define MIPS_O_DIRECT    0x8000
#define MIPS_O_NOATIME   0x40000
#define MIPS_O_PATH      0x200000
#define MIPS_O_TMPFILE   0x410000
#endif

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
#define VANADIS_SYSCALL_MIPS_UNMAP 4091
#define VANADIS_SYSCALL_MIPS_UNAME 4122
#define VANADIS_SYSCALL_MIPS_READV 4145
#define VANADIS_SYSCALL_MIPS_WRITEV 4146
#define VANADIS_SYSCALL_MIPS_RT_SETSIGMASK 4195
#define VANADIS_SYSCALL_MIPS_MMAP2 4210
#define VANADIS_SYSCALL_MIPS_FSTAT 4215
#define VANADIS_SYSCALL_MIPS_MADVISE 4218
#define VANADIS_SYSCALL_MIPS_FUTEX 4238
#define VANADIS_SYSCALL_MIPS_SET_TID 4252
#define VANADIS_SYSCALL_MIPS_EXIT_GROUP 4246
#define VANADIS_SYSCALL_MIPS_SET_THREAD_AREA 4283
#define VANADIS_SYSCALL_MIPS_RM_INOTIFY 4286
#define VANADIS_SYSCALL_MIPS_OPENAT 4288
#define VANADIS_SYSCALL_MIPS_GETTIME64 4403

namespace SST {
namespace Vanadis {

class VanadisMIPSOSHandler : public VanadisCPUOSHandler {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(VanadisMIPSOSHandler, "vanadis", "VanadisMIPSOSHandler",
                                          SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                          "Provides SYSCALL handling for a MIPS-based decoding core",
                                          SST::Vanadis::VanadisCPUOSHandler)

    SST_ELI_DOCUMENT_PARAMS({ "brk_zero_memory", "Zero memory during OS calls to brk", "0" })

    VanadisMIPSOSHandler(ComponentId_t id, Params& params) : VanadisCPUOSHandler(id, params) {

        brk_zero_memory = params.find<bool>("brk_zero_memory", false);
    }

    virtual ~VanadisMIPSOSHandler() {}

    virtual bool handleSysCall(VanadisSysCallInstruction* syscallIns) {
        const uint16_t call_link_reg = isaTable->getIntPhysReg(31);
        uint64_t call_link_value = regFile->getIntReg<uint64_t>(call_link_reg);
        output->verbose(CALL_INFO, 8, 0, "System Call (syscall-ins: 0x%0llx, link-reg: 0x%llx)\n",
                        syscallIns->getInstructionAddress(), call_link_value);

        const uint32_t hw_thr = syscallIns->getHWThread();

        // MIPS puts codes in GPR r2
        const uint16_t os_code_phys_reg = isaTable->getIntPhysReg(2);
        const uint64_t os_code = regFile->getIntReg<uint64_t>(os_code_phys_reg);

        output->verbose(CALL_INFO, 8, 0, "--> [SYSCALL-handler] syscall-ins: 0x%0llx / call-code: %" PRIu64 "\n",
                        syscallIns->getInstructionAddress(), os_code);
        VanadisSyscallEvent* call_ev = nullptr;

        switch (os_code) {
        case VANADIS_SYSCALL_MIPS_READLINK: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t readlink_path = regFile->getIntReg<uint64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t readlink_buff_ptr = regFile->getIntReg<uint64_t>(phys_reg_5);

            const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
            int64_t readlink_size = regFile->getIntReg<uint64_t>(phys_reg_6);

            call_ev = new VanadisSyscallReadLinkEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, readlink_path, readlink_buff_ptr, readlink_size);
        } break;

        case VANADIS_SYSCALL_MIPS_READ: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            int64_t read_fd = regFile->getIntReg<int64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t read_buff_ptr = regFile->getIntReg<uint64_t>(phys_reg_5);

            const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
            int64_t read_count = regFile->getIntReg<int64_t>(phys_reg_6);

            call_ev = new VanadisSyscallReadEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, read_fd, read_buff_ptr, read_count);
        } break;

        case VANADIS_SYSCALL_MIPS_ACCESS: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t path_ptr = regFile->getIntReg<uint64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t access_mode = regFile->getIntReg<uint64_t>(phys_reg_5);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to access( 0x%llx, %" PRIu64 " )\n",
                            path_ptr, access_mode);
            call_ev = new VanadisSyscallAccessEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, path_ptr, access_mode);
        } break;

        case VANADIS_SYSCALL_MIPS_BRK: {
            const uint64_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t newBrk = regFile->getIntReg<uint64_t>(phys_reg_4);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to brk( value: %" PRIu64 " ), zero: %s\n",
                            newBrk, brk_zero_memory ? "yes" : "no");
            call_ev = new VanadisSyscallBRKEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, newBrk, brk_zero_memory);
        } break;

        case VANADIS_SYSCALL_MIPS_SET_THREAD_AREA: {
            const uint64_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t thread_area_ptr = regFile->getIntReg<uint64_t>(phys_reg_4);

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to set_thread_area( value: %" PRIu64 " / 0x%llx )\n",
                            thread_area_ptr, thread_area_ptr);

            if (tls_address != nullptr) {
                (*tls_address) = thread_area_ptr;
            }

            call_ev = new VanadisSyscallSetThreadAreaEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, thread_area_ptr);
        } break;

        case VANADIS_SYSCALL_MIPS_RM_INOTIFY: {
            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to inotify_rm_watch(), "
                            "by-passing and removing.\n");
            const uint16_t rc_reg = isaTable->getIntPhysReg(7);
            regFile->setIntReg(rc_reg, (uint64_t)0);

            writeSyscallResult(true);

#if 0
            for (int i = 0; i < returnCallbacks.size(); ++i) {
                returnCallbacks[i](hw_thr);
            }
#endif
        } break;

        case VANADIS_SYSCALL_MIPS_UNAME: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t uname_addr = regFile->getIntReg<uint64_t>(phys_reg_4);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to uname()\n");

            call_ev = new VanadisSyscallUnameEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, uname_addr);
        } break;

        case VANADIS_SYSCALL_MIPS_FSTAT: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            int32_t file_handle = regFile->getIntReg<int32_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t fstat_addr = regFile->getIntReg<uint64_t>(phys_reg_5);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to fstat( %" PRId32 ", %" PRIu64 " )\n",
                            file_handle, fstat_addr);

            call_ev = new VanadisSyscallFstatEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, file_handle, fstat_addr);
        } break;

        case VANADIS_SYSCALL_MIPS_UNLINK: {
            int32_t path_addr = getRegister( 4 );

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to unlink( %" PRId32 " )\n",path_addr);

            call_ev = new VanadisSyscallUnlinkEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, path_addr);
        } break;

        case VANADIS_SYSCALL_MIPS_UNLINKAT: {
            int32_t dirFd = getRegister( 4 );
            int32_t path_addr = getRegister( 5 );
            int32_t flags = getRegister( 6 );

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to unlinkat( %d, %" PRId32 ", %#" PRIx32" )\n",dirFd,path_addr,flags);

            call_ev = new VanadisSyscallUnlinkatEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, dirFd,path_addr,flags);
        } break;

        case VANADIS_SYSCALL_MIPS_CLOSE: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint32_t close_file = regFile->getIntReg<uint32_t>(phys_reg_4);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to close( %" PRIu32 " )\n", close_file);

            call_ev = new VanadisSyscallCloseEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, close_file);
        } break;

        case VANADIS_SYSCALL_MIPS_OPEN: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t open_path_ptr = regFile->getIntReg<uint64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t open_flags = regFile->getIntReg<uint64_t>(phys_reg_5);

            const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
            uint64_t open_mode = regFile->getIntReg<uint64_t>(phys_reg_6);

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to open( 0x%llx, %" PRIu64 ", %" PRIu64 " )\n",
                            open_path_ptr, open_flags, open_mode);

            call_ev = new VanadisSyscallOpenEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, open_path_ptr, convertFlags(open_flags), open_mode);
        } break;

        case VANADIS_SYSCALL_MIPS_OPENAT: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t openat_dirfd = regFile->getIntReg<uint64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t openat_path_ptr = regFile->getIntReg<uint64_t>(phys_reg_5);

            const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
            uint64_t openat_flags = regFile->getIntReg<uint64_t>(phys_reg_6);

            const uint16_t phys_reg_7 = isaTable->getIntPhysReg(7);
            uint64_t openat_mode = regFile->getIntReg<uint64_t>(phys_reg_7);

#ifdef SST_COMPILE_MACOSX
            if ( openat_dirfd == -100 ) {
                openat_dirfd = -2;
            }
#endif

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to openat()\n");
            call_ev = new VanadisSyscallOpenAtEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, openat_dirfd, openat_path_ptr, convertFlags(openat_flags), openat_mode);
        } break;

        case VANADIS_SYSCALL_MIPS_READV: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            int64_t readv_fd = regFile->getIntReg<int64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t readv_iovec_ptr = regFile->getIntReg<uint64_t>(phys_reg_5);

            const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
            int64_t readv_iovec_count = regFile->getIntReg<int64_t>(phys_reg_6);

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to readv( %" PRId64 ", 0x%llx, %" PRId64 " )\n", readv_fd,
                            readv_iovec_ptr, readv_iovec_count);
            call_ev = new VanadisSyscallReadvEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, readv_fd, readv_iovec_ptr, readv_iovec_count);
        } break;


        case VANADIS_SYSCALL_MIPS_WRITEV: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            int64_t writev_fd = regFile->getIntReg<int64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t writev_iovec_ptr = regFile->getIntReg<uint64_t>(phys_reg_5);

            const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
            int64_t writev_iovec_count = regFile->getIntReg<int64_t>(phys_reg_6);

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to writev( %" PRId64 ", 0x%llx, %" PRId64 " )\n", writev_fd,
                            writev_iovec_ptr, writev_iovec_count);
            call_ev = new VanadisSyscallWritevEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, writev_fd, writev_iovec_ptr, writev_iovec_count);
        } break;

        case VANADIS_SYSCALL_MIPS_EXIT_GROUP: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            int64_t exit_code = regFile->getIntReg<int64_t>(phys_reg_4);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to exit_group( %" PRId64 " )\n",
                            exit_code);
            call_ev = new VanadisSyscallExitGroupEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, exit_code);
        } break;

        case VANADIS_SYSCALL_MIPS_WRITE: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            int64_t write_fd = regFile->getIntReg<int64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t write_buff = regFile->getIntReg<uint64_t>(phys_reg_5);

            const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
            uint64_t write_count = regFile->getIntReg<int64_t>(phys_reg_6);

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to write( %" PRId64 ", 0x%llx, %" PRIu64 " )\n", write_fd,
                            write_buff, write_count);
            call_ev = new VanadisSyscallWriteEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, write_fd, write_buff, write_count);
        } break;

        case VANADIS_SYSCALL_MIPS_SET_TID: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            int64_t new_tid = regFile->getIntReg<int64_t>(phys_reg_4);

            setThreadID(new_tid);
            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found call to set_tid( %" PRId64 " )\n", new_tid);

            recvSyscallResp(new VanadisSyscallResponse(new_tid));
        } break;

        case VANADIS_SYSCALL_MIPS_MADVISE: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t advise_addr = regFile->getIntReg<int64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t advise_len = regFile->getIntReg<int64_t>(phys_reg_5);

            const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
            uint64_t advise_advice = regFile->getIntReg<int64_t>(phys_reg_6);

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found call to madvise( 0x%llx, %" PRIu64 ", %" PRIu64 " )\n",
                            advise_addr, advise_len, advise_advice);

            // output->fatal(CALL_INFO, -1, "STOP\n");
            recvSyscallResp(new VanadisSyscallResponse(0));
        } break;

        case VANADIS_SYSCALL_MIPS_FUTEX: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t futex_addr = regFile->getIntReg<uint64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            int32_t futex_op = regFile->getIntReg<uint64_t>(phys_reg_5);

            const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
            uint32_t futex_val = regFile->getIntReg<int32_t>(phys_reg_6);

            const uint16_t phys_reg_7 = isaTable->getIntPhysReg(7);
            uint64_t futex_timeout_addr = regFile->getIntReg<int32_t>(phys_reg_7);

            const uint16_t phys_reg_sp = isaTable->getIntPhysReg(29);
            uint64_t stack_ptr = regFile->getIntReg<uint64_t>(phys_reg_sp);

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to futex( 0x%llx, %" PRId32 ", %" PRIu32 ", %" PRIu64
                            ", sp: 0x%llx (arg-count is greater than 4))\n",
                            futex_addr, futex_op, futex_val, futex_timeout_addr, stack_ptr);

            recvSyscallResp(new VanadisSyscallResponse(0));
        } break;

        case VANADIS_SYSCALL_MIPS_IOCTL: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            int64_t fd = regFile->getIntReg<int64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t io_req = regFile->getIntReg<uint64_t>(phys_reg_5);

            const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
            uint64_t ptr = regFile->getIntReg<uint64_t>(phys_reg_6);

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

            call_ev = new VanadisSyscallIOCtlEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, fd, is_read, is_write, io_op, io_driver, ptr,
                                                   data_size);
        } break;

        case VANADIS_SYSCALL_MIPS_UNMAP: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t unmap_addr = regFile->getIntReg<uint64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t unmap_len = regFile->getIntReg<uint64_t>(phys_reg_5);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to unmap( 0x%llx, %" PRIu64 " )\n",
                            unmap_addr, unmap_len);

            if ((0 == unmap_addr)) {
                recvSyscallResp(new VanadisSyscallResponse(-22));
            } else {
                call_ev = new VanadisSyscallMemoryUnMapEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, unmap_addr, unmap_len);
            }
        } break;

        case VANADIS_SYSCALL_MIPS_MMAP2: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t map_addr = regFile->getIntReg<uint64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t map_len = regFile->getIntReg<uint64_t>(phys_reg_5);

            const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
            int32_t map_prot = regFile->getIntReg<int32_t>(phys_reg_6);

            const uint16_t phys_reg_7 = isaTable->getIntPhysReg(7);
            int32_t map_flags = regFile->getIntReg<int32_t>(phys_reg_7);

            const uint16_t phys_reg_sp = isaTable->getIntPhysReg(29);
            uint64_t stack_ptr = regFile->getIntReg<uint64_t>(phys_reg_sp);

            int sst_flags = 0;
            if ( map_flags & 0x2 ) sst_flags |= MAP_PRIVATE;
            if ( map_flags & 0x10 ) sst_flags |= MAP_FIXED;
            if ( map_flags & 0x800 ) sst_flags |= MAP_ANON;

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to mmap2( 0x%llx, %" PRIu64 ", %" PRId32 ", %" PRId32
                            ", sp: 0x%llx (> 4 arguments) )\n",
                            map_addr, map_len, map_prot, map_flags, stack_ptr);

            call_ev = new VanadisSyscallMemoryMap2Event(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, map_addr, map_len, map_prot, sst_flags,
                                                       stack_ptr, 4096);
        } break;

        case VANADIS_SYSCALL_MIPS_GETTIME64: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            int64_t clk_type = regFile->getIntReg<int64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t time_addr = regFile->getIntReg<uint64_t>(phys_reg_5);

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to clock_gettime64( %" PRId64 ", 0x%llx )\n", clk_type,
                            time_addr);

            call_ev = new VanadisSyscallGetTime64Event(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_32B, clk_type, time_addr);
        } break;

        case VANADIS_SYSCALL_MIPS_RT_SETSIGMASK: {
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            int32_t how = regFile->getIntReg<int32_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t signal_set_in = regFile->getIntReg<uint64_t>(phys_reg_5);

            const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
            uint64_t signal_set_out = regFile->getIntReg<uint64_t>(phys_reg_6);

            const uint16_t phys_reg_7 = isaTable->getIntPhysReg(7);
            int32_t signal_set_size = regFile->getIntReg<int32_t>(phys_reg_7);

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to rt_sigprocmask( %" PRId32 ", 0x%llx, 0x%llx, %" PRId32
                            ")\n",
                            how, signal_set_in, signal_set_out, signal_set_size);

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
            output->verbose(CALL_INFO, 16, 0, "Sending event to operating system...\n");
            sendSyscallEvent(call_ev);
            return false;
        } else {
            return true;
        }
    }

    void recvSyscallResp( VanadisSyscallResponse* os_resp ) {
        output->verbose(CALL_INFO, 8, 0, "syscall return-code: %" PRId64 " (success: %3s)\n",
                            os_resp->getReturnCode(), os_resp->isSuccessful() ? "yes" : "no");
        output->verbose(CALL_INFO, 8, 0, "-> issuing call-backs to clear syscall ROB stops...\n");

        // Set up the return code (according to ABI, this goes in r2)
        const uint16_t rc_reg = isaTable->getIntPhysReg(2);
        const int64_t rc_val = (int64_t)os_resp->getReturnCode();
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
        const uint64_t os_success = 0;
        const uint64_t os_failed = 1;
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


	uint64_t convertFlags( uint64_t flags ) {
		uint64_t out = 0;

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
        if ( flags & MIPS_O_LARGEFILE ) {
            flags &= ~MIPS_O_LARGEFILE;
        }
#endif
        assert( 0 == flags );

		return out;
	}
    uint32_t getRegister( int reg ) {
        return regFile->getIntReg<uint32_t>( isaTable->getIntPhysReg( reg ) );
    }

    bool brk_zero_memory;
};

} // namespace Vanadis
} // namespace SST

#endif
