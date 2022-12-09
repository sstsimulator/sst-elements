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

#ifndef _H_VANADIS_NODE_OS_INCLUDE_PROCESS
#define _H_VANADIS_NODE_OS_INCLUDE_PROCESS

#include <math.h>
#include <sys/mman.h>

#include "sst/elements/mmu/mmu.h"

#include "os/include/hwThreadID.h"
#include "os/include/virtMemMap.h"
#include "os/include/fdTable.h"
#include "os/include/futex.h"
#include "os/include/threadGrp.h"
#include "os/include/page.h"
#include "os/vphysmemmanager.h"

namespace SST {
namespace Vanadis {

class VanadisSyscall;

namespace OS {

class ProcessInfo {
  public:

    ProcessInfo( ) : m_pid(-1), m_fileTable(nullptr), m_virtMemMap(nullptr), m_tidAddress(0) { }
    
    ProcessInfo( const ProcessInfo& obj, unsigned pid, unsigned debug_level = 0 ) : m_pid(pid), m_tid(pid), m_tidAddress(0) { 

        char buffer[100];
        snprintf(buffer,100,"@t::ProcessInfo::@p():@l ");
        m_dbg.init( buffer, debug_level, 0,Output::STDOUT );

        m_mmu = obj.m_mmu;
        m_physMemMgr = obj.m_physMemMgr;
        m_elfInfo = obj.m_elfInfo;
        m_pageSize = obj.m_pageSize;
        m_params = obj.m_params; 
        m_pageShift = obj.m_pageShift;
        m_ppid = obj.m_pid;
        m_pgid = obj.m_pgid;
       
        m_futex = new Futex;
        m_threadGrp = new ThreadGrp( this );
        m_virtMemMap = new VirtMemMap( *obj.m_virtMemMap );
        m_fileTable = new FileDescriptorTable( 1024 ); 

        //openFileWithFd( "stdin-" + std::to_string(pid), 0 );
        openFileWithFd( "stdout-" + std::to_string(m_pid), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR, 1 );
        openFileWithFd( "stderr-" + std::to_string(m_pid), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR, 2 );

        m_fileTable->update( obj.m_fileTable );
    }

    ProcessInfo( MMU_Lib::MMU* mmu, PhysMemManager* physMemMgr, unsigned pid, VanadisELFInfo* elfInfo, int debug_level, unsigned pageSize, Params& params )
        : m_mmu(mmu), m_physMemMgr(physMemMgr), m_pid(pid), m_ppid(0), m_pgid(pid), m_tid(pid), m_elfInfo(elfInfo), m_pageSize(pageSize), m_params(params), m_tidAddress(0)
    {
        uint64_t initial_brk = 0;
        char buffer[100];
        snprintf(buffer,100,"@t::ProcessInfo::@p():@l ");
        m_dbg.init( buffer, debug_level, 0,Output::STDOUT );

        m_pageShift = log2(m_pageSize);

        m_futex = new Futex;
        m_threadGrp = new ThreadGrp( this );
        m_virtMemMap = new VirtMemMap;
        m_fileTable = new FileDescriptorTable( 1024 ); 

        for ( size_t i = 0; i < m_elfInfo->countProgramHeaders(); ++i ) {

            const VanadisELFProgramHeaderEntry* hdr = m_elfInfo->getProgramHeader(i);
            if ( PROG_HEADER_LOAD == hdr->getHeaderType() ) {
                uint64_t virtAddr = hdr->getVirtualMemoryStart();
                size_t   memLen = hdr->getHeaderMemoryLength();
                uint64_t virtAddrPage = roundDown( virtAddr, m_pageSize );
                uint64_t virtAddrEnd = roundUp(virtAddr + memLen, m_pageSize );

                m_dbg.verbose( CALL_INFO, 2, 0,"virtAddr %#" PRIx64 ", page aligned virtual address region: %#" PRIx64 " - %#" PRIx64 "\n", 
                        virtAddr, virtAddrPage, virtAddrEnd );
                std::string name;
                if ( m_elfInfo->getEntryPoint() >= virtAddrPage && m_elfInfo->getEntryPoint() < virtAddrEnd ) {
                    name = "text";
                }else{
                    name = "data";
                }

                addMemRegion( name, virtAddrPage, virtAddrEnd-virtAddrPage, hdr->getSegmentFlags(), new MemoryBacking( m_elfInfo ) );

                if ( virtAddrEnd > initial_brk ) {
                    initial_brk = virtAddrEnd;
                }
            }
        }

        //openFileWithFd( "stdin-" + std::to_string(pid), 0 );
        openFileWithFd( "stdout-" + std::to_string(m_pid), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR, 1 );
        openFileWithFd( "stderr-" + std::to_string(m_pid), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR, 2 );
        
        initBrk( initial_brk );

        printf("initial_brk=%" PRIx64 "\n",initial_brk);

        printRegions("after text/bss setup");
    }

