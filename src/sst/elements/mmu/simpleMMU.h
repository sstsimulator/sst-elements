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

#define MMU_DBG_SNAPSHOT (1<<0)
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
        {"debug_level", "Debug verbosity level (0-10) where 0 is no output. Output is only available if sst-core was compiled with `--enable-debug`.", "0"},
    )

    SimpleMMU(SST::ComponentId_t id, SST::Params& params);
    void snapshot( std::string ) override;
    void snapshotLoad( std::string ) override;

    virtual void removeWrite( uint32_t pid ) override;
    virtual void map( uint32_t pid, uint32_t vpn, std::vector<uint32_t>& ppns, uint32_t page_size, uint64_t flags ) override;
    virtual void map( uint32_t pid, uint32_t vpn, uint32_t ppn, uint32_t page_size, uint64_t flags ) override;
    virtual void unmap( uint32_t pid, uint32_t vpn, size_t num_pages ) override;
    virtual void dup( uint32_t from_pid, uint32_t to_pid ) override;
    virtual void flushTlb( uint32_t core, uint32_t hw_thread ) override;
    virtual void faultHandled( RequestID, uint32_t link, uint32_t pid, uint32_t vpn, bool success ) override;

    void init( unsigned int phase ) override
    {
        MMU::init( phase );
    }

    void initPageTable( uint32_t pid ) override {
        initPageTable( pid, nullptr );
    }

    void setCoreToPageTable( uint32_t core, uint32_t hw_thread, uint32_t pid ) override {
#ifdef __SST_DEBUG_OUTPUT__
        dbg_.debug(CALL_INFO_LONG,1,0,"pid=%" PRIu32 " core=%" PRIu32 " hw_thread=%" PRIu32 "\n",pid,core,hw_thread);
#endif
        core_to_pid_[core][hw_thread] = pid;
    }

    virtual uint32_t getPerms( uint32_t pid, uint32_t vpn ) override {
        auto page_table = page_table_map_[pid];
        assert( page_table );
        uint32_t perms = std::numeric_limits<uint32_t>::max();
        PTE* pte = nullptr;
        if ( ( pte = page_table->find( vpn ) ) ) {
#ifdef __SST_DEBUG_OUTPUT__
            dbg_.debug(CALL_INFO_LONG,1,0,"found PTE ppn %" PRIu32 ", perms %#" PRIx32 "\n",pte->ppn,pte->perms);
#endif
            perms = pte->perms;
        }
#ifdef __SST_DEBUG_OUTPUT__
        dbg_.debug(CALL_INFO_LONG,1,0,"pid=%" PRIu32 ", vpn=%" PRIu64 " -> perms=%#" PRIx32 "\n",pid,vpn,perms);
#endif
        return perms;
    }

    virtual uint32_t virtToPhys( uint32_t pid, uint32_t vpn ) override {
        auto page_table = page_table_map_[pid];
        assert( page_table );
        uint32_t ppn= std::numeric_limits<uint32_t>::max();
        PTE* pte = nullptr;
        if ( ( pte = page_table->find( vpn ) ) ) {
#ifdef __SST_DEBUG_OUTPUT__
            dbg_.debug(CALL_INFO_LONG,1,0,"found PTE ppn %" PRIu32 ", perms %#" PRIx32 "\n",pte->ppn,pte->perms);
#endif
            ppn = pte->ppn;
        }
#ifdef __SST_DEBUG_OUTPUT__
        dbg_.debug(CALL_INFO_LONG,1,0,"pid=%" PRIu32 " vpn=%" PRIu32 " -> ppn=%" PRIu32 "\n",pid,vpn,ppn);
#endif
        return ppn;
    }

  private:

    class PageTable {
      public:
        PageTable() {}
        PageTable( SST::Output* output, FILE* fp ) {
            size_t size;

            assert( 1 == fscanf( fp, "pteMap.size() zu\n", &size ) );
            output->debug(CALL_INFO_LONG,1,MMU_DBG_SNAPSHOT,"pteMap.size() %zu\n",size);
            for ( auto i = 0; i < size; i++ ) {
                uint32_t vpn;
                uint32_t ppn;
                uint32_t perms;
                assert( 3 == fscanf( fp, "vpn: %" PRIu32 ", ppn: %" PRIu32 ", perms: %" PRIx32 "\n", &vpn, &ppn, &perms ) );
                output->debug(CALL_INFO_LONG,1,MMU_DBG_SNAPSHOT,"vpn: %" PRIu32 ", ppn: %" PRIu32 ", perms: %" PRIx32 "\n", vpn, ppn, perms );
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
                printf("PageTabl::%s() %s vpn=%" PRIu32 " ppn=%" PRIu32 " perm=%#" PRIx32 "\n",__func__,str.c_str(),kv.first,kv.second.ppn,kv.second.perms);
            }
        }
        void snapshot( FILE* fp ) {
            fprintf(fp,"pteMap.size() %zu\n",pteMap.size());
            for ( auto & x : pteMap ) {
                fprintf(fp,"vpn: %" PRIu32 ", ppn: %" PRIu32 ", perms: %#" PRIx32 " \n", x.first,x.second.ppn,x.second.perms );
            }
        }
      private:
        std::map<uint32_t,PTE> pteMap;
    };

    void initPageTable( uint32_t pid, PageTable* table = nullptr ) {
#ifdef __SST_DEBUG_OUTPUT__
        dbg_.debug(CALL_INFO_LONG,1,0,"pid=%" PRIu32 "\n",pid);
#endif
        auto iter = page_table_map_.find(pid);
        if ( iter == page_table_map_.end() ) {
            if ( nullptr == table ) {
                table = new PageTable;
            }

            page_table_map_[pid] = table;
        } else {
            assert(0);
        }
    }

    void handleTlbEvent( Event* ev, int link );
    void handleNicTlbEvent( Event* ev );


    uint32_t getPid( uint32_t core, uint32_t hw_thread ) {
#ifdef __SST_DEBUG_OUTPUT__
        dbg_.debug(CALL_INFO_LONG,1,0,"core=%" PRIu32 " hw_thread=%" PRIu32 " -> pid=%" PRIu32 "\n",core,hw_thread,core_to_pid_[core][hw_thread]);
#endif
        return core_to_pid_[core][hw_thread];
    }

    PageTable* getPageTable( uint32_t core, uint32_t hw_thread ) {
#ifdef __SST_DEBUG_OUTPUT__
        dbg_.debug(CALL_INFO_LONG,1,0,"core=%" PRIu32 " hw_thread=%" PRIu32 "\n",core,hw_thread);
#endif
        uint32_t pid = core_to_pid_[core][hw_thread];
        return getPageTable( pid );
    }

    PageTable* getPageTable( uint32_t pid) {
        auto iter = page_table_map_.find( pid );
        if ( iter == page_table_map_.end() ) {
            return nullptr;
        }
        return iter->second;
    }

    std::map< uint32_t, PageTable* > page_table_map_;      /* Maps pid to PageTable for that process */

    std::vector< std::vector< uint32_t > > core_to_pid_;    /* Maps [core_id][hw_thread] = [pid]*/
};

} //namespace MMU_Lib
} //namespace SST

#endif /* SIMPLE_MMU_H */
