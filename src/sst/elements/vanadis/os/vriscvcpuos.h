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
#include "os/resp/voscallresp.h"
#include "os/resp/vosexitresp.h"
#include "os/vcpuos.h"
#include "os/voscallev.h"
#include <functional>

#include <fcntl.h>

#define GET_REGISTER( reg ) \
            regFile->getIntReg<uint64_t>( isaTable->getIntPhysReg(reg) );

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

#ifndef SST_COMPILE_MACOSX
#define RISCV_O_DIRECT      040000
#define RISCV_O_LARGEFILE   0100000 
#define RISCV_O_NOATIME     01000000  
#define RISCV_O_PATH        010000000 
#define RISCV_O_TMPFILE     020200000 
#endif


#define VANADIS_SYSCALL_RISCV_READ 63
#define VANADIS_SYSCALL_RISCV_CLOSE 57
#define VANADIS_SYSCALL_RISCV_WRITE 64
#define VANADIS_SYSCALL_RISCV_BRK 214
#define VANADIS_SYSCALL_RISCV_IOCTL 29
#define VANADIS_SYSCALL_RISCV_MMAP 222
#define VANADIS_SYSCALL_RISCV_UNMAP 215
#define VANADIS_SYSCALL_RISCV_UNAME 160
#define VANADIS_SYSCALL_RISCV_WRITEV 66
#define VANADIS_SYSCALL_RISCV_RT_SETSIGMASK 135
#define VANADIS_SYSCALL_RISCV_MADVISE 233
#define VANADIS_SYSCALL_RISCV_FUTEX 98
#define VANADIS_SYSCALL_RISCV_SET_TID 96
#define VANADIS_SYSCALL_RISCV_EXIT 93
#define VANADIS_SYSCALL_RISCV_EXIT_GROUP 94
#define VANADIS_SYSCALL_RISCV_RM_INOTIFY 28
#define VANADIS_SYSCALL_RISCV_OPENAT 56
#define VANADIS_SYSCALL_RISCV_RET_REG 10
#define VANADIS_SYSCALL_RISCV_SET_RLIST 99
#define VANADIS_SYSCALL_RISCV_GET_RLIST 100

//These are undefined in RV64
#define VANADIS_SYSCALL_RISCV_GETTIME64 4403
#define VANADIS_SYSCALL_RISCV_SET_THREAD_AREA 4283
#define VANADIS_SYSCALL_RISCV_MMAP2 4210
#define VANADIS_SYSCALL_RISCV_FSTAT 4215
#define VANADIS_SYSCALL_RISCV_READLINK 4085
#define VANADIS_SYSCALL_RISCV_ACCESS 4033

namespace SST {
namespace Vanadis {

class VanadisRISCV64OSHandler : public VanadisCPUOSHandler {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(VanadisRISCV64OSHandler, "vanadis", "VanadisRISCV64OSHandler",
                                          SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                          "Provides SYSCALL handling for a RISCV-based decoding core",
                                          SST::Vanadis::VanadisCPUOSHandler)

    SST_ELI_DOCUMENT_PORTS({ "os_link", "Connects this handler to the main operating system of the node", {} })

    SST_ELI_DOCUMENT_PARAMS({ "brk_zero_memory", "Zero memory during OS calls to brk", "0" })

    VanadisRISCV64OSHandler(ComponentId_t id, Params& params) : VanadisCPUOSHandler(id, params) {

        os_link = configureLink("os_link", "0ns",
                                new Event::Handler<VanadisRISCV64OSHandler>(this, &VanadisRISCV64OSHandler::recvOSEvent));

        brk_zero_memory = params.find<bool>("brk_zero_memory", false);
    }

    virtual ~VanadisRISCV64OSHandler() {}