    ~ProcessInfo() {
        m_threadGrp->remove( this );
        if ( 0 == numThreads() ) {
            delete m_threadGrp;
        }

        m_virtMemMap->decRefCnt(); 
        if ( 0 == m_virtMemMap->refCnt() ) {
            delete m_virtMemMap;
        }

        m_fileTable->decRefCnt();
        if ( 0 == m_fileTable->refCnt() ) {
            delete m_fileTable;
        }
    }

    void decRefCnts() { 
        m_fileTable->decRefCnt();
        m_virtMemMap->decRefCnt(); 
    }
    void incRefCnts() { 
        m_fileTable->incRefCnt();
        m_virtMemMap->incRefCnt(); 
    }

    void setTidAddress(  uint64_t addr ) {
        m_tidAddress = addr;
    }
    uint64_t getTidAddress( ) {
        return m_tidAddress;
    }

    unsigned numThreads() {
        return m_threadGrp->size();
    }

    ProcessInfo* getThread() {
        return m_threadGrp->getThread( this );
    }

    void addThread(ProcessInfo* thread ) {
        m_threadGrp->add( thread );
    }

    void addMemRegion( std::string name, uint64_t start, size_t length, uint32_t perms, MemoryBacking* backing = nullptr ) {
        m_virtMemMap->addRegion( name, start, length, perms, backing );
    }
   
    MemoryRegion* findMemRegion( uint64_t addr ) {
        return m_virtMemMap->findRegion( addr );
    }

    void addFutexWait( uint64_t addr, VanadisSyscall* syscall ) {
        m_dbg.verbose(CALL_INFO,1,0,"addr=%#" PRIx64 "\n",addr);
        m_futex->addWait( addr, syscall );
    }

    VanadisSyscall* findFutex( uint64_t addr ) {
        m_dbg.verbose(CALL_INFO,1,0,"addr=%#" PRIx64 "\n",addr);
        return m_futex->findWait( addr );
    }

    int futexGetNumWaiters( uint64_t addr ) {
        m_dbg.verbose(CALL_INFO,1,0,"addr=%#" PRIx64 "\n",addr);
        return m_futex->getNumWaiters( addr );
    }

    void mapVirtToPage( unsigned vpn, OS::Page* page ) {
        m_dbg.verbose(CALL_INFO,1,0,"vpn=%d ppn=%d virtAddr=%#" PRIx64 "\n", vpn, page->getPPN(), (uint64_t) vpn << m_pageShift );
        auto region = findMemRegion( vpn << m_pageShift );
        assert( region );
        region->mapVirtToPhys( vpn, page );
    }

    uint64_t virtToPhys( uint64_t virtAddr) {
        uint32_t vpn = virtAddr >> m_pageShift;

        auto region = findMemRegion(virtAddr);
        if ( nullptr == region ) {
            m_dbg.fatal(CALL_INFO, -1, "Error: can't find memory region for addr %#" PRIx64 "\n", virtAddr);
        }
        
        uint32_t ppn = m_mmu->virtToPhys( getpid(), vpn );
        if ( -1 == ppn ) {
            assert(0);
        }
        uint64_t physAddr = ppn << m_pageShift | virtAddr & ( (1<<m_pageShift) - 1 );
        m_dbg.verbose(CALL_INFO,1,0,"pid=%d virtAddr=%#" PRIx64 " -> %#" PRIx64 "\n",getpid(),virtAddr,physAddr);
        return physAddr;
    }

    uint64_t getBrk() { 
        return m_virtMemMap->getBrk();
    }

