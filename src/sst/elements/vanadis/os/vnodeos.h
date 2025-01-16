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
    SST_ELI_REGISTER_COMPONENT(VanadisNodeOSComponent, "vanadis", "VanadisNodeOS", SST_ELI_ELEMENT_VERSION(1, 0, 0),
                               "Vanadis Generic Operating System Component", COMPONENT_CATEGORY_PROCESSOR)

    SST_ELI_DOCUMENT_PARAMS({ "cores", "Number of cores that can request OS services via a link.", NULL },
                            { "dbgLevel", "Debug level (verbosity) for OS debug output", "0" }, 
                            { "dbgMask", "Mask for debug output", "0" }, 
                            { "node_id", "If specificied as > 0, this id will be used to tag stdout/stderr files.", "-1" },
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
            mem_if(mem_if), addr(addr), length(length), data(data), callback(callback), offset(0) { } 
        virtual ~PageMemReq() {
            (*callback)();
            delete callback;
        }
        virtual bool handleResp( StandardMem::Request* ev = nullptr ) = 0;
        virtual void sendReq() = 0;
      protected:
        StandardMem* mem_if;
        size_t offset;
        uint64_t addr;
        size_t length;
        Callback* callback;
        std::map<StandardMem::Request::id_t,uint64_t> reqMap;
        uint8_t* data;
    };

    class PageMemWriteReq : public PageMemReq {
      public:
        PageMemWriteReq( StandardMem* mem_if, uint64_t addr, size_t length, uint8_t* data, Callback* callback ) : 
            PageMemReq( mem_if, addr, length, data, callback ) {}

        virtual ~PageMemWriteReq() {
            delete[] data;
        }

        bool handleResp( StandardMem::Request* ev );
        void sendReq();
    };

    class PageMemReadReq : public PageMemReq {
      public:
        PageMemReadReq( StandardMem* mem_if, uint64_t addr, size_t length, uint8_t* data, Callback* callback ) : 
            PageMemReq( mem_if, addr, length, data, callback ), m_currentReqOffset(0) {}

        virtual ~PageMemReadReq() { }

        bool handleResp( StandardMem::Request* ev );
        void sendReq();
      private:
        size_t m_currentReqOffset;
    };

    struct PageFault {
        PageFault(MMU_Lib::RequestID reqId, unsigned link, unsigned core,unsigned hwThread, unsigned pid,  uint32_t vpn,
                            uint32_t faultPerms, uint64_t instPtr, uint64_t memVirtAddr, VanadisSyscall* syscall )
            : reqId(reqId), link(link), core(core), hwThread(hwThread), pid(pid), vpn(vpn), faultPerms(faultPerms),
                instPtr(instPtr), memVirtAddr(memVirtAddr), syscall(syscall) {}
        MMU_Lib::RequestID reqId;
        unsigned link;
        unsigned core;
        unsigned hwThread;
        unsigned pid;
        uint32_t vpn;
        uint32_t faultPerms;
        uint64_t instPtr;
        uint64_t memVirtAddr;
        VanadisSyscall* syscall;
    };


    VanadisNodeOSComponent();                              // for serialization only
    VanadisNodeOSComponent(const VanadisNodeOSComponent&); // do not implement
    void operator=(const VanadisNodeOSComponent&);         // do not implement

    virtual void init(unsigned int phase);
    void setup();
    void finish();
    void handleIncomingSyscall(SST::Event* ev);
    VanadisSyscall* handleIncomingSyscall( OS::ProcessInfo*, VanadisSyscallEvent*, SST::Link* core_link );
    void processSyscallPost( VanadisSyscall* syscall );

    void handleIncomingMemory(StandardMem::Request* ev);

    void processOsPageFault( VanadisSyscall*, uint64_t virtAddr, bool isWrite );

    void pageFaultHandler( MMU_Lib::RequestID reqId, unsigned link, unsigned core, unsigned hwThread, unsigned pid,
        uint32_t vpn, uint32_t perms, uint64_t instPtr, uint64_t memVirtAddr ) 
    {
        pageFaultHandler2( reqId, link, core, hwThread, pid, vpn, perms, instPtr, memVirtAddr ); 
    }

    void pageFaultHandler2( MMU_Lib::RequestID, unsigned link, unsigned core, unsigned hwThread,  unsigned pid,
        uint32_t vpn, uint32_t perms, uint64_t instPtr, uint64_t memVirtAddr, VanadisSyscall* syscall = nullptr );

    void pageFault( PageFault* );
    void pageFaultFini( PageFault*, bool success = true );
    void startProcess( OS::HwThreadID&, OS::ProcessInfo* process );
    void copyPage(uint64_t physFrom, uint64_t physTo, unsigned pageSize, Callback* );

    void sendMemoryEvent(VanadisSyscall* syscall, StandardMem::Request* ev ) {
        m_memRespMap.insert(std::pair<StandardMem::Request::id_t, VanadisSyscall*>(ev->getID(), syscall));
        mem_if->send(ev);
    }

    uint64_t getNanoSeconds() { return getCurrentSimTimeNano() + m_osStartTimeNano;  }

    void writePage( uint64_t physAddr, uint8_t* data, unsigned page_size, Callback* callback )
    {
        queueBlockMemoryReq( new PageMemWriteReq( mem_if, physAddr, page_size, data, callback ) );
    }

    void readPage( uint64_t physAddr, uint8_t* data, unsigned page_size, Callback* callback )
    {
        queueBlockMemoryReq( new PageMemReadReq( mem_if, physAddr, page_size, data, callback ) );
    }

    void queueBlockMemoryReq( PageMemReq* req ) {
        m_blockMemoryWriteReqQ.push( req );
        if ( 1 == m_blockMemoryWriteReqQ.size() ) {
            startBlockXfer( m_blockMemoryWriteReqQ.front() );    
        }
    } 

    void startBlockXfer( PageMemReq* req ) {
        // this specfies how many requests should be initially sent before waiting for a response 
        // this value was choose because hight does not increase performance, for the configuration used to test
        unsigned startWithNum = 6;
        for ( int i = 0; i < startWithNum; i++ ) {  
            req->sendReq();
        }
    }

    void handleIncomingMemory( VanadisSyscall* syscall, StandardMem::Request* req ) {

        output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL,"handleIncomingMemory\n");
        syscall->handleMemRespBase( req );

        processSyscallPost( syscall ); 
    }

    OS::Page* checkPageCache( VanadisELFInfo* elf_info , int vpn ) {
        auto iter = m_elfPageCache.find( elf_info ); 
        if ( iter != m_elfPageCache.end() ) {
            auto tmp = iter->second; 
            auto iter2 = tmp.find(vpn);
            if ( iter2 != tmp.end() ) {
                return iter2->second;
            }
        } 
        return nullptr;
    } 

    void updatePageCache( VanadisELFInfo* elf_info , int vpn, OS::Page* page ) {
        m_elfPageCache[elf_info][vpn] = page;
    } 

    void writeMem( OS::ProcessInfo*, uint64_t virtAddr, std::vector<uint8_t>* data, int perms, unsigned pageSize, Callback* callback );

    template<typename T>
    T convertEvent( std::string name, VanadisSyscallEvent* sys_ev ) {
        output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL, "-> call is %s()\n",name.c_str());
        T out = dynamic_cast<T>(sys_ev);

        if (nullptr == out ) {
            output->fatal(CALL_INFO, -1, "-> error: unable to case syscall to a %s event.\n",name.c_str());
        }
        return out;
    } 

    class HardwareThreadInfo {
      public:
        HardwareThreadInfo( ) : m_processInfo(nullptr), m_syscall(nullptr) {}
        void setProcess( OS::ProcessInfo* process ) { m_processInfo = process; }
        void setSyscall( VanadisSyscall* syscall ) { assert( nullptr == m_syscall ); m_syscall = syscall; }
        void clearSyscall( ) { assert(m_syscall); m_syscall = nullptr; }
        OS::ProcessInfo* getProcess() { return m_processInfo; }
        VanadisSyscall* getSyscall() { return m_syscall; }
        void checkpoint( FILE* fp ) {
            if ( m_processInfo ) {
                fprintf(fp,"pid,tid: %d,%d\n",m_processInfo->getpid(),m_processInfo->gettid());
            } else {
                fprintf(fp,"pid,tid: %d,%d\n",-1,-1);
            }
            assert( nullptr == m_syscall );
        }
      private:
        OS::ProcessInfo* m_processInfo;
        VanadisSyscall* m_syscall;
    };

    class CoreInfo {
      public:
        CoreInfo( unsigned numHwThreads ) : m_hwThreadMap(numHwThreads) {}
        void setSyscall( unsigned hwThread,  VanadisSyscall* syscall ) { m_hwThreadMap[hwThread].setSyscall( syscall ); }
        void clearSyscall( unsigned hwThread ) { m_hwThreadMap[hwThread].clearSyscall(); }
        void setProcess( unsigned hwThread,  OS::ProcessInfo* process ) { m_hwThreadMap[hwThread].setProcess( process ); }

        OS::ProcessInfo* getProcess( unsigned hwThread ) { return m_hwThreadMap.at(hwThread).getProcess(); }
        VanadisSyscall* getSyscall( unsigned hwThread ) { return m_hwThreadMap.at(hwThread).getSyscall(); }

        void checkpoint( FILE* fp ) {
            fprintf(fp, "m_hwThreadMap.size(): %zu\n",m_hwThreadMap.size());
            for ( auto i = 0; i < m_hwThreadMap.size(); i++ ) {
                fprintf(fp, "hwThread: %d\n",i);
                m_hwThreadMap[i].checkpoint( fp );
            }
        }
        void checkpointLoad( FILE* fp ) {
        }
      private:
        std::vector< HardwareThreadInfo > m_hwThreadMap;
    };

    VanadisSyscall* getSyscall( int core, int hwThread ) {
        return m_coreInfoMap[core].getSyscall( hwThread ); 
    }

