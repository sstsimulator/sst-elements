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

#ifndef _H_VANADIS_NODE_OS_INCLUDE_PROCESS
#define _H_VANADIS_NODE_OS_INCLUDE_PROCESS

#include <map>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
#include <cstdio>
#include <math.h>
#include <sys/mman.h>
//#include <iostream>
//#include <fstream>

#include <cstdint>

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

    ProcessInfo( const ProcessInfo& obj, unsigned pid, unsigned debug_level = 0 ) : m_pid(pid), m_tid(pid), m_uid(8000), m_gid(1000), m_tidAddress(0) {

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
        m_cpusMask = obj.m_cpusMask;
        m_numLogicalCores = obj.m_numLogicalCores;

        m_futex = new Futex;
        m_threadGrp = new ThreadGrp();
        m_threadGrp->add( this, gettid() );
        m_virtMemMap = new VirtMemMap( *obj.m_virtMemMap );
        m_fileTable = new FileDescriptorTable( 1024 );

        //openFileWithFd( "stdin-" + std::to_string(pid), 0 );
        openFileWithFd( "stdout-" + std::to_string(m_pid), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR, 1 );
        openFileWithFd( "stderr-" + std::to_string(m_pid), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR, 2 );

        m_fileTable->update( obj.m_fileTable );
    }

    ProcessInfo( MMU_Lib::MMU* mmu, PhysMemManager* physMemMgr, int node, unsigned pid, VanadisELFInfo* elfInfo, int debug_level, unsigned pageSize, uint32_t numLogicalCores, Params& params )
        : m_mmu(mmu), m_physMemMgr(physMemMgr), m_pid(pid), m_ppid(0), m_pgid(pid), m_tid(pid), m_uid(8000), m_gid(1000),
          m_elfInfo(elfInfo), m_pageSize(pageSize), m_numLogicalCores(numLogicalCores), m_params(params), m_tidAddress(0)
    {
        uint64_t initial_brk = 0;
        char buffer[100];
        snprintf(buffer,100,"@t::ProcessInfo::@p():@l ");
        m_dbg.init( buffer, debug_level, 0,Output::STDOUT );

        m_pageShift = log2(m_pageSize);

        m_futex = new Futex;
        m_threadGrp = new ThreadGrp();
        m_threadGrp->add( this, gettid() );
        m_virtMemMap = new VirtMemMap;
        m_fileTable = new FileDescriptorTable( 1024 );

        m_cpusMask.resize( 128, 0 );
        for ( size_t i = 0; i < m_numLogicalCores; ++i ) {
            setLogicalCoreAffinity( i );
        }

        for ( size_t i = 0; i < m_elfInfo->countProgramHeaders(); ++i ) {

            const VanadisELFProgramHeaderEntry* hdr = m_elfInfo->getProgramHeader(i);
            if ( PROG_HEADER_LOAD == hdr->getHeaderType() ) {
                uint64_t virtAddr = hdr->getVirtualMemoryStart();
                size_t   memLen = hdr->getHeaderMemoryLength();
                uint64_t virtAddrPage = roundDown( virtAddr, m_pageSize );
                uint64_t virtAddrEnd = roundUp(virtAddr + memLen, m_pageSize );

                m_dbg.verbose( CALL_INFO, 2, 0,"virtAddr %#" PRIx64 ", page aligned virtual address region: %#" PRIx64 " - %#" PRIx64 " flags=%#" PRIx64 "\n",
                        virtAddr, virtAddrPage, virtAddrEnd, hdr->getSegmentFlags() );
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
        if ( node >= 0 ) {
            openFileWithFd( "stdout-" + std::to_string(node) + "-" + std::to_string(m_pid), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR, 1 );
            openFileWithFd( "stderr-" + std::to_string(node) + "-" + std::to_string(m_pid), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR, 2 );
        } else {
            openFileWithFd( "stdout-" + std::to_string(m_pid), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR, 1 );
            openFileWithFd( "stderr-" + std::to_string(m_pid), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR, 2 );
        }

        initBrk( initial_brk );

        printRegions("after text/bss setup");
    }

    ProcessInfo( SST::Output* output, std::string checkpointDir,
        MMU_Lib::MMU* mmu, PhysMemManager* physMemMgr, int node, unsigned pid, VanadisELFInfo* elfInfo, int debug_level, unsigned pageSize , uint32_t numLogicalCores)
        : m_mmu(mmu), m_physMemMgr(physMemMgr), m_pid(pid), m_pgid(pid), m_tid(pid), m_elfInfo(elfInfo), m_pageSize(pageSize), m_numLogicalCores(numLogicalCores)
    {
        std::stringstream filename;
        filename << checkpointDir << "/process-"  << getpid();

        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"Checkpoint load process %d %s\n",getpid(), filename.str().c_str());

        auto fp = fopen(filename.str().c_str(),"r");
        assert(fp);

        m_pageShift = log2(m_pageSize);

        int val;
        assert( 1 == fscanf(fp,"m_pageSize: %d\n",&val) );
        assert( val == m_pageSize );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_pageSize: %d\n",m_pageSize);

        assert( 1 == fscanf(fp,"m_pid: %d\n",&val) );
        assert( val == m_pid );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_pid: %d\n",m_pid);

        assert( 1 == fscanf(fp,"m_tid: %d\n",&val) );
        assert( val == m_tid );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_tid: %d\n",m_tid);

        assert( 1 == fscanf(fp,"m_ppid: %d\n", &m_ppid) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_ppid: %d\n", m_ppid);

        assert( 1 == fscanf(fp,"m_pgid: %d\n", &val) );
        assert( val == m_pgid );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_pgid: %d\n", m_pgid);

        assert( 1 == fscanf(fp,"m_uid: %d\n",&m_uid) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_uid: %d\n",m_uid);

        assert( 1 == fscanf(fp,"m_gid: %d\n",&m_gid) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_gid: %d\n",m_gid);

        assert( 1 == fscanf(fp,"m_core: %d\n",&m_core) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_core: %d\n",m_core);

        assert( 1 == fscanf(fp,"m_hwThread: %d\n",&m_hwThread) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_hwThread: %d\n",m_hwThread);

        assert( 1 == fscanf(fp,"m_tidAddress: %" PRIx64 "\n",&m_tidAddress) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_tidAddress: %#" PRIx64 "\n",m_tidAddress);

        m_virtMemMap = new VirtMemMap(output,fp,physMemMgr,elfInfo);
        m_fileTable = new FileDescriptorTable(output,fp);

        m_threadGrp = new ThreadGrp;
        m_futex = new Futex;

        m_cpusMask.resize( 128, 0 );
        for ( size_t i = 0; i < m_numLogicalCores; ++i ) {
            setLogicalCoreAffinity( i );
        }

        size_t size;
        assert( 1 == fscanf(fp,"m_params.size() %zu\n",&size) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_params.size() %zu\n",size);

        char* tmp = nullptr;
        size_t num = 0;
        (void) !getline( &tmp, &num, fp );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"%s",tmp);
        assert( 0 == strcmp(tmp,"Local params:\n") );
        free(tmp);

        for ( auto i = 0; i < size; i++ ) {
            char tmp1[80],tmp2[80];
            (void) !fscanf(fp, "%s %s\n", tmp1, tmp2 );
            std::string key = tmp1;
            key.erase( key.size() - 1, 1 );
            std::string value = tmp2;
            auto pos1 = key.find_first_of('=' ) + 1;
            auto pos2 = value.find_first_of('=' ) + 1;
            output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"%s %s\n",key.c_str(),value.c_str());
            m_params.insert(key.substr(pos1).c_str(),value.substr(pos2).c_str());
        }
    #if 0
        m_params.print_all_params( std::cout );
    #endif
    }

    ~ProcessInfo() {
        m_threadGrp->remove( this->gettid() );
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

    void checkpoint( SST::Output* output, std::string checkpointDir ) {
        std::stringstream filename;
        filename << checkpointDir << "/process-"  << getpid();

        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"dump process %d %s\n",getpid(), filename.str().c_str());

        auto fp = fopen(filename.str().c_str(),"w+");
        assert(fp);
        fprintf(fp,"m_pageSize: %d\n",m_pageSize);
        fprintf(fp,"m_pid: %d\n",m_pid);
        fprintf(fp,"m_tid: %d\n",m_tid);
        fprintf(fp,"m_ppid: %d\n", m_ppid);
        fprintf(fp,"m_pgid: %d\n", m_pgid);
        fprintf(fp,"m_uid: %d\n",m_uid);
        fprintf(fp,"m_gid: %d\n",m_gid);
        fprintf(fp,"m_core: %d\n",m_core);
        fprintf(fp,"m_hwThread: %d\n",m_hwThread);
        fprintf(fp,"m_tidAddress: %#" PRIx64 "\n",m_tidAddress);

        m_virtMemMap->snapshot(fp);
        m_fileTable->checkpoint(fp);

        #if 0
        fprintf(fp,"#ThreadGrp start\n");

        auto tmp = m_threadGrp->getThreadList();

        fprintf(fp,"m_group.size(): %zu\n",tmp.size());
        for ( auto & x : tmp ) {
            fprintf(fp,"tid: %d\n", x.first );
        }
        fprintf(fp,"#ThreadGrp end\n");
        #endif

        assert( m_futex->isEmpty() );

        std::stringstream ss;

        fprintf(fp,"m_params.size() %zu\n",m_params.size());
        m_params.print_all_params( ss );
        fprintf(fp,"%s",ss.str().c_str());
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

    void removeThread( int tid ) {
        m_threadGrp->remove(tid);
    }

    std::map<int,ProcessInfo*>& getThreadList() {
        return m_threadGrp->getThreadList();
    }

    void addThread(ProcessInfo* thread ) {
        m_threadGrp->add( thread, thread->gettid() );
    }

    void addMemRegion( std::string name, uint64_t start, size_t length, uint32_t perms, MemoryBacking* backing = nullptr ) {
        m_virtMemMap->addRegion( name, start, length, perms, backing );
    }

    MemoryRegion* findMemRegion( uint64_t addr ) {
        return m_virtMemMap->findRegion( addr );
    }

    MemoryRegion* findMemRegion( std::string name ) {
        return m_virtMemMap->findRegion( name );
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
        m_dbg.verbose(CALL_INFO,1,VANADIS_OS_DBG_VIRT2PHYS,"vpn=%d ppn=%d virtAddr=%#" PRIx64 "\n", vpn, page->getPPN(), (uint64_t) vpn << m_pageShift );
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

            return -1;
        }
        uint64_t physAddr = ppn << m_pageShift | virtAddr & ( (1<<m_pageShift) - 1 );
        m_dbg.verbose(CALL_INFO,1,VANADIS_OS_DBG_VIRT2PHYS,"pid=%d virtAddr=%#" PRIx64 " -> %#" PRIx64 "\n",getpid(),virtAddr,physAddr);
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

    uint64_t mmap( uint64_t addr, size_t length, int prot, int flags, Device* dev, size_t offset ) {
        m_dbg.verbose(CALL_INFO,1,0,"addr=%#" PRIx64 " length=%zu prot=%#x flags=%#x offset=%zu\n",addr, length, prot, flags, offset );

        length = roundUp( length, m_pageSize );

        uint32_t perms = 0;
        if ( prot & PROT_WRITE ) { perms |= 1<<1;}
        if ( prot & PROT_READ ) { perms |= 1<<2;}

        uint64_t ret;
        if ( ( ret = m_virtMemMap->mmap( length, perms, dev ) ) ) {
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
    unsigned getuid() { return m_uid; }
    unsigned getgid() { return m_gid; }

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

    void setAffinity(const std::vector<uint8_t>& mask) { m_cpusMask = mask; }
    std::vector<uint8_t> getAffinity() const { return m_cpusMask; }

    void setLogicalCoreAffinity(unsigned core) {
        m_cpusMask[(core / 8)] |= (1 << (core % 8));
    }

    bool getLogicalCoreAffinity(unsigned core) {
        return (m_cpusMask[core / 8] & (1 << (core % 8))) != 0;
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
    unsigned m_uid;
    unsigned m_gid;
    unsigned m_core;
    unsigned m_hwThread;
    uint64_t m_tidAddress;

    std::vector<uint8_t> m_cpusMask;
    uint32_t m_numLogicalCores;

    MMU_Lib::MMU*           m_mmu;
    PhysMemManager*         m_physMemMgr;
    VanadisELFInfo*         m_elfInfo;

    VirtMemMap*             m_virtMemMap;
    FileDescriptorTable*    m_fileTable;
    ThreadGrp*              m_threadGrp;
    Futex*                  m_futex;
    SST::Output* m_output;
};

}
}
}

#endif
