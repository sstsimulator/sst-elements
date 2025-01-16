// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_OP_SYS2
#define _H_VANADIS_OP_SYS2

#include "os/vcpuos.h"

namespace SST {
namespace Vanadis {

#define VANADIS_AT_FDCWD -100

#define InstallFuncPtr( opCode, funcName ) case SYSCALL_OP_##funcName:  m_functionMap[ opCode ] = MakeFuncPtr( funcName ); break;
#define MakeFuncPtr( funcName ) std::bind(&VanadisCPUOSHandler2< T1,BitType,regZero,OsCodeReg,LinkReg >::funcName, this, std::placeholders::_1 )
       
#define Install_ISA_FuncPtr( isa, funcName ) install( VANADIS_SYSCALL_##isa##_##funcName, Make_ISA_FuncPtr( isa, funcName ) )
#define Make_ISA_FuncPtr( isa, x ) std::bind(&Vanadis##isa##OSHandler2< T1,BitType,RegZero,OsCodeReg,LinkReg >::x, this, std::placeholders::_1 )

#define FOREACH_COMMON( FUNC, arg ) \
        FUNC( arg, BRK ); \
        FUNC( arg, CLOSE ); \
        FUNC( arg, EXIT ); \
        FUNC( arg, EXIT_GROUP); \
        FUNC( arg, FSTAT ); \
        FUNC( arg, GETPGID ); \
        FUNC( arg, GETPID ); \
        FUNC( arg, GETPPID ); \
        FUNC( arg, GETTID ); \
        FUNC( arg, IOCTL); \
        FUNC( arg, KILL ); \
        FUNC( arg, MADVISE ); \
        FUNC( arg, MPROTECT ); \
        FUNC( arg, OPENAT ); \
        FUNC( arg, READ ); \
        FUNC( arg, READV ); \
        FUNC( arg, RT_SIGACTION ); \
        FUNC( arg, RT_SIGPROCMASK ); \
        FUNC( arg, SCHED_GETAFFINITY ); \
        FUNC( arg, SET_TID_ADDRESS ); \
        FUNC( arg, UNAME ); \
        FUNC( arg, UNLINKAT ); \
        FUNC( arg, UNMAP ); \
        FUNC( arg, WRITE ); \
        FUNC( arg, WRITEV); 


#define InstallCommonFuncs( isa ) FOREACH_COMMON( InstallFunc, isa )

#define InstallFunc( isa,funcName ) install( VANADIS_SYSCALL_##isa##_##funcName, SYSCALL_OP_##funcName )

#define AddUsing\
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::isaTable; \
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::regFile; \
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::output; \
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::core_id; \
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::sendSyscallEvent; \
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::getArgRegister; \
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::getLinkReg; \
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::getOsCode; \
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::tls_address; \
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::install; \
    using VanadisCPUOSHandler2< T1, BitType, RegZero, OsCodeReg, LinkReg >::instPtr;

template <class T1, VanadisOSBitType BitType, int regZero, int OsCodeReg, int LinkReg >
class VanadisCPUOSHandler2 : public VanadisCPUOSHandler {

public:
    VanadisCPUOSHandler2(ComponentId_t id, Params& params, const char* isaName ) : VanadisCPUOSHandler(id,params), m_isaName(isaName) { }

    typedef std::function<VanadisSyscallEvent*(int)> FuncPtr;

    void install( int opCode, FuncPtr funcPtr ) {
        output->verbose(CALL_INFO, 16, 0, " opCode %d \n", opCode);

        assert( m_functionMap.find( opCode ) == m_functionMap.end() );

        m_functionMap[ opCode ] = funcPtr;
    }