    bool setBrk( uint64_t brk ) { 
        return m_virtMemMap->setBrk( roundUp( brk, m_pageSize ) );
    }

    void initBrk( uint64_t addr ) { 
        m_virtMemMap->initBrk( addr );
    }

    void printRegions(std::string msg) {
        m_virtMemMap->print( msg );
    }

    void mprotect( uint64_t addr, size_t length, int prot ) {

        m_virtMemMap->mprotect( addr, length, prot ); 
        m_virtMemMap->print( "mprotect" );
    }

    uint64_t mmap( uint64_t addr, size_t length, int prot, int flags, size_t offset ) {
        m_dbg.verbose(CALL_INFO,1,0,"addr=%#" PRIx64 " length=%zu prot=%#x flags=%#x offset=%zu\n",addr, length, prot, flags, offset );

        
        length = roundUp( length, m_pageSize );

        uint32_t perms = 0;
        if ( prot & PROT_WRITE ) { perms |= 1<<1;}
        if ( prot & PROT_READ ) { perms |= 1<<2;}

        uint64_t ret;
        if ( ( ret = m_virtMemMap->mmap( length, perms ) ) ) {
            m_virtMemMap->print( "mmapped" );
            return ret; 
        }
        
        m_dbg.verbose(CALL_INFO,0,0,"didn't find enough memory\n");
        assert(0);
        return 0;
    }
    
    int unmap( uint64_t addr, size_t length ) {
        if ( 0 == length ) return EINVAL;
        if ( addr & ( 1 << m_pageShift ) - 1 ) return EINVAL;

        return m_virtMemMap->unmap( addr, length );
    }

    void setHwThread( OS::HwThreadID& id ) {
        m_core = id.core;
        m_hwThread = id.hwThread;
    } 

    void settid( unsigned id ) { m_tid = id; }
    void setppid( unsigned id ) { m_ppid = id; }
    void setpgid( unsigned id ) { m_pgid = id; }

    unsigned getCore() { return m_core; }
    unsigned getHwThread() { return m_hwThread; }
    unsigned getpid() { return m_pid; }
    unsigned gettid() { return m_tid; }
    unsigned getppid() { return m_ppid; }
    unsigned getpgid() { return m_pgid; }

    int closeFile( int fd ) {
        return m_fileTable->close( fd );
    }

    int openFile(std::string file_path, int flags, mode_t mode){
        return m_fileTable->open( file_path, flags, mode );
    }

    int openFile(std::string file_path, int dirfd, int flags, mode_t mode) {
        return m_fileTable->openat( file_path, dirfd, flags, mode );
    }

    int getFileDescriptor( uint32_t handle ) {
        return m_fileTable->getDescriptor( handle );
    } 

    std::string getFilePath( uint32_t handle ) {
        return m_fileTable->getPath( handle ); 
    }

    Params& getParams() { return m_params; }
    uint64_t getEntryPoint() { return m_elfInfo->getEntryPoint(); }
    VanadisELFInfo* getElfInfo() { return m_elfInfo; }
    bool isELF32() { return m_elfInfo->isELF32();}

  private:  

    void openFileWithFd(std::string file_path, int flags, int mode, int fd ) {
        assert( fd == m_fileTable->open( file_path, flags, mode, fd ) );
    }

    size_t roundDown( size_t num, size_t step ) {
        return num & ~( step - 1 );
    }

    size_t roundUp( size_t num, size_t step ) {
        if ( num & ( step - 1 ) ) {
            return roundDown( num, step) + step;
        } else {
            return num;
        }
    }

    Output m_dbg;
    SST::Params m_params;
    unsigned m_pageShift;
    unsigned m_pageSize;
    unsigned m_pid;
    unsigned m_tid;
    unsigned m_ppid;
    unsigned m_pgid;
    unsigned m_core;
    unsigned m_hwThread;
    uint64_t m_tidAddress;

    MMU_Lib::MMU*           m_mmu;
    PhysMemManager*         m_physMemMgr;
    VanadisELFInfo*         m_elfInfo;

    VirtMemMap*             m_virtMemMap;
    FileDescriptorTable*    m_fileTable;
    ThreadGrp*              m_threadGrp;
    Futex*                  m_futex;
};

}
}
}

#endif
