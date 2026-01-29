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

#ifndef _H_VANADIS_NODE_OS
#define _H_VANADIS_NODE_OS

#include <unordered_set>
#include <queue>

#include <sst/core/component.h>
#include <sst/core/interfaces/stdMem.h>

#include "os/vosDbgFlags.h"
#include "os/include/hwThreadID.h"
#include "os/voscallev.h"
#include "os/vstartthreadreq.h"
#include "os/vappruntimememory.h"
#include "os/vphysmemmanager.h"
#include "os/include/process.h"
#include "os/syscall/fork.h"
#include "os/syscall/clone.h"
#include "os/syscall/exit.h"
#include "os/syscall/syscall.h"


using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisNodeOSComponent : public SST::Component {

public:
    SST_ELI_REGISTER_COMPONENT(VanadisNodeOSComponent,
    #ifdef VANADIS_BUILD_DEBUG
        "vanadisdbg",
    #else
        "vanadis",
    #endif
        "VanadisNodeOS",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Vanadis Generic Operating System Component",
        COMPONENT_CATEGORY_PROCESSOR)

    SST_ELI_DOCUMENT_PARAMS(
        { "cores", "Number of cores that can request OS services via a link.", "1" },
        { "dbgLevel", "Debug level (verbosity) for OS debug output", "0" },
        { "dbgMask", "Mask for debug output", "0" },
        { "node_id", "If specified as > 0, this id will be used to tag stdout/stderr files.", "-1" },
        { "hardwareThreadCount", "Number of hardware threads pert core", "1" },
        { "osStartTimeNano", "'Epoch' time added to simulation time for syscalls like gettimeofday", "1000000000"},
        { "program_header_address", "Program header address", "0x60000000" },
        { "processDebugLevel", "Debug level (verbosity) for process debug output", "0" },
        { "physMemSize", "Size of available physical memory in bytes, with units. Ex: 2GiB", NULL },
        { "page_size", "Size of a page, in bytes", "4096" },
        { "useMMU", "Whether an MMU subcomponent is being used.", "False" },
        { "process%(processnum)d.env_count", "Number of environment variables to pass to the process", "0"},
        { "process%(processnum)d.env%(argnum)d", "Environment variable to pass to the process. Example: 'OMPNUMTHREADS=64'. 'argnum' should be contiguous starting at 0 and ending at env_count-1", ""},
        { "proccess%(processnum)d.exe", "Name of executable, including path", NULL},
        { "process%(processnum)d.argc", "Number of arguments to the executable (including arg0)", "1"},
        { "process%(processnum)d.arg0", "Name of the executable (path not needed)", NULL},
        { "process%(processnum)d.arg%(argnum)d", "Arguments for the executable. Each argument should be specified in a separate parameter and 'argnum' should be contigous starting at 1 to argc-1", ""},
)

    SST_ELI_DOCUMENT_PORTS({ "core%(cores)d", "Connects to a CPU core", {} })

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS({ "mem_interface", "Interface to memory system for data access",
                                          "SST::Interface::StandardMem" })

    VanadisNodeOSComponent(SST::ComponentId_t id, SST::Params& params);
    ~VanadisNodeOSComponent();

private:
    enum PageInitType { Zeros, Random };

    typedef std::function<void()> Callback;

    class PageMemReq {
    public:
        PageMemReq( StandardMem* mem_if, uint64_t addr, size_t length, uint8_t* data, Callback* callback ) :
            mem_if_(mem_if), addr_(addr), length_(length), data_(data), callback_(callback), offset_(0) { }

        virtual ~PageMemReq() {
            (*callback_)();
            delete callback_;
        }
        virtual bool handleResp( StandardMem::Request* ev = nullptr ) = 0;
        virtual void sendReq() = 0;

    protected:
        StandardMem* mem_if_;
        size_t offset_;
        uint64_t addr_;
        size_t length_;
        Callback* callback_;
        std::map<StandardMem::Request::id_t,uint64_t> req_map_;
        uint8_t* data_;
    };

    class PageMemWriteReq : public PageMemReq {
    public:
        PageMemWriteReq( StandardMem* mem_if, uint64_t addr, size_t length, uint8_t* data, Callback* callback ) :
            PageMemReq( mem_if, addr, length, data, callback ) {}

        virtual ~PageMemWriteReq() {
            delete[] data_;
        }

        bool handleResp( StandardMem::Request* ev );
        void sendReq();
    };

    class PageMemReadReq : public PageMemReq {
    public:
        PageMemReadReq( StandardMem* mem_if, uint64_t addr, size_t length, uint8_t* data, Callback* callback ) :
            PageMemReq( mem_if, addr, length, data, callback ), current_req_offset_(0) {}

        virtual ~PageMemReadReq() { }

        bool handleResp( StandardMem::Request* ev );
        void sendReq();
    private:
        size_t current_req_offset_;
    };

    struct PageFault {
        PageFault(MMU_Lib::RequestID req_id, uint32_t link, uint32_t core, uint32_t hw_thread, uint32_t pid,  uint32_t vpn,
                            uint32_t fault_perms, uint64_t inst_ptr, uint64_t mem_virt_addr, VanadisSyscall* syscall )
            : req_id(req_id), link(link), core(core), hw_thread(hw_thread), pid(pid), vpn(vpn), fault_perms(fault_perms),
                inst_ptr(inst_ptr), mem_virt_addr(mem_virt_addr), syscall(syscall) {}
        MMU_Lib::RequestID req_id;
        uint32_t link;
        uint32_t core;
        uint32_t hw_thread;
        uint32_t pid;
        uint32_t vpn;
        uint32_t fault_perms;
        uint64_t inst_ptr;
        uint64_t mem_virt_addr;
        VanadisSyscall* syscall;
    };


    VanadisNodeOSComponent();                              // for serialization only
    VanadisNodeOSComponent(const VanadisNodeOSComponent&); // do not implement
    void operator=(const VanadisNodeOSComponent&);         // do not implement

    virtual void init(unsigned int phase);
    void setup();
    void finish();
    void handleIncomingSyscallEvent(SST::Event* ev);
    VanadisSyscall* handleIncomingSyscall( OS::ProcessInfo*, VanadisSyscallEvent*, SST::Link* core_link );
    void processSyscallPost( VanadisSyscall* syscall );

    void handleIncomingMemoryCallback(StandardMem::Request* ev);

    void processOsPageFault( VanadisSyscall*, uint64_t virt_addr, bool is_write );

    void pageFaultHandler( MMU_Lib::RequestID req_id, uint32_t link, uint32_t core, uint32_t hw_thread, uint32_t pid,
        uint32_t vpn, uint32_t perms, uint64_t inst_ptr, uint64_t mem_virt_addr )
    {
        pageFaultHandler2( req_id, link, core, hw_thread, pid, vpn, perms, inst_ptr, mem_virt_addr );
    }

    void pageFaultHandler2( MMU_Lib::RequestID req_id, uint32_t link, uint32_t core, uint32_t hw_thread,  uint32_t pid,
        uint32_t vpn, uint32_t perms, uint64_t inst_ptr, uint64_t mem_virt_addr, VanadisSyscall* syscall = nullptr );

    void pageFault( PageFault* fault);
    void pageFaultFini( PageFault* fault, bool success = true );
    void startProcess( OS::HwThreadID& thread, OS::ProcessInfo* process );
    void copyPage(uint64_t phys_from, uint64_t phy_tTo, uint32_t page_size, Callback* callback);

    void sendMemoryEvent(VanadisSyscall* syscall, StandardMem::Request* ev ) {
        mem_resp_map_.insert(std::pair<StandardMem::Request::id_t, VanadisSyscall*>(ev->getID(), syscall));
        mem_if_->send(ev);
    }

    uint64_t getNanoSeconds() { return getCurrentSimTimeNano() + os_start_time_nano_;  }

    void writePage( uint64_t phys_addr, uint8_t* data, uint32_t page_size, Callback* callback )
    {
        queueBlockMemoryReq( new PageMemWriteReq( mem_if_, phys_addr, page_size, data, callback ) );
    }

    void readPage( uint64_t phys_addr, uint8_t* data, uint32_t page_size, Callback* callback )
    {
        queueBlockMemoryReq( new PageMemReadReq( mem_if_, phys_addr, page_size, data, callback ) );
    }

    void queueBlockMemoryReq( PageMemReq* req ) {
        block_memory_write_req_queue_.push( req );
        if ( 1 == block_memory_write_req_queue_.size() ) {
            startBlockXfer( block_memory_write_req_queue_.front() );
        }
    }

    void startBlockXfer( PageMemReq* req ) {
        // this specfies how many requests should be initially sent before waiting for a response
        // this value was choose because higher does not increase performance, for the configuration used to test
        unsigned start_with_num = 6;
        for ( unsigned i = 0; i < start_with_num; i++ ) {
            req->sendReq();
        }
    }

    void handleIncomingMemory( VanadisSyscall* syscall, StandardMem::Request* req ) {

        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL,"handleIncomingMemory\n");
        #endif
        syscall->handleMemRespBase( req );

        processSyscallPost( syscall );
    }

    OS::Page* checkPageCache( VanadisELFInfo* elf_info , int vpn ) {
        auto iter = elf_page_cache_.find( elf_info );
        if ( iter != elf_page_cache_.end() ) {
            auto tmp = iter->second;
            auto iter2 = tmp.find(vpn);
            if ( iter2 != tmp.end() ) {
                return iter2->second;
            }
        }
        return nullptr;
    }

    void updatePageCache( VanadisELFInfo* elf_info , uint32_t vpn, OS::Page* page ) {
        elf_page_cache_[elf_info][vpn] = page;
    }

    void writeMem( OS::ProcessInfo*, uint64_t virt_addr, std::vector<uint8_t>* data, uint32_t perms, uint32_t page_size, Callback* callback );

    template<typename T>
    T convertEvent( std::string name, VanadisSyscallEvent* sys_ev ) {
        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL, "-> call is %s()\n",name.c_str());
        #endif
        T out = dynamic_cast<T>(sys_ev);

        if (nullptr == out ) {
            output_->fatal(CALL_INFO, -1, "-> error: unable to case syscall to a %s event.\n",name.c_str());
        }
        return out;
    }

    class HardwareThreadInfo {
      public:
        HardwareThreadInfo( ) : process_info_(nullptr), syscall_(nullptr) {}
        void setProcess( OS::ProcessInfo* process ) { process_info_ = process; }
        void setSyscall( VanadisSyscall* syscall ) { assert( nullptr == syscall_ ); syscall_ = syscall; }
        void clearSyscall( ) { assert(syscall_); syscall_ = nullptr; }
        OS::ProcessInfo* getProcess() { return process_info_; }
        VanadisSyscall* getSyscall() { return syscall_; }
        void checkpoint( FILE* fp ) {
            if ( process_info_ ) {
                fprintf(fp,"pid,tid: %" PRIu32 ",%" PRIu32 "\n",process_info_->getpid(), process_info_->gettid());
            } else {
                fprintf(fp,"pid,tid: -1,-1\n");
            }
            assert( nullptr == syscall_ );
        }
      private:
        OS::ProcessInfo* process_info_;
        VanadisSyscall* syscall_;
    };

    class CoreInfo {
      public:
        CoreInfo( uint32_t num_hw_threads ) : hw_thread_map_(num_hw_threads) {}
        void setSyscall( uint32_t hw_thread,  VanadisSyscall* syscall ) { hw_thread_map_[hw_thread].setSyscall( syscall ); }
        void clearSyscall( uint32_t hw_thread ) { hw_thread_map_[hw_thread].clearSyscall(); }
        void setProcess( uint32_t hw_thread,  OS::ProcessInfo* process ) { hw_thread_map_[hw_thread].setProcess( process ); }

        OS::ProcessInfo* getProcess( uint32_t hw_thread ) { return hw_thread_map_.at(hw_thread).getProcess(); }
        VanadisSyscall* getSyscall( uint32_t hw_thread ) { return hw_thread_map_.at(hw_thread).getSyscall(); }

        void checkpoint( FILE* fp ) {
            fprintf(fp, "m_hwThreadMap.size(): %zu\n",hw_thread_map_.size());
            for ( auto i = 0; i < hw_thread_map_.size(); i++ ) {
                fprintf(fp, "hwThread: %" PRIu32 "\n",i);
                hw_thread_map_[i].checkpoint( fp );
            }
        }
        void checkpointLoad( FILE* fp ) {
        }
      private:
        std::vector< HardwareThreadInfo > hw_thread_map_;
    };

    VanadisSyscall* getSyscall( uint32_t core, uint32_t hw_thread ) {
        return core_info_[core].getSyscall( hw_thread );
    }