    void install( int opCode, VanadisSyscallOp syscall ) {

        output->verbose(CALL_INFO, 16, 0, " opCode %d -> syscall %d\n", opCode,syscall);

        assert( m_functionMap.find( opCode ) == m_functionMap.end() );

        switch ( syscall ) {
            FOREACH_COMMON( InstallFuncPtr, opCode )
            default: assert(0);
        }
    }
    uint64_t instPtr;
    virtual std::tuple<bool,bool> handleSysCall(VanadisSysCallInstruction* syscallIns) {
        instPtr = syscallIns->getInstructionAddress();
        uint64_t call_link_value = getLinkReg();

        flushLSQ = false;
        const uint32_t hw_thr = syscallIns->getHWThread();

        const uint64_t os_code = getOsCode();

        output->verbose(CALL_INFO, 16, 0, "core=%d hw_thr=%d syscall-ins: %#" PRIx64 ", link-reg: %#" PRIx64", os_code=%" PRIu64 ", linkReg=(%d,%d), OsCodeReg=(%d,%d)\n",
                        core_id, hw_thr, syscallIns->getInstructionAddress(), call_link_value, os_code, LinkReg, isaTable->getIntPhysReg(LinkReg), OsCodeReg, isaTable->getIntPhysReg(OsCodeReg));
        VanadisSyscallEvent* call_ev = nullptr;

        if ( m_functionMap.find( os_code ) != m_functionMap.end() ) {
            call_ev = m_functionMap[os_code]( hw_thr ); // TODO_Anu: will the functionmap have entries for the sub threads != super thread?
        } else {
            output->fatal(CALL_INFO, -1, "Error: unknown code %" PRIu64 " (ins: %#" PRIx64 ", link-reg: %#" PRIx64 ")\n",
                os_code, syscallIns->getInstructionAddress(), call_link_value);
        }

        if (nullptr != call_ev) {
            output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL, "Sending event to operating system...\n");
            sendSyscallEvent(call_ev);
            return std::make_tuple(false,flushLSQ);
        } else {
            // printf("Returning Syscall return True\n");
            return std::make_tuple(true,flushLSQ);
        }
    }

protected:

    VanadisSyscallEvent* KILL( int hw_thr ) {
        T1  pid = getArgRegister( 0 );
        T1  sig = getArgRegister( 1 );

        output->verbose(CALL_INFO, 16, 0, "kill( %" PRIdXX", %" PRIdXX"  )\n",pid,sig);

        return new VanadisSyscallKillEvent(core_id, hw_thr, VanadisOSBitType::VANADIS_OS_64B, pid, sig );
    }

    VanadisSyscallEvent* MADVISE( int hw_thr ) {
        T1 advise_addr    = getArgRegister( 0 );
        T1 advise_len     = getArgRegister( 1 );
        T1 advise_advice  = getArgRegister( 2 );

        output->verbose(CALL_INFO, 16, 0,
                            "madvise( %#" PRIxXX ", %" PRIuXX ", %" PRIuXX " )\n", advise_addr, advise_len, advise_advice);

        printf("Warning: VANADIS_SYSCALL_%s_%s not implemented returning success\n",m_isaName,__func__);

        recvSyscallResp(new VanadisSyscallResponse(0));
        return nullptr;
    }

    VanadisSyscallEvent* FSTAT( int hw_thr ) {
        T1 file_handle    = getArgRegister(0);
        T1 addr           = getArgRegister(1);

        output->verbose(CALL_INFO, 16, 0, "fstat( %" PRIdXX ", %" PRIuXX " )\n", file_handle, addr);

        return new VanadisSyscallFstatEvent(core_id, hw_thr, BitType, file_handle, addr);
    }

    VanadisSyscallEvent* SCHED_GETAFFINITY( int hw_thr ) {
        T1 pid        = getArgRegister(0);
        T1 cpusetsize = getArgRegister(1);
        T1 maskAddr   = getArgRegister(2);

        output->verbose(CALL_INFO, 16, 0,
                            "sched_getaffinity( %" PRIdXX ", %" PRIdXX", %#" PRIxXX " )\n", pid, cpusetsize, maskAddr );

        return new VanadisSyscallGetaffinityEvent(core_id, hw_thr, BitType, pid, cpusetsize, maskAddr );
    }
    VanadisSyscallEvent* MPROTECT( int hw_thr ) {
        T1 addr   = getArgRegister(0);
        T1 len    = getArgRegister(1);
        T1 prot   = getArgRegister(2);

        T1 myProt = 0;
        if ( prot & 0x1 ) {
            myProt |= PROT_READ;
        }
        if ( prot & 0x2 ) {
            myProt |= PROT_WRITE;
        }

        return new VanadisSyscallMprotectEvent(core_id, hw_thr, BitType, addr, len, myProt );
    }

    VanadisSyscallEvent* RT_SIGACTION( int hw_thr ) {
        printf("Warning: VANADIS_SYSCALL_%s_%s not implemented returning success\n",m_isaName,__func__);
        recvSyscallResp(new VanadisSyscallResponse(0));
        return nullptr;
    }

    VanadisSyscallEvent* RT_SIGPROCMASK( int hw_thr ) {
        T1 how        = getArgRegister(0);
        T1 set_in     = getArgRegister(1);
        T1 set_out    = getArgRegister(2);
        T1 set_size   = getArgRegister(3);

        output->verbose(CALL_INFO, 16, 0, "rt_sigprocmask( %" PRIdXX ", %#" PRIxXX ", %#" PRIxXX ", %" PRIdXX ")\n", how, set_in, set_out, set_size);
        printf("Warning: VANADIS_SYSCALL_%s_%s not implemented returning success\n",m_isaName,__func__);

        recvSyscallResp(new VanadisSyscallResponse(0));
        return nullptr;
    }

