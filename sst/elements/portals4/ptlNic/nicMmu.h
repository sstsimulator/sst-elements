// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _nicMmu_h
#define _nicMmu_h

#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>

#if 0
#define MMU_DBG( fmt, args... ) \
    fprintf(stderr, "NicMmu::%s():%d: "fmt, __func__, __LINE__, ##args)
#else
#define MMU_DBG( fmt, args... )
#endif

namespace SST {
namespace Portals4 {

class NicMmu 
{
    typedef uint64_t Addr;
    static const int NumTables = 1000; 

#if THE_ISA == X86_ISA
    static const int PageSizeBits = 12;
    static const int LevelBits = 9; 
#elif THE_ISA == ALPHA_ISA
    static const int PageSizeBits = 13;
    static const int LevelBits = 10; 
#elif THE_ISA == SPARC_ISA
    static const int PageSizeBits = 13;
    static const int LevelBits = 10; 
#else
    #error What ISA
#endif  
    static const int NumEntries = 1 << LevelBits;
    static const int Mask = NumEntries - 1;
    
    struct Entry { 
        int pfn;
    };

    struct Table {
        Entry entry[NumEntries];   
    };

    typedef Table pgd_t;
    typedef Table pmd_t;
    typedef Table pte_t;

    pgd_t* m_pgd;

  public:
    
    NicMmu( std::string fileName, bool create ) :
        m_nextPFN( 1 )
    {
        int oflags = O_RDWR; 
        int prot = PROT_READ|PROT_WRITE;
        
		MMU_DBG("create=%d PageSizeBits=%d LevelBits=%d pageSize=%d\n", 
	    		create, PageSizeBits, LevelBits, (1<<PageSizeBits) - 1 );
        MMU_DBG("NumEntries=%d Mask=%#x\n", NumEntries, Mask);

        size_t pos = fileName.find_last_of( "/" );

        std::stringstream tmp;
        tmp << fileName.substr(pos).c_str() << "." << getpid();

        MMU_DBG("shm_open( `%s` ) \n", tmp.str().c_str() );

        if ( create ) {
            oflags |= O_CREAT | O_TRUNC;
        }
        
        m_fd = shm_open( tmp.str().c_str(), oflags, 0777 );
       
        if( m_fd == -1 ) {
            perror("shm_open");
            abort();
        }

        if ( create ) {
            if ( ftruncate( m_fd, sizeof(Table) *  NumTables ) == -1 ) {
                perror("ftruncate");
                abort();
            }
        }
        m_pgd = (Table*) mmap( 0, sizeof(Table) *  NumTables, 
                            prot, MAP_SHARED ,m_fd, 0 );
        if ( m_pgd == (void*) -1 ) {
            perror("mmap");
            abort();
        }
        m_table = m_pgd + 1;
        
        MMU_DBG("pdpt=%p free=%p\n",m_pgd,m_table); 
    }

    void add( Addr vaddr, Addr paddr ) 
    {
        Entry* entry = find_L3_entry( vaddr, true ); 
        MMU_DBG("vaddr=%#lx paddr %#lx pfn=%d %d\n", vaddr, paddr,
                        (int) paddr >> PageSizeBits, entry->pfn );
        if ( (unsigned int)entry->pfn == paddr >> PageSizeBits ) return;  
        assert( entry->pfn == 0 );
        entry->pfn = paddr >> PageSizeBits; 
    }

    pgd_t* pgd_offset( Addr vaddr, bool fill ) {
        Addr index = vaddr >> (PageSizeBits + (LevelBits * 3)); 

        if ( m_pgd->entry[index].pfn == 0 ) {
            if ( fill ) {
                m_pgd->entry[index].pfn = new_page();
            } else {
                return NULL;
            }
        }
//        MMU_DBG("pgd index %lu -> pfn %d\n", index, m_pgd->entry[index].pfn );
        return m_table + m_pgd->entry[index].pfn; 
    }

    pmd_t* pmd_offset( pgd_t* pgd, Addr vaddr, bool fill )
    {
        Addr index = vaddr >> (PageSizeBits + (LevelBits * 2)); 
        index &= Mask;

        if ( pgd->entry[index].pfn == 0 ) {
            if ( fill ) {
                pgd->entry[index].pfn = new_page();
            } else {
                return NULL;
            }
        }
//        MMU_DBG("pmd index %lu -> pfn %d\n", index, pgd->entry[index].pfn );
        return m_table + pgd->entry[index].pfn; 
    }

    pmd_t* pte_offset( pmd_t* pmd, Addr vaddr, bool fill )
    {
        Addr index = vaddr >> (PageSizeBits + (LevelBits * 1)); 
        index &= Mask;

        if ( pmd->entry[index].pfn == 0 ) {
            if ( fill ) {
                pmd->entry[index].pfn = new_page();
            } else {
                return NULL;
            }
        }
//        MMU_DBG("pte index %lu -> pfn %d\n", index, pmd->entry[index].pfn );
        return m_table + pmd->entry[index].pfn; 
    }


    Entry* find_L3_entry( Addr vaddr, bool fill = false ) {

//        MMU_DBG("vaddr=%#lx\n",vaddr);
        pgd_t* pgd = pgd_offset( vaddr, fill );
        if ( ! pgd ) {
            fprintf(stderr,"couldn't find pgd for  vaddr=%#"PRIx64"\n",vaddr);
            abort();
        }

        pmd_t* pmd = pmd_offset( pgd, vaddr, fill );
        if ( ! pmd ) {
            fprintf(stderr,"couldn't find pmd for  vaddr=%#"PRIx64"\n",vaddr);
            abort();
        }

        pte_t* pte = pte_offset( pmd, vaddr, fill );
        if ( ! pte ) {
            fprintf(stderr,"couldn't find pte for  vaddr=%#"PRIx64"x\n",vaddr);
            abort();
        }

        Addr tmp = vaddr >> PageSizeBits; 
        int  l3 = tmp & Mask;

//        MMU_DBG("pte=%p index=%d pfn=%d\n",pte, l3, pte->entry[l3].pfn);
        return &pte->entry[l3];
    }

    bool lookup( Addr vaddr, Addr &paddr ) {
        Entry* entry = find_L3_entry( vaddr ); 
        assert( entry );
        paddr = ( entry->pfn << PageSizeBits )  | 
                    ( vaddr & ( ( 1 << PageSizeBits) - 1 ) );
        MMU_DBG("vaddr=%#lx paddr=%#lx pfn=%d\n", vaddr, paddr, entry->pfn );
        return true;
    }

    int pageSize() {
        return 1 << PageSizeBits;
    }
    
  private:
    int new_page() {
        int tmp = m_nextPFN++; 
        
        if( m_nextPFN == NumTables ) {
            printf("%d %d\n",m_nextPFN,NumTables);
            assert( 0 );
        }
        MMU_DBG("new pfn=%d\n",tmp);
        return tmp;
    }
    int     m_nextPFN;
    int     m_fd;
    Table*  m_table;
    int     m_l0_pfn;
};

}
}

#endif