public:

    void sendEvent( uint32_t core, SST::Event* ev ) {
        core_links_.at(core)->send( ev );
    }

    void setProcess( uint32_t core, uint32_t hw_thread,  OS::ProcessInfo* process ) {
        core_info_.at(core).setProcess( hw_thread, process );
    }

    Output* getOutput() { return output_; }

    void setSyscall( uint32_t core, uint32_t hw_thread, VanadisSyscall* syscall) {
        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL,"core=%" PRIu32 " hwThread=%" PRIu32 "\n", core, hw_thread);
        #endif
        core_info_[core].setSyscall( hw_thread, syscall );
    }
    void clearSyscall( uint32_t core, uint32_t hw_thread ) {
        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL,"core=%" PRIu32 " hwThread=%" PRIu32 "\n", core, hw_thread );
        #endif
        core_info_[core].clearSyscall( hw_thread );
    }

    OS::HwThreadID* allocHwThread() {
        if ( avail_hw_threads_.empty() ) {
            return nullptr;
        }

        OS::HwThreadID* thread_id = avail_hw_threads_.front();
        avail_hw_threads_.pop();

        return thread_id;
    }

    void removeThread( unsigned core, unsigned hw_thread, unsigned tid ) {

        auto iter = thread_map_.find( tid );
        assert( iter != thread_map_.end() );
        thread_map_.erase( iter );

        getMMU()->flushTlb( core, hw_thread );

        // clear the process/thread to hwThread map
        core_info_.at( core ).setProcess( hw_thread, nullptr );

        // we are puting this hwThread back into avail pool
        // do we need to worry about reallocating it before it gets the halt message, probably not, should put a check in the core to see if idle?
        avail_hw_threads_.push( new OS::HwThreadID( core, hw_thread ) );

        if ( thread_map_.empty() ) {
            printf("all processes have exited\n");
            primaryComponentOKToEndSim();
        }
    }

    uint32_t getNewTid() { return current_tid_++; }
    MMU_Lib::MMU* getMMU() { return mmu_; };

    void setThread( int tid, OS::ProcessInfo* thread ) {
        thread_map_[tid] = thread;
    }

    OS::Device* getDevice( int id ) {
    	assert( device_list_.find(id) != device_list_.end() );
	return device_list_[id];
    }

    int getNodeNum() { return node_num_; }
    uint32_t getPageSize() { return page_size_; }
    uint32_t getPageShift() { return page_shift_; }

    uint32_t getNumCores() { return core_count_; }
    uint32_t getNumHwThreads() { return hardware_thread_count_; }