public:

    void sendEvent( int core, SST::Event* ev ) {
        core_links.at(core)->send( ev );
    }

    void setProcess( int core, int hwThread,  OS::ProcessInfo* process ) {
        m_coreInfoMap.at(core).setProcess( hwThread, process );
    }

    Output* getOutput() { return output; }

    void setSyscall( int core, int hwThread, VanadisSyscall* syscall) {
        output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL,"core=%d hwThread=%d\n",core,hwThread);
        m_coreInfoMap[core].setSyscall( hwThread, syscall ); 
    }
    void clearSyscall( int core, int hwThread ) {
        output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL,"core=%d hwThread=%d\n",core, hwThread );
        m_coreInfoMap[core].clearSyscall( hwThread ); 
    }

    OS::HwThreadID* allocHwThread() {
        if ( m_availHwThreads.empty() ) {
            return nullptr;
        } 

        OS::HwThreadID* threadID = m_availHwThreads.front();
        m_availHwThreads.pop();

        return threadID;
    }

    void removeThread( unsigned core, unsigned hwThread, unsigned tid ) {

        auto iter = m_threadMap.find( tid );
        assert( iter != m_threadMap.end() );
        m_threadMap.erase( iter );

        getMMU()->flushTlb( core, hwThread );

        // clear the process/thread to hwThread map 
        m_coreInfoMap.at( core ).setProcess( hwThread, nullptr );

        // we are puting this hwThread back into avail pool
        // do we need to worry about reallocating it before it gets the halt message, probably not, should put a check in the core to see if idle?
        m_availHwThreads.push( new OS::HwThreadID( core, hwThread ) );

        if ( m_threadMap.empty() ) {
            printf("all process have exited\n");
            primaryComponentOKToEndSim();
        }
    }

    int getNewTid() { return m_currentTid++; }
    MMU_Lib::MMU* getMMU() { return m_mmu; };

    void setThread( int tid, OS::ProcessInfo* thread ) {
        m_threadMap[tid] = thread;
    }

    OS::Device* getDevice( int id ) { 
    	assert( m_deviceList.find(id) != m_deviceList.end() );
	return m_deviceList[id];
    }

    int getNodeNum() { return m_nodeNum; }
    int getPageSize() { return m_pageSize; }
    int getPageShift() { return m_pageShift; }