    virtual void handleSysCall(VanadisSysCallInstruction* syscallIns) {
        const uint16_t call_link_reg = isaTable->getIntPhysReg(31);
        uint64_t call_link_value = regFile->getIntReg<uint64_t>(call_link_reg);
        output->verbose(CALL_INFO, 8, 0, "System Call (syscall-ins: 0x%0llx, link-reg: 0x%llx)\n",
                        syscallIns->getInstructionAddress(), call_link_value);

        const uint32_t hw_thr = syscallIns->getHWThread();

        // RISCV puts codes in GPR r2
        const uint16_t os_code_phys_reg = isaTable->getIntPhysReg(17);
        const uint64_t os_code = regFile->getIntReg<uint64_t>(os_code_phys_reg);

        output->verbose(CALL_INFO, 8, 0, "--> [SYSCALL-handler] syscall-ins: 0x%0llx / call-code: %" PRIu64 "\n",
                        syscallIns->getInstructionAddress(), os_code);
        VanadisSyscallEvent* call_ev = nullptr;

        switch (os_code) {
        case VANADIS_SYSCALL_RISCV_READLINK: {
    assert(0);
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t readlink_path = regFile->getIntReg<uint64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t readlink_buff_ptr = regFile->getIntReg<uint64_t>(phys_reg_5);

            const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
            int64_t readlink_size = regFile->getIntReg<uint64_t>(phys_reg_6);

            call_ev = new VanadisSyscallReadLinkEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, readlink_path, readlink_buff_ptr, readlink_size);
        } break;

        case VANADIS_SYSCALL_RISCV_READ: {
    assert(0);
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            int64_t read_fd = regFile->getIntReg<int64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t read_buff_ptr = regFile->getIntReg<uint64_t>(phys_reg_5);

            const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
            int64_t read_count = regFile->getIntReg<int64_t>(phys_reg_6);

            call_ev = new VanadisSyscallReadEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, read_fd, read_buff_ptr, read_count);
        } break;

        case VANADIS_SYSCALL_RISCV_ACCESS: {
    assert(0);
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t path_ptr = regFile->getIntReg<uint64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t access_mode = regFile->getIntReg<uint64_t>(phys_reg_5);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to access( 0x%llx, %" PRIu64 " )\n",
                            path_ptr, access_mode);
            call_ev = new VanadisSyscallAccessEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, path_ptr, access_mode);
        } break;