private:

    SST::Output*                output_;
    std::vector<SST::Link*>     core_links_;
    std::vector< CoreInfo >     core_info_;
    MMU_Lib::MMU*               mmu_;
    StandardMem*                mem_if_;
    PhysMemManager*             phys_mem_mgr_;
    AppRuntimeMemoryMod*        app_runtime_memory_;
    uint32_t                    process_debug_level_;
    uint32_t                    page_size_;
    uint32_t                    page_shift_;
    uint64_t                    phdr_address_;
    uint64_t                    stack_top_;
    int                         node_num_;
    uint64_t                    os_start_time_nano_;
    uint32_t                    core_count_;
    uint32_t                    hardware_thread_count_;
    uint32_t                    logical_core_count_;

    std::queue<PageFault*>                          pending_fault_;
    std::map<std::string, VanadisELFInfo* >         elf_map_;
    std::unordered_map<uint32_t,OS::ProcessInfo*>   thread_map_;
    std::queue<PageMemReq*>                         block_memory_write_req_queue_;

    std::map< VanadisELFInfo*, std::map<int,OS::Page*> >            elf_page_cache_;
    std::unordered_map<StandardMem::Request::id_t, VanadisSyscall*> mem_resp_map_;

    std::queue< OS::HwThreadID* > avail_hw_threads_;

    std::map< int, OS::Device* > device_list_;

    uint32_t current_tid_;

    OS::Page* allocPage() {
        auto page = new OS::Page(phys_mem_mgr_);
        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"ppn=%" PRIu32 "\n",page->getPPN());
        #endif
        return page;
    }

    std::string checkpoint_dir_;
    enum { NO_CHECKPOINT, CHECKPOINT_LOAD, CHECKPOINT_SAVE }  enable_checkpoint_;

    void checkpoint( std::string dir );
    int checkpointLoad( std::string dir );
    std::deque<uint64_t> flush_pages_;
};

} // namespace Vanadis
} // namespace SST

#endif