private:

    SST::Output*                output;
    std::vector<SST::Link*>     core_links;
    std::vector< CoreInfo >     m_coreInfoMap;
    MMU_Lib::MMU*               m_mmu;
    StandardMem*                mem_if;
    PhysMemManager*             m_physMemMgr;
    AppRuntimeMemoryMod*        m_appRuntimeMemory;
    int                         m_processDebugLevel;
    int                         m_pageSize;
    int                         m_pageShift;
    uint64_t                    m_phdr_address;
    uint64_t                    m_stack_top;
    int                         m_nodeNum;
    uint64_t                    m_osStartTimeNano;

    std::queue<PageFault*>                          m_pendingFault;
    std::map<std::string, VanadisELFInfo* >         m_elfMap; 
    std::unordered_map<uint32_t,OS::ProcessInfo*>   m_threadMap;
    std::queue<PageMemReq*>                         m_blockMemoryWriteReqQ;

    std::map< VanadisELFInfo*, std::map<int,OS::Page*> >            m_elfPageCache;
    std::unordered_map<StandardMem::Request::id_t, VanadisSyscall*> m_memRespMap;

    std::queue< OS::HwThreadID* > m_availHwThreads;

    std::map< int, OS::Device* > m_deviceList;

    int m_currentTid;

    OS::Page* allocPage() {
        auto page = new OS::Page(m_physMemMgr);
        output->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"ppn=%d\n",page->getPPN());
        return page;
    }

    std::string m_checkpointDir;
    enum { NO_CHECKPOINT, CHECKPOINT_LOAD, CHECKPOINT_SAVE }  m_checkpoint;

    void checkpoint( std::string dir );
    int checkpointLoad( std::string dir );
    std::deque<uint64_t> m_flushPages;
};

} // namespace Vanadis
} // namespace SST

#endif