        case VANADIS_SYSCALL_RISCV_BRK: {
            const uint64_t phys_reg_10 = isaTable->getIntPhysReg(10);
            uint64_t newBrk = regFile->getIntReg<uint64_t>(phys_reg_10);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to brk( value: %" PRIu64 " / 0x%llx ), zero: %s\n",
                            newBrk, newBrk, brk_zero_memory ? "yes" : "no");
            call_ev = new VanadisSyscallBRKEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, newBrk, brk_zero_memory);
        } break;

        case VANADIS_SYSCALL_RISCV_SET_THREAD_AREA: {
    assert(0);
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

        case VANADIS_SYSCALL_RISCV_RM_INOTIFY: {
    assert(0);
            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to inotify_rm_watch(), "
                            "by-passing and removing.\n");
            const uint16_t rc_reg = isaTable->getIntPhysReg(VANADIS_SYSCALL_RISCV_RET_REG);
            regFile->setIntReg(rc_reg, (uint64_t)0);

            for (int i = 0; i < returnCallbacks.size(); ++i) {
                returnCallbacks[i](hw_thr);
            }
        } break;

        case VANADIS_SYSCALL_RISCV_UNAME: {
    assert(0);
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t uname_addr = regFile->getIntReg<uint64_t>(phys_reg_4);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to uname()\n");

            call_ev = new VanadisSyscallUnameEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, uname_addr);
        } break;

        case VANADIS_SYSCALL_RISCV_FSTAT: {
    assert(0);
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            int32_t file_handle = regFile->getIntReg<int32_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t fstat_addr = regFile->getIntReg<uint64_t>(phys_reg_5);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to fstat( %" PRId32 ", %" PRIu64 " )\n",
                            file_handle, fstat_addr);

            call_ev = new VanadisSyscallFstatEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, file_handle, fstat_addr);
        } break;

        case VANADIS_SYSCALL_RISCV_CLOSE: {
            const uint64_t close_file = GET_REGISTER( 10 );

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to close( %" PRIu64 " )\n", close_file);

            call_ev = new VanadisSyscallCloseEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, close_file);
        } break;

        case VANADIS_SYSCALL_RISCV_OPENAT: {
            uint64_t openat_dirfd = GET_REGISTER( 10 );
            uint64_t openat_path_ptr = GET_REGISTER( 11 );
            uint64_t openat_flags = GET_REGISTER( 12 );
            uint64_t openat_mode = GET_REGISTER(13);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to openat( %d, %#llx, %#" PRIx64 ", %#" PRIx64 ")\n",
                    openat_dirfd, openat_path_ptr, openat_flags, openat_mode);

            call_ev = new VanadisSyscallOpenAtEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, openat_dirfd, openat_path_ptr, convertFlags(openat_flags), openat_mode);
        } break;

        case VANADIS_SYSCALL_RISCV_WRITEV: {
            int64_t writev_fd = GET_REGISTER( 10 );
            uint64_t writev_iovec_ptr = GET_REGISTER( 11 );
            int64_t writev_iovec_count = GET_REGISTER( 12 );

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
        } break;

        case VANADIS_SYSCALL_RISCV_EXIT_GROUP: {
            const uint16_t phys_reg_10 = isaTable->getIntPhysReg(10);
            int64_t exit_code = regFile->getIntReg<int64_t>(phys_reg_10);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to exit_group( %" PRId64 " )\n",
                            exit_code);
            call_ev = new VanadisSyscallExitGroupEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, exit_code);
        } break;

        case VANADIS_SYSCALL_RISCV_WRITE: {
            int64_t write_fd = GET_REGISTER( 10 );
            uint64_t write_buff = GET_REGISTER( 11 );
            uint64_t write_count = GET_REGISTER( 12 );

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to write( %" PRId64 ", 0x%llx, %" PRIu64 " )\n", write_fd,
                            write_buff, write_count);
            call_ev = new VanadisSyscallWriteEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, write_fd, write_buff, write_count);
        } break;

        case VANADIS_SYSCALL_RISCV_SET_TID: {
            const uint16_t phys_reg_10 = isaTable->getIntPhysReg(10);
            int64_t new_tid = regFile->getIntReg<int64_t>(phys_reg_10);

            setThreadID(new_tid);
            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found call to set_tid( %" PRId64 " / 0x%llx )\n", new_tid, new_tid);

            recvOSEvent(new VanadisSyscallResponse(new_tid));
        } break;

        case VANADIS_SYSCALL_RISCV_MADVISE: {
    assert(0);
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
            recvOSEvent(new VanadisSyscallResponse(0));
        } break;

        case VANADIS_SYSCALL_RISCV_FUTEX: {
    assert(0);
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

            recvOSEvent(new VanadisSyscallResponse(0));
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

            call_ev = new VanadisSyscallIOCtlEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, fd, is_read, is_write, io_op, io_driver, ptr,
                                                   data_size);
        } break;

        case VANADIS_SYSCALL_RISCV_MMAP: {
            const uint16_t phys_reg_10 = isaTable->getIntPhysReg(10);
            uint64_t map_addr = regFile->getIntReg<uint64_t>(phys_reg_10);

            const uint16_t phys_reg_11 = isaTable->getIntPhysReg(11);
            uint64_t map_len = regFile->getIntReg<uint64_t>(phys_reg_11);

            const uint16_t phys_reg_12 = isaTable->getIntPhysReg(12);
            int32_t map_prot = regFile->getIntReg<int32_t>(phys_reg_12);

            const uint16_t phys_reg_13 = isaTable->getIntPhysReg(13);
            int32_t map_flags = regFile->getIntReg<int32_t>(phys_reg_13);

            const uint16_t phys_reg_14 = isaTable->getIntPhysReg(14);
            int32_t map_fd = regFile->getIntReg<int32_t>(phys_reg_14);

            const uint16_t phys_reg_15 = isaTable->getIntPhysReg(15);
            int64_t map_offset = regFile->getIntReg<int64_t>(phys_reg_15);

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to mmap( 0x%llx, %" PRIu64 ", %" PRId32 ", %" PRId32
                            ", %" PRId32 ", %" PRId64 " )\n",
                            map_addr, map_len, map_prot, map_flags, map_fd, map_offset);

            output->fatal(CALL_INFO, -1, "STOP\n");

            if ((0 == map_addr) && (0 == map_len)) {
                recvOSEvent(new VanadisSyscallResponse(-22));
            } else {
            }
        } break;

        case VANADIS_SYSCALL_RISCV_UNMAP: {
    assert(0);
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            uint64_t unmap_addr = regFile->getIntReg<uint64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t unmap_len = regFile->getIntReg<uint64_t>(phys_reg_5);

            output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to unmap( 0x%llx, %" PRIu64 " )\n",
                            unmap_addr, unmap_len);

            if ((0 == unmap_addr)) {
                recvOSEvent(new VanadisSyscallResponse(-22));
            } else {
                call_ev = new VanadisSyscallMemoryUnMapEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, unmap_addr, unmap_len);
            }
        } break;

        case VANADIS_SYSCALL_RISCV_MMAP2: {
    assert(0);
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

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to mmap2( 0x%llx, %" PRIu64 ", %" PRId32 ", %" PRId32
                            ", sp: 0x%llx (> 4 arguments) )\n",
                            map_addr, map_len, map_prot, map_flags, stack_ptr);

            call_ev = new VanadisSyscallMemoryMapEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, map_addr, map_len, map_prot, map_flags,
                                                       stack_ptr, 4096);
        } break;

        case VANADIS_SYSCALL_RISCV_GETTIME64: {
    assert(0);
            const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
            int64_t clk_type = regFile->getIntReg<int64_t>(phys_reg_4);

            const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
            uint64_t time_addr = regFile->getIntReg<uint64_t>(phys_reg_5);

            output->verbose(CALL_INFO, 8, 0,
                            "[syscall-handler] found a call to clock_gettime64( %" PRId64 ", 0x%llx )\n", clk_type,
                            time_addr);

            call_ev = new VanadisSyscallGetTime64Event(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, clk_type, time_addr);
        } break;

        case VANADIS_SYSCALL_RISCV_RT_SETSIGMASK: {
    assert(0);
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

            recvOSEvent(new VanadisSyscallResponse(0));
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
            os_link->send(call_ev);
        }
    }

