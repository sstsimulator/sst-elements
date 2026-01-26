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

    ProcessInfo( ) : process_id_(-1), file_table_(nullptr), virtual_memory_map_(nullptr), thread_id_address_(0) { }

    ProcessInfo( const ProcessInfo& obj, unsigned pid, unsigned debug_level = 0 ) : process_id_(pid), thread_id_(pid), user_id_(8000), group_id_(1000), thread_id_address_(0) {

        char buffer[100];
        snprintf(buffer,100,"@t::ProcessInfo::@p():@l ");
        dbg_.init( buffer, debug_level, 0,Output::STDOUT );

        mmu_ = obj.mmu_;
        physical_memory_mgr_ = obj.physical_memory_mgr_;
        elf_info_ = obj.elf_info_;
        page_size_ = obj.page_size_;
        params_ = obj.params_;
        page_shift_ = obj.page_shift_;
        parent_pid_ = obj.process_id_;
        process_group_id_ = obj.process_group_id_;
        cpu_mask_ = obj.cpu_mask_;
        num_logical_cores_ = obj.num_logical_cores_;

        futex_ = new Futex;
        thread_group_ = new ThreadGrp();
        thread_group_->add( this, gettid() );
        virtual_memory_map_ = new VirtMemMap( *obj.virtual_memory_map_ );
        file_table_ = new FileDescriptorTable( 1024 );

        //openFileWithFd( "stdin-" + std::to_string(pid), 0 );
        openFileWithFd( "stdout-" + std::to_string(process_id_), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR, 1 );
        openFileWithFd( "stderr-" + std::to_string(process_id_), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR, 2 );

        file_table_->update( obj.file_table_ );
    }

    ProcessInfo( MMU_Lib::MMU* mmu, PhysMemManager* phys_mem_mgr, int node, unsigned pid, VanadisELFInfo* elf_info, int debug_level, unsigned page_size, uint32_t num_logical_cores, Params& params )
        : mmu_(mmu), physical_memory_mgr_(phys_mem_mgr), process_id_(pid), parent_pid_(0), process_group_id_(pid), thread_id_(pid), user_id_(8000), group_id_(1000),
          elf_info_(elf_info), page_size_(page_size), num_logical_cores_(num_logical_cores), params_(params), thread_id_address_(0)
    {
        uint64_t initial_brk = 0;
        char buffer[100];
        snprintf(buffer,100,"@t::ProcessInfo::@p():@l ");
        dbg_.init( buffer, debug_level, 0,Output::STDOUT );

        page_shift_ = log2(page_size_);

        futex_ = new Futex;
        thread_group_ = new ThreadGrp();
        thread_group_->add( this, gettid() );
        virtual_memory_map_ = new VirtMemMap;
        file_table_ = new FileDescriptorTable( 1024 );

        cpu_mask_.resize( 128, 0 );
        for ( size_t i = 0; i < num_logical_cores_; ++i ) {
            setLogicalCoreAffinity( i );
        }

        for ( size_t i = 0; i < elf_info_->countProgramHeaders(); ++i ) {

            const VanadisELFProgramHeaderEntry* hdr = elf_info_->getProgramHeader(i);
            if ( PROG_HEADER_LOAD == hdr->getHeaderType() ) {
                uint64_t virt_addr = hdr->getVirtualMemoryStart();
                size_t   mem_len = hdr->getHeaderMemoryLength();
                uint64_t virt_addr_page = roundDown( virt_addr, page_size_ );
                uint64_t virt_addr_end = roundUp(virt_addr + mem_len, page_size_ );

                dbg_.verbose( CALL_INFO, 2, 0,"virtAddr %#" PRIx64 ", page aligned virtual address region: %#" PRIx64 " - %#" PRIx64 " flags=%#" PRIx64 "\n",
                        virt_addr, virt_addr_page, virt_addr_end, hdr->getSegmentFlags() );
                std::string name;
                if ( elf_info_->getEntryPoint() >= virt_addr_page && elf_info_->getEntryPoint() < virt_addr_end ) {
                    name = "text";
                }else{
                    name = "data";
                }

                addMemRegion( name, virt_addr_page, virt_addr_end-virt_addr_page, hdr->getSegmentFlags(), new MemoryBacking( elf_info_ ) );

                if ( virt_addr_end > initial_brk ) {
                    initial_brk = virt_addr_end;
                }
            }
        }

        //openFileWithFd( "stdin-" + std::to_string(pid), 0 );
        if ( node >= 0 ) {
            openFileWithFd( "stdout-" + std::to_string(node) + "-" + std::to_string(process_id_), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR, 1 );
            openFileWithFd( "stderr-" + std::to_string(node) + "-" + std::to_string(process_id_), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR, 2 );
        } else {
            openFileWithFd( "stdout-" + std::to_string(process_id_), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR, 1 );
            openFileWithFd( "stderr-" + std::to_string(process_id_), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR, 2 );
        }

        initBrk( initial_brk );

        printRegions("after text/bss setup");
    }

    ProcessInfo( SST::Output* output, std::string checkpoint_dir,
        MMU_Lib::MMU* mmu, PhysMemManager* phys_mem_mgr, int node, unsigned pid, VanadisELFInfo* elf_info, int debug_level, unsigned page_size , uint32_t num_logical_cores)
        : mmu_(mmu), physical_memory_mgr_(phys_mem_mgr), process_id_(pid), process_group_id_(pid), thread_id_(pid), elf_info_(elf_info), page_size_(page_size), num_logical_cores_(num_logical_cores)
    {
        std::stringstream filename;
        filename << checkpoint_dir << "/process-"  << getpid();

        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"Checkpoint load process %" PRIu32 " %s\n",getpid(), filename.str().c_str());

        auto fp = fopen(filename.str().c_str(),"r");
        assert(fp);

        page_shift_ = log2(page_size_);

        uint32_t val;
        assert( 1 == fscanf(fp,"page_size_: %" SCNu32 "\n",&val) );
        assert( val == page_size_ );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"page_size_: %" PRIu32 "\n",page_size_);

        assert( 1 == fscanf(fp,"process_id_: %" SCNu32 "\n",&val) );
        assert( val == process_id_ );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"process_id_: %" PRIu32 "\n",process_id_);

        assert( 1 == fscanf(fp,"thread_id_: %" SCNu32 "\n",&val) );
        assert( val == thread_id_ );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"thread_id_: %" PRIu32 "\n",thread_id_);

        assert( 1 == fscanf(fp,"parent_pid_: %" SCNu32 "\n", &parent_pid_) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"parent_pid_: %" PRIu32 "\n", parent_pid_);

        assert( 1 == fscanf(fp,"process_group_id_: %" SCNu32 "\n", &val) );
        assert( val == process_group_id_ );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"process_group_id_: %" PRIu32 "\n", process_group_id_);

        assert( 1 == fscanf(fp,"user_id_: %" SCNu32 "\n",&user_id_) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"user_id_: %" PRIu32 "\n",user_id_);

        assert( 1 == fscanf(fp,"group_id_: %" SCNu32 "\n",&group_id_) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"group_id_: %" PRIu32 "\n",group_id_);

        assert( 1 == fscanf(fp,"core_: %" SCNu32 "\n",&core_) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"core_: %" PRIu32 "\n",core_);

        assert( 1 == fscanf(fp,"hw_thread_: %" SCNu32 "\n",&hw_thread_) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"hw_thread_: %" PRIu32 "\n",hw_thread_);

        assert( 1 == fscanf(fp,"thread_id_address: %" SCNx64 "\n",&thread_id_address_) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"thread_id_address: %#" PRIx64 "\n",thread_id_address_);

        virtual_memory_map_ = new VirtMemMap(output,fp,phys_mem_mgr,elf_info);
        file_table_ = new FileDescriptorTable(output,fp);

        thread_group_ = new ThreadGrp;
        futex_ = new Futex;

        cpu_mask_.resize( 128, 0 );
        for ( size_t i = 0; i < num_logical_cores_; ++i ) {
            setLogicalCoreAffinity( i );
        }

        size_t size;
        assert( 1 == fscanf(fp,"params_.size() %zu\n",&size) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"params_.size() %zu\n",size);

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
            params_.insert(key.substr(pos1).c_str(),value.substr(pos2).c_str());
        }
    #if 0
        params_.print_all_params( std::cout );
    #endif
    }

    ~ProcessInfo() {
        thread_group_->remove( this->gettid() );
        if ( 0 == numThreads() ) {
            delete thread_group_;
        }

        virtual_memory_map_->decRefCnt();
        if ( 0 == virtual_memory_map_->refCnt() ) {
            delete virtual_memory_map_;
        }

        file_table_->decRefCnt();
        if ( 0 == file_table_->refCnt() ) {
            delete file_table_;
        }
    }

    void checkpoint( SST::Output* output, std::string checkpointDir ) {
        std::stringstream filename;
        filename << checkpointDir << "/process-"  << getpid();

        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"dump process %" PRIu32 " %s\n",getpid(), filename.str().c_str());

        auto fp = fopen(filename.str().c_str(),"w+");
        assert(fp);
        fprintf(fp,"page_size_: %" PRIu32 "\n",page_size_);
        fprintf(fp,"process_id_: %" PRIu32 "\n",process_id_);
        fprintf(fp,"thread_id_: %" PRIu32 "\n",thread_id_);
        fprintf(fp,"parent_pid_: %" PRIu32 "\n", parent_pid_);
        fprintf(fp,"process_group_id_: %" PRIu32 "\n", process_group_id_);
        fprintf(fp,"user_id_: %" PRIu32 "\n",user_id_);
        fprintf(fp,"group_id_: %" PRIu32 "\n",group_id_);
        fprintf(fp,"core_: %" PRIu32 "\n",core_);
        fprintf(fp,"hw_thread_: %" PRIu32 "\n",hw_thread_);
        fprintf(fp,"thread_id_address_: %#" PRIx64 "\n",thread_id_address_);

        virtual_memory_map_->snapshot(fp);
        file_table_->checkpoint(fp);

        #if 0
        fprintf(fp,"#ThreadGrp start\n");

        auto tmp = thread_group_->getThreadList();

        fprintf(fp,"m_group.size(): %zu\n",tmp.size());
        for ( auto & x : tmp ) {
            fprintf(fp,"tid: %d\n", x.first );
        }
        fprintf(fp,"#ThreadGrp end\n");
        #endif

        assert( futex_->isEmpty() );

        std::stringstream ss;

        fprintf(fp,"params_.size() %zu\n",params_.size());
        params_.print_all_params( ss );
        fprintf(fp,"%s",ss.str().c_str());
    }

    void decRefCnts() {
        file_table_->decRefCnt();
        virtual_memory_map_->decRefCnt();
    }
    void incRefCnts() {
        file_table_->incRefCnt();
        virtual_memory_map_->incRefCnt();
    }

    void setTidAddress(  uint64_t addr ) {
        thread_id_address_ = addr;
    }

    uint64_t getTidAddress( ) {
        return thread_id_address_;
    }

    unsigned numThreads() {
        return thread_group_->size();
    }

    void removeThread( int tid ) {
        thread_group_->remove(tid);
    }

    std::map<int,ProcessInfo*>& getThreadList() {
        return thread_group_->getThreadList();
    }

    void addThread(ProcessInfo* thread ) {
        thread_group_->add( thread, thread->gettid() );
    }

    void addMemRegion( std::string name, uint64_t start, size_t length, uint32_t perms, MemoryBacking* backing = nullptr ) {
        virtual_memory_map_->addRegion( name, start, length, perms, backing );
    }

    MemoryRegion* findMemRegion( uint64_t addr ) {
        return virtual_memory_map_->findRegion( addr );
    }

    MemoryRegion* findMemRegion( std::string name ) {
        return virtual_memory_map_->findRegion( name );
    }


    void addFutexWait( uint64_t addr, VanadisSyscall* syscall ) {
        dbg_.verbose(CALL_INFO,1,0,"addr=%#" PRIx64 "\n",addr);
        futex_->addWait( addr, syscall );
    }

    VanadisSyscall* findFutex( uint64_t addr ) {
        dbg_.verbose(CALL_INFO,1,0,"addr=%#" PRIx64 "\n",addr);
        return futex_->findWait( addr );
    }

    int futexGetNumWaiters( uint64_t addr ) {
        dbg_.verbose(CALL_INFO,1,0,"addr=%#" PRIx64 "\n",addr);
        return futex_->getNumWaiters( addr );
    }

    void mapVirtToPage( uint32_t vpn, OS::Page* page ) {
        dbg_.verbose(CALL_INFO,1,VANADIS_OS_DBG_VIRT2PHYS,"vpn=%" PRIu32 " ppn=%" PRIu32 " virtAddr=%#" PRIx64 "\n", vpn, page->getPPN(), (uint64_t) vpn << page_shift_ );
        auto region = findMemRegion( vpn << page_shift_ );
        assert( region );
        region->mapVirtToPhys( vpn, page );
    }

    uint64_t virtToPhys( uint64_t virtAddr) {

        uint32_t vpn = virtAddr >> page_shift_;

        auto region = findMemRegion(virtAddr);

        if ( nullptr == region ) {
            dbg_.fatal(CALL_INFO, -1, "Error: can't find memory region for addr %#" PRIx64 "\n", virtAddr);
        }

        uint32_t ppn = mmu_->virtToPhys( getpid(), vpn );

        if ( -1 == ppn ) {

            return -1;
        }
        uint64_t physAddr = ppn << page_shift_ | virtAddr & ( (1<<page_shift_) - 1 );
        dbg_.verbose(CALL_INFO,1,VANADIS_OS_DBG_VIRT2PHYS,"pid=%" PRIu32 " virtAddr=%#" PRIx64 " -> %#" PRIx64 "\n", getpid(),virtAddr, physAddr);
        return physAddr;
    }

    uint64_t getBrk() {
        return virtual_memory_map_->getBrk();
    }

    bool setBrk( uint64_t brk ) {
        return virtual_memory_map_->setBrk( roundUp( brk, page_size_ ) );
    }

    void initBrk( uint64_t addr ) {
        virtual_memory_map_->initBrk( addr );
    }

    void printRegions(std::string msg) {
        virtual_memory_map_->print( msg );
    }

    void mprotect( uint64_t addr, size_t length, int prot ) {

        virtual_memory_map_->mprotect( addr, length, prot );
        virtual_memory_map_->print( "mprotect" );
    }

    uint64_t mmap( uint64_t addr, size_t length, int prot, int flags, Device* dev, size_t offset ) {
        dbg_.verbose(CALL_INFO,1,0,"addr=%#" PRIx64 " length=%zu prot=%#x flags=%#x offset=%zu\n",addr, length, prot, flags, offset );

        length = roundUp( length, page_size_ );

        uint32_t perms = 0;
        if ( prot & PROT_WRITE ) { perms |= 1<<1;}
        if ( prot & PROT_READ ) { perms |= 1<<2;}

        uint64_t ret;
        if ( ( ret = virtual_memory_map_->mmap( length, perms, dev ) ) ) {
            virtual_memory_map_->print( "mmapped" );
            return ret;
        }

        dbg_.verbose(CALL_INFO,0,0,"didn't find enough memory\n");
        assert(0);
        return 0;
    }

    int unmap( uint64_t addr, size_t length ) {
        if ( 0 == length ) return EINVAL;
        if ( addr & ( 1 << page_shift_ ) - 1 ) return EINVAL;

        return virtual_memory_map_->unmap( addr, length );
    }

    void setHwThread( OS::HwThreadID& id ) {
        core_ = id.core;
        hw_thread_ = id.hw_thread;
    }

    void settid( uint32_t id ) { thread_id_ = id; }
    void setppid( uint32_t id ) { parent_pid_ = id; }
    void setpgid( uint32_t id ) { process_group_id_ = id; }

    uint32_t getCore() { return core_; }
    uint32_t getHwThread() { return hw_thread_; }
    uint32_t getpid() { return process_id_; }
    uint32_t gettid() { return thread_id_; }
    uint32_t getppid() { return parent_pid_; }
    uint32_t getpgid() { return process_group_id_; }
    uint32_t getuid() { return user_id_; }
    uint32_t getgid() { return group_id_; }

    int closeFile( int fd ) {
        return file_table_->close( fd );
    }

    int openFile(std::string file_path, int flags, mode_t mode){
        return file_table_->open( file_path, flags, mode );
    }

    int openFile(std::string file_path, int dirfd, int flags, mode_t mode) {
        return file_table_->openat( file_path, dirfd, flags, mode );
    }

    int getFileDescriptor( uint32_t handle ) {
        return file_table_->getDescriptor( handle );
    }

    std::string getFilePath( uint32_t handle ) {
        return file_table_->getPath( handle );
    }

    void setAffinity(const std::vector<uint8_t>& mask) { cpu_mask_ = mask; }
    std::vector<uint8_t> getAffinity() const { return cpu_mask_; }

    void setLogicalCoreAffinity(uint32_t core) {
        cpu_mask_[(core / 8)] |= (1 << (core % 8));
    }

    bool getLogicalCoreAffinity(uint32_t core) {
        return (cpu_mask_[core / 8] & (1 << (core % 8))) != 0;
    }

    Params& getParams() { return params_; }
    uint64_t getEntryPoint() { return elf_info_->getEntryPoint(); }
    VanadisELFInfo* getElfInfo() { return elf_info_; }
    bool isELF32() { return elf_info_->isELF32();}

  private:

    void openFileWithFd(std::string file_path, int flags, int mode, int fd ) {
        assert( fd == file_table_->open( file_path, flags, mode, fd ) );
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

    Output dbg_;
    SST::Params params_;
    uint32_t page_shift_;
    uint32_t page_size_;
    uint32_t process_id_;
    uint32_t thread_id_;
    uint32_t parent_pid_;
    uint32_t process_group_id_;
    uint32_t user_id_;
    uint32_t group_id_;
    uint32_t core_;
    uint32_t hw_thread_;
    uint64_t thread_id_address_;

    std::vector<uint8_t> cpu_mask_;
    uint32_t num_logical_cores_;

    MMU_Lib::MMU*           mmu_;
    PhysMemManager*         physical_memory_mgr_;
    VanadisELFInfo*         elf_info_;

    VirtMemMap*             virtual_memory_map_;
    FileDescriptorTable*    file_table_;
    ThreadGrp*              thread_group_;
    Futex*                  futex_;
    SST::Output* output_;
};

}
}
}

#endif