    VanadisSyscallEvent* UNMAP( int hw_thr ) {
        T1 addr   = getArgRegister(0);
        T1 len    = getArgRegister(1);

        output->verbose(CALL_INFO, 16, 0, "unmap( %#" PRIxXX", %" PRIuXX " )\n", addr, len);

        if ((0 == addr)) {
            recvSyscallResp(new VanadisSyscallResponse(-22));
            return nullptr;
        } else {
            return new VanadisSyscallMemoryUnMapEvent(core_id, hw_thr, BitType, addr, len);
        }
    }

    VanadisSyscallEvent* READV( int hw_thr ) {
        T1 fd          = getArgRegister(0);
        T1 iovec_ptr   = getArgRegister(1);
        T1 iovec_count = getArgRegister(2);

        output->verbose(CALL_INFO, 16, 0, "readv( %" PRIdXX ", %#" PRIxXX", %" PRIdXX " )\n", fd, iovec_ptr, iovec_count);
        return new VanadisSyscallReadvEvent(core_id, hw_thr, BitType, fd, iovec_ptr, iovec_count);
    }

    VanadisSyscallEvent* EXIT( int hw_thr ) {
        T1 code = getArgRegister(0);

        output->verbose(CALL_INFO, 16, 0, "exit( %" PRIdXX " )\n", code);
        flushLSQ = true;
        return new VanadisSyscallExitEvent(core_id, hw_thr, BitType, code);
    }

    VanadisSyscallEvent* GETPID( int hw_thr ) {
        output->verbose(CALL_INFO, 16, 0, "getpid()\n");
        return new VanadisSyscallGetxEvent(core_id, hw_thr, BitType,SYSCALL_OP_GETPID);
    }

    VanadisSyscallEvent* GETPGID( int hw_thr ) {
        output->verbose(CALL_INFO, 16, 0, "getpgid()\n");
        return new VanadisSyscallGetxEvent(core_id, hw_thr, BitType,SYSCALL_OP_GETPGID);
    }
    VanadisSyscallEvent* GETPPID( int hw_thr ) {
        output->verbose(CALL_INFO, 16, 0, "getppid()\n");
        return new VanadisSyscallGetxEvent(core_id, hw_thr, BitType,SYSCALL_OP_GETPPID);
    }

    VanadisSyscallEvent* GETTID( int hw_thr ) {
        output->verbose(CALL_INFO, 16, 0, "gettid()\n");
        return new VanadisSyscallGetxEvent(core_id, hw_thr, BitType,SYSCALL_OP_GETTID);
    }

    VanadisSyscallEvent* READ( int hw_thr ) {
        T1 fd       = getArgRegister(0);
        T1 buff_ptr = getArgRegister(1);
        T1 count    = getArgRegister(2);

        output->verbose(CALL_INFO, 16, 0, "read( %" PRIdXX ", %#" PRIxXX ", %#" PRIxXX ")\n", fd, buff_ptr, count );
        return new VanadisSyscallReadEvent(core_id, hw_thr, BitType, fd, buff_ptr, count);
    }

    VanadisSyscallEvent* WRITE( int hw_thr ) {
        T1 fd     = getArgRegister(0);
        T1 buff   = getArgRegister(1);
        T1 count  = getArgRegister(2);

        output->verbose(CALL_INFO, 16, 0, "write( %" PRIdXX ", %#" PRIxXX ", %" PRIuXX " )\n", fd, buff, count);
        return new VanadisSyscallWriteEvent(core_id, hw_thr, BitType, fd, buff, count);
    }

    VanadisSyscallEvent* UNLINKAT( int hw_thr ) {
        T1 dirFd       = getArgRegister( 0 );
        T1 path_addr   = getArgRegister( 1 );
        T1 flags       = getArgRegister( 2 );

        #ifdef SST_COMPILE_MACOSX
        if (  VANADIS_AT_FDCWD == dirFd ) {
            dirFd = AT_FDCWD;
        }
        #endif
        output->verbose(CALL_INFO, 16, 0, "unlinkat( %" PRIdXX ", %" PRIdXX ", %#" PRIxXX" )\n",dirFd,path_addr,flags);

        return new VanadisSyscallUnlinkatEvent(core_id, hw_thr, BitType, dirFd,path_addr,flags);
    }

    VanadisSyscallEvent* CLOSE( int hw_thr ) {
        T1 file = getArgRegister(0);

        output->verbose(CALL_INFO, 16, 0, "close( %" PRIuXX " )\n", file);


       return new VanadisSyscallCloseEvent(core_id, hw_thr, BitType, file);
    }

