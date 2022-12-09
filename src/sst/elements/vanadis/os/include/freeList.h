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

#ifndef _H_VANADIS_NODE_OS_INCLUDE_FREE_LIST
#define _H_VANADIS_NODE_OS_INCLUDE_FREE_LIST

//#include <iterator>
//#include <string>

namespace SST {
namespace Vanadis {

namespace OS {

#if 0
#define FreeListDbg( format, ... ) printf( "FreeList::%s() " format, __func__, ##__VA_ARGS__ )
#else
#define FreeListDbg( ... )
#endif

struct FreeList {

    struct FreeEntry {
        FreeEntry( uint64_t start, size_t end ) : start(start), end(end) {}
        uint64_t start;
        uint64_t end;
    };

    FreeList( uint64_t start, uint64_t end ) {
        m_freeList[start] = new FreeEntry( start, end );
        print();
    }

    FreeList ( const FreeList& obj ) {
        m_freeList = obj.m_freeList;
    }

    void print() {
        for ( const auto kv: m_freeList) {
            auto entry = kv.second;
            FreeListDbg("free %#" PRIx64 " %#" PRIx64 "\n",entry->start, entry->end);
        }
    }

    bool alloc( uint64_t addr, size_t len ) {
        FreeListDbg("[%#" PRIx64 "-%#" PRIx64 "]\n",addr,addr+len);
        int ret = false;
        for ( const auto kv: m_freeList) {
            auto entry = kv.second;
            FreeListDbg("checking freeEntry [%#" PRIx64 "-%#" PRIx64 "]\n",entry->start, entry->end);
            if ( addr >= entry->start && addr + len <= entry->end ) {
                ret = true;
                FreeListDbg("found avail\n"); 
                if ( addr == entry->start ) { 
                    
                    if ( addr + len < entry->end ) {
                        entry->start = addr + len; 
                    } else {
                        delete entry; 
                        m_freeList.erase( kv.first );
                    }
                         
                // start is different but end is the same as the free entry
                } else if ( addr + len == entry->end )  {
                    // just move the end of the free entry
                    entry->end = addr;
                // trucate the current free entry and add a new one 
                } else {
                    uint64_t end = entry->end; 
                    // end of the fee entry is now the start of removed region
                    entry->end = addr;
                    // new free entry starts at the end of the removed region and ends at end of the original free entry
                    m_freeList[addr+len] = new FreeEntry( addr+len, end );
                }
                break;
            }
        }
            
        print();
        FreeListDbg("ret=%d\n",ret);
        return ret; 
    }

    uint64_t alloc( size_t wantLen ) {

        FreeListDbg("length=%zu\n",wantLen);
        uint64_t ret = 0;
        for ( const auto kv: m_freeList) {
            auto entry = kv.second;
            auto length =  entry->end - entry->start;
            FreeListDbg("checking freeEntry [%#" PRIx64 "-%#" PRIx64 "]\n",entry->start, entry->end);
            if ( wantLen <= length ) {
                FreeListDbg("found avail\n"); 
                ret = entry->start;
                if ( wantLen == length ) {
                    delete entry;
                    m_freeList.erase(kv.first);
                } else {
                    entry->start = entry->start+wantLen; 
                }
                break;
            }
        }
        print();
        FreeListDbg("ret=%#" PRIx64 "\n",ret);
        return ret;
    }
    void free( uint64_t addr, size_t length ) {
        FreeListDbg("[%#" PRIx64 "-%#" PRIx64 "]\n",addr,addr+length);
        for ( const auto kv: m_freeList) {
            auto entry = kv.second;
            FreeListDbg("checking freeEntry [%#" PRIx64 "-%#" PRIx64 "]\n",entry->start, entry->end);
            if ( addr == entry->end ) {
                FreeListDbg("merge tail\n");
                entry->end = addr + length;
                merge();
                break;
            } else if ( addr + length == entry->start ) {
                FreeListDbg("merge head\n");
                entry->start = addr;
                merge();
                break;
            } else {
                FreeListDbg("new\n");
                m_freeList[addr] = new FreeEntry( addr, addr + length );
                break;
            }
        }
        print();
    }

    bool update( uint64_t start, size_t length ) {
        bool ret = false;
        FreeListDbg("start=%#" PRIx64 " length=%zu\n",start,length);
        auto iter = m_freeList.find( start );
        if ( iter != m_freeList.end() ) {
            FreeListDbg("found free entry start=%#" PRIx64 " length=%zu\n", start,length);
            auto entry = iter->second;
            if ( entry->start + length < entry->end ) {
                entry->start += length;
                ret = true;
            } else if ( entry->start + length == entry->end ) {
                delete entry;
                m_freeList.erase( iter );
                ret = true;
            }
        }
        print();
        return ret;
    } 

    void merge() {
        FreeListDbg("\n");
         
        for ( auto iter = m_freeList.begin(); iter != m_freeList.end(); ++iter) {
            auto entry = iter->second;
            FreeListDbg("checking freeEntry [%#" PRIx64 "-%#" PRIx64 "]\n", entry->start, entry->end);
            auto next = std::next(iter,1); 

            if ( next != m_freeList.end() ) {
                auto nextEntry = next->second;
                if ( entry->end == nextEntry->start ) {
                    FreeListDbg("merge [%#" PRIx64 "-%#" PRIx64 "] [%#" PRIx64 "-%#" PRIx64 "]",
                            entry->start, entry->end, nextEntry->start, nextEntry->end);
                    
                    // merge free list blocks
                    entry->end = nextEntry->end;  

                    // remove entry from free list 
                    delete nextEntry; 
                    next = m_freeList.erase(next); 

                    if ( next != m_freeList.end() ) {
                        auto nextEntry = next->second;
                        if ( entry->end == nextEntry->start ) {
                            FreeListDbg("merge [%#" PRIx64 "-%#" PRIx64 "] [%#" PRIx64 "-%#" PRIx64 "]",
                            entry->start, entry->end, nextEntry->start, nextEntry->end);

                            // merge free list block
                            entry->end = nextEntry->end;
                            delete nextEntry;
                            m_freeList.erase(next);
                        }
                    }
                }
            }
        }
    }
    std::map< uint64_t, FreeEntry* > m_freeList;
}; 

}
}
}

#endif