protected:
/*
    void writeSyscallResult(const bool success) {
        const uint64_t os_success = 0;
        const uint64_t os_failed = 1;
        const uint16_t succ_reg = isaTable->getIntPhysReg(VANADIS_SYSCALL_RISCV_RET_REG);

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
*/
    void recvOSEvent(SST::Event* ev) {
        output->verbose(CALL_INFO, 8, 0, "-> recv os response\n");

        VanadisSyscallResponse* os_resp = dynamic_cast<VanadisSyscallResponse*>(ev);

        if (nullptr != os_resp) {
            output->verbose(CALL_INFO, 8, 0, "syscall return-code: %" PRId64 " (success: %3s)\n",
                            os_resp->getReturnCode(), os_resp->isSuccessful() ? "yes" : "no");
            output->verbose(CALL_INFO, 8, 0, "-> issuing call-backs to clear syscall ROB stops...\n");

            // Set up the return code (according to ABI, this goes in r2)
            const uint16_t rc_reg = isaTable->getIntPhysReg(VANADIS_SYSCALL_RISCV_RET_REG);
            const int64_t rc_val = (int64_t)os_resp->getReturnCode();
            regFile->setIntReg(rc_reg, rc_val);

//            if (os_resp->isSuccessful()) {
//                if (rc_val < 0) {
//                    writeSyscallResult(false);
//                } else {
//                    // Generate correct markers for OS return code checks
//                    writeSyscallResult(os_resp->isSuccessful());
//                }
//            } else {
//                writeSyscallResult(false);
//            }

            for (int i = 0; i < returnCallbacks.size(); ++i) {
                returnCallbacks[i](hw_thr);
            }
        } else {
            VanadisStartThreadReq* os_req = dynamic_cast<VanadisStartThreadReq*>(ev);
 
            if ( nullptr != os_req ) {
                output->verbose(CALL_INFO, 8, 0,
                            "received start thread %d command from the operating system \n",os_req->getThread());
                startThrCallBack(os_req->getThread(), os_req->getStackStart(), os_req->getInstructionPointer());
            } else {

                VanadisExitResponse* os_exit = dynamic_cast<VanadisExitResponse*>(ev);

                if ( nullptr != os_exit ) {
                    output->verbose(CALL_INFO, 8, 0,
                             "received an exit command from the operating system "
                             "(return-code: %" PRId64 " )\n",
                             os_exit->getReturnCode());
 
                    haltThrCallBack(hw_thr, os_exit->getReturnCode());
                } else { 
                    assert(0);
                }
            } 
        }

        delete ev;
    }

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

#ifndef SST_COMPILE_MACOSX
        RISC_CONVERT( DIRECT );
        RISC_CONVERT( LARGEFILE );
        RISC_CONVERT( NOATIME );
        RISC_CONVERT( PATH );
        RISC_CONVERT( TMPFILE );
#endif

        assert( 0 == flags );

        return out;
    }


    SST::Link* os_link;
    bool brk_zero_memory;
};

} // namespace Vanadis
} // namespace SST

#endif
