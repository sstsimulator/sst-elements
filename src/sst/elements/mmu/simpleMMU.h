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

#ifndef SIMPLE_MMU_H
#define SIMPLE_MMU_H

#include <sst/core/link.h>
#include "mmu.h"
#include "mmuTypes.h"

namespace SST {

#define MMU_DBG_CHECKPOINT (1<<0)
namespace MMU_Lib {

class SimpleMMU : public MMU {

  public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        SimpleMMU,
        "mmu",
        "simpleMMU",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Simple MMU,",
        SST::MMU_Lib::SimpleMMU
    )

    SST_ELI_DOCUMENT_PARAMS(
#if 0
        {"hitLatency", "latency of MMU hit in ns","0"},
#endif
    )

    SimpleMMU(SST::ComponentId_t id, SST::Params& params);
    void checkpoint( std::string );
    void checkpointLoad( std::string );

    virtual void removeWrite( unsigned pid );
    virtual void map( unsigned pid, uint32_t vpn, std::vector<uint32_t>& ppns, int pageSize, uint64_t flags );
    virtual void map( unsigned pid, uint32_t vpn, uint32_t ppn, int pageSize, uint64_t flags );
    virtual void unmap( unsigned pid, uint32_t vpn, size_t numPages );
    virtual void dup( unsigned fromPid, unsigned toPid );

    virtual void flushTlb( unsigned core, unsigned hwThread );

    virtual int getPerms( unsigned pid, uint32_t vpn );
    virtual void faultHandled( RequestID, unsigned link, unsigned pid, unsigned vpn, bool success );

    void init( unsigned int phase )
    {
        m_dbg.debug(CALL_INFO_LONG,1,0,"phase=%d\n",phase);
        MMU::init( phase );
    }

    void initPageTable( unsigned pid ) {
        initPageTable( pid, nullptr );
    }

    void setCoreToPageTable( unsigned core, unsigned hwThread, unsigned pid ) {
        m_dbg.debug(CALL_INFO_LONG,1,0,"pid=%d core=%d hwTread=%d\n",pid,core,hwThread);
        m_coreToPid[core][hwThread] = pid;
    }

    virtual uint32_t getPerms( unsigned pid, uint64_t vpn ) {
        auto pageTable = m_pageTableMap[pid];
        assert( pageTable );
        uint32_t perms = -1;
        PTE* pte = nullptr;
        if ( ( pte = pageTable->find( vpn ) ) ) {
            m_dbg.debug(CALL_INFO_LONG,1,0,"found PTE ppn %d, perms %#x\n",pte->ppn,pte->perms);
            perms = pte->perms;
        }
        m_dbg.debug(CALL_INFO_LONG,1,0,"pid=%d vpn=%" PRIu64 " -> perms=%d\n",pid,vpn,perms);
        return perms;
    }

    virtual uint32_t virtToPhys( unsigned pid, uint64_t vpn ) {
        auto pageTable = m_pageTableMap[pid];
        assert( pageTable );
        uint32_t ppn= -1;
        PTE* pte = nullptr;
        if ( ( pte = pageTable->find( vpn ) ) ) {
            m_dbg.debug(CALL_INFO_LONG,1,0,"found PTE ppn %d, perms %#x\n",pte->ppn,pte->perms);
            ppn = pte->ppn;
        }
        m_dbg.debug(CALL_INFO_LONG,1,0,"pid=%d vpn=%" PRIu64 " -> ppn=%d\n",pid,vpn,ppn);
        return ppn;
    }

  private:

    class PageTable {
      public:
        PageTable() {}
        PageTable( SST::Output* output, FILE* fp ) {
            int size;

            assert( 1 == fscanf( fp, "pteMap.size() %d\n", &size ) );
            output->debug(CALL_INFO_LONG,1,MMU_DBG_CHECKPOINT,"pteMap.size() %d\n",size);
            for ( auto i = 0; i < size; i++ ) {
                uint32_t vpn;
                uint32_t ppn;
                uint32_t perms;
                assert( 3 == fscanf( fp, "vpn: %d, ppn: %d, perms: %x\n", &vpn, &ppn, &perms ) );
                output->debug(CALL_INFO_LONG,1,MMU_DBG_CHECKPOINT,"vpn: %d, ppn: %d, perms: %x\n", vpn, ppn, perms );
                pteMap[vpn] = PTE( ppn, perms );
            }
        }

        void add( uint32_t vpn, PTE pte ) { 
            pteMap[vpn] = pte;
        }
        void remove( uint32_t vpn ) { 
            pteMap.erase(vpn);
        }
        PTE* find( uint32_t vpn ) {
            if ( pteMap.find( vpn ) == pteMap.end() ) {
                return nullptr;
            } else { 
                return &pteMap[vpn];
            }
        }
        void removeWrite(  ) { 
            for ( auto& kv : pteMap ) {
                kv.second.perms &= ~0x2;
            }
        }
        void print( const std::string str) {
            for ( auto& kv : pteMap ) {
                printf("PageTabl::%s() %s vpn=%d ppn=%d perm=%#x\n",__func__,str.c_str(),kv.first,kv.second.ppn,kv.second.perms);
            }
        }
        void checkpoint( FILE* fp ) {
            fprintf(fp,"pteMap.size() %zu\n",pteMap.size());
            for ( auto & x : pteMap ) {
                fprintf(fp,"vpn: %d, ppn: %d, perms: %d \n", x.first,x.second.ppn,x.second.perms );
            }
        }
      private:
        std::map<uint32_t,PTE> pteMap; 
    };

    void initPageTable( unsigned pid, PageTable* table = nullptr ) {
        m_dbg.debug(CALL_INFO_LONG,1,0,"pid=%d\n",pid);
        auto iter = m_pageTableMap.find(pid);
        if ( iter == m_pageTableMap.end() ) {
            if ( nullptr == table ) {
                table = new PageTable;
            }    
             
            m_pageTableMap[pid] = table;
        } else {
            assert(0);
        }
    }

    void handleTlbEvent( Event* ev, int link );
    void handleNicTlbEvent( Event* ev );


    unsigned getPid( unsigned core, unsigned hwThread ) {
        m_dbg.debug(CALL_INFO_LONG,1,0,"core=%d hwThread=%d -> pid=%d\n",core,hwThread,m_coreToPid[core][hwThread]);
        return m_coreToPid[core][hwThread];
    }

    PageTable* getPageTable( unsigned core, unsigned hwThread ) {
        m_dbg.debug(CALL_INFO_LONG,1,0,"core=%d hwThread=%d\n",core,hwThread);
        int pid = m_coreToPid[core][hwThread];
        return getPageTable( pid );
    }

    PageTable* getPageTable( unsigned pid) {
        auto iter = m_pageTableMap.find( pid );
        if ( iter == m_pageTableMap.end() ) {
            return nullptr;
        }
        return iter->second;
    }

    std::map< unsigned, PageTable* > m_pageTableMap;

    std::vector< std::vector< unsigned > > m_coreToPid;
};

} //namespace MMU_Lib
} //namespace SST

#endif /* SIMPLE_MMU_H */