    VanadisSyscallEvent* OPENAT( int hw_thr ) {
        T1 dirfd      = getArgRegister(0);
        T1 path_ptr   = getArgRegister(1);
        T1 flags      = getArgRegister(2);
        T1 mode       = getArgRegister(3);

        #ifdef SST_COMPILE_MACOSX
        if (  VANADIS_AT_FDCWD == dirfd ) {
            dirfd = AT_FDCWD;
        }
        #endif
        output->verbose(CALL_INFO, 16, 0, "openat( %" PRIuXX ", %#" PRIxXX ", %#" PRIxXX ", %#" PRIxXX ")\n", dirfd, path_ptr, flags, mode );
        return new VanadisSyscallOpenatEvent(core_id, hw_thr, BitType, dirfd, path_ptr, convertFlags(flags), mode);
    }

    VanadisSyscallEvent* BRK( int hw_thr ) {
        T1 newBrk = getArgRegister(0);

        output->verbose(CALL_INFO, 16, 0, "brk( value: %" PRIuXX " )\n", newBrk);
        
        return new VanadisSyscallBRKEvent(core_id, hw_thr, BitType, newBrk );
    }

    VanadisSyscallEvent* WRITEV( int hw_thr ) {
        T1 writev_fd          = getArgRegister( 0 );
        T1 writev_iovec_ptr   = getArgRegister( 1 );
        T1 writev_iovec_count = getArgRegister( 2 );
        return new VanadisSyscallWritevEvent(core_id, hw_thr, BitType, writev_fd, writev_iovec_ptr, writev_iovec_count);
    } 

    VanadisSyscallEvent* SET_TID_ADDRESS( int hw_thr ) {
        T1 addr = getArgRegister( 0 );

        output->verbose(CALL_INFO, 16, 0, "set_tid_address( %#" PRIxXX " )\n", addr);
        
        return new VanadisSyscallSetTidAddressEvent(core_id, hw_thr, BitType, addr);
    }

    VanadisSyscallEvent* UNAME( int hw_thr ) {
        T1 addr = getArgRegister(0);

        output->verbose(CALL_INFO, 16, 0, "uname( %#" PRIxXX ")\n",addr);
        return new VanadisSyscallUnameEvent(core_id, hw_thr, BitType, addr);
    }

    VanadisSyscallEvent* EXIT_GROUP( int hw_thr ) {
        T1 exit_code = getArgRegister( 0 );

        output->verbose(CALL_INFO, 16, 0, "exit_group( %" PRIdXX " )\n", exit_code);
        
        flushLSQ=true;

        return new VanadisSyscallExitGroupEvent(core_id, hw_thr, BitType, exit_code);
    }

    VanadisSyscallEvent* IOCTL( int hw_thr ) {
        T1 fd     = getArgRegister(0);
        T1 io_req = getArgRegister(1);
        T1 ptr    = getArgRegister(2);

        T1 access_type = (io_req & 0xE0000000) >> 29;

        bool is_read = (access_type & 0x1) != 0;
        bool is_write = (access_type & 0x2) != 0;

        T1 data_size = ((io_req)&0x1FFF0000) >> 16;
        T1 io_op = ((io_req)&0xFF);

        T1 io_driver = ((io_req)&0xFF00) >> 8;

        output->verbose(CALL_INFO, 16, 0,
                            "ioctl( %" PRIdXX ", %" PRIuXX " / %#" PRIxXX ", %" PRIuXX " / %#" PRIxXX " )\n",
                            fd, io_req, io_req, ptr, ptr);
        output->verbose(CALL_INFO, 16, 0,
                          "-> R: %c W: %c / size: %" PRIuXX " / op: %" PRIuXX " / drv: %" PRIuXX "\n",
                            is_read ? 'y' : 'n', is_write ? 'y' : 'n', data_size, io_op, io_driver);

        return new VanadisSyscallIoctlEvent(core_id, hw_thr, BitType, fd, is_read, is_write, io_op, io_driver, ptr, data_size);
    }

    virtual T1 convertFlags( T1 ) = 0;

    // SHOULD getIntReg be type uint64_t? 
    T1 getLinkReg() {
        return regFile->getIntReg< uint64_t >( isaTable->getIntPhysReg(LinkReg));
    }

    T1 getOsCode() {
        return regFile->getIntReg<uint64_t>( isaTable->getIntPhysReg(OsCodeReg) );
    }

    T1 getArgRegister( int reg ) {
        return regFile->getIntReg<uint64_t>( isaTable->getIntPhysReg( reg + regZero ) );
    }
private:
    std::map<int,FuncPtr> m_functionMap;
    bool flushLSQ;
    const char* m_isaName;
};

} // namespace Vanadis
} // namespace SST

#endif
