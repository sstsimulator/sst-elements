#ifndef _nicMmu_h
#define _nicMmu_h

#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>

class NicMmu 
{
    typedef unsigned long Addr;
    static const int NumTables = 100; 
    static const int PageSizeBits = 13;
    static const int LevelBits = 10; 
    static const int NumEntries = 1 << LevelBits;
    static const int Mask = NumEntries - 1;
    
    struct Entry { 
        int pfn;
    };

    struct Table {
        Entry entry[NumEntries];   
    };

  public:
    
    NicMmu( std::string fileName, bool create ) :
        m_nextPFN( 1 ),
        m_l0_pfn( 0 )
    {
        int oflags = O_RDWR; 
        int prot = PROT_READ|PROT_WRITE;
        
        //printf("NicMmu::%s() `%s` NumEntries=%d Mask=%#x\n",__func__,
         //           fileName.c_str(),NumEntries,Mask);
#if 1 

        if ( create ) {
            oflags |= O_CREAT | O_TRUNC;
        }
        
        m_fd = shm_open( fileName.c_str(), oflags, 0777 );
       
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
        m_table = (Table*) mmap( 0, sizeof(Table) *  NumTables, 
                            prot, MAP_SHARED ,m_fd, 0 );
        if ( m_table == (void*) -1 ) {
            perror("mmap");
            abort();
        }
#endif
    }

    void add( Addr vaddr, Addr paddr ) 
    {
        //printf("add() vaddr=%#lx paddr %#lx pfn=%d\n", vaddr, paddr,
         //               (int) paddr >> 13 );
        Entry* entry = find_L3_entry( vaddr ); 
        entry->pfn = paddr >> 13; 
    }

    Entry* find_L3_entry( Addr vaddr ) {
        Addr tmp = vaddr >> PageSizeBits; 
        int  l3 = tmp & Mask;
        tmp >>= LevelBits;
        int  l2 = tmp & Mask;
        tmp >>= LevelBits;
        int  l1 = tmp & Mask;
        tmp >>= LevelBits;
        int  l0 = tmp & Mask; 
        
        //printf("vaddr %#lx L0=%#x L1=%#x L2=%#x L3=%#x\n",
        //                    vaddr, l0, l1, l2, l3 );
    
        Table* l0_table = &m_table[ m_l0_pfn ];  
        Entry* l0_entry = &l0_table->entry[l0]; 
        if ( l0_entry->pfn == 0 ) {
            l0_entry->pfn = new_page();
//            printf("new L1 pfn=%d\n",l0_entry->pfn); 
        }

        Table* l1_table = &m_table[ l0_entry->pfn ];  
        Entry* l1_entry = &l1_table->entry[l1]; 
        if ( l1_entry->pfn == 0 ) {
            l1_entry->pfn = new_page();
//            printf("new L2 pfn=%d\n",l1_entry->pfn); 
        }

        Table* l2_table = &m_table[ l1_entry->pfn ];  
        Entry* l2_entry = &l2_table->entry[ l2 ];  
        if ( l2_entry->pfn == 0 ) {
            l2_entry->pfn = new_page();
//            printf("new L3 pfn=%d\n",l2_entry->pfn); 
        }
        //printf("l0pfn %d l1pfn %d l2pfn %d\n",
        //l0_entry->pfn, l1_entry->pfn, l2_entry->pfn);
        
        return &m_table[ l2_entry->pfn ].entry[l3];  
    }

    bool lookup( Addr vaddr, Addr &paddr ) {
        //Addr tmp = vaddr;
        Entry* entry = find_L3_entry( vaddr ); 
        paddr = ( entry->pfn << 13 )  | (vaddr & 0x1fff);
        //printf("vaddr %#lx paddr %#lx %d\n", tmp,vaddr, entry->pfn );
        return true;
    }
    
  private:
    int new_page() {
        int tmp = m_nextPFN++; 
        assert( m_nextPFN != NumTables );
        return tmp;
    }
    int     m_nextPFN;
    int     m_fd;
    Table*  m_table;
    int     m_l0_pfn;
};

#endif
