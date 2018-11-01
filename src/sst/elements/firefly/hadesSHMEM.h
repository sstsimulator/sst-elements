// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_FIREFLY_HADESSHMEM_H
#define COMPONENTS_FIREFLY_HADESSHMEM_H

#include <sst/core/elementinfo.h>
#include <sst/core/params.h>

#include <stdlib.h>
#include <string.h>
#include "sst/elements/hermes/shmemapi.h"
#include "hades.h"
#include "shmem/common.h"
#include "shmem/barrier.h"
#include "shmem/broadcast.h"
#include "shmem/collect.h"
#include "shmem/fcollect.h"
#include "shmem/alltoall.h"
#include "shmem/alltoalls.h"
#include "shmem/reduction.h"

#define SHMEM_BASE      1<<0
#define SHMEM_BARRIER   1<<1

using namespace Hermes;

namespace SST {
namespace Firefly {

class HadesSHMEM : public Shmem::Interface
{
  public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        HadesSHMEM,
        "firefly",
        "hadesSHMEM",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        ""
    )
    SST_ELI_DOCUMENT_PARAMS(
        {"verboseLevel","Sets the level of debug verbosity",""},
        {"verboseMask","Sets the debug mask",""},
        {"enterLat_ns","Sets the latency of entering a SHMEM call","" },
        {"returnLat_ns","Sets the latency of returning from a SHMEM call","" },
        {"blockingReturnLat_ns","Sets the latency of returning from a SHMEM call that blocked on response","" },
    )

    typedef std::function<void()> Callback;

  private:
    class DelayEvent : public SST::Event {
      public:

		enum { One, Two } type;
        DelayEvent( Callback callback ) :
            Event(),
			type( One ),
            m_callback1( callback )
        {
		}

        DelayEvent( Shmem::Callback callback, int retval ) :
            Event(),
			type( Two ),
            m_callback2( callback ),
            m_retval( retval )
        {
            //printf("%s() %p\n",__func__,this);
        }
        ~DelayEvent()
        {
            //printf("%s() %p\n",__func__,this);
        }

        Callback 		m_callback1;
        Shmem::Callback m_callback2;
        int m_retval;

        NotSerializable(DelayEvent)
    };

    class Heap {
        struct Entry {
            Entry() {}
            Entry( Hermes::MemAddr addr, size_t length ) : addr(addr), length(length) {}

            Hermes::MemAddr addr;
            size_t length;
        };
      public:
        Heap( ) : m_curAddr(0x1000) {}
		~Heap() { 
            std::map<Hermes::Vaddr,Entry>::iterator iter = m_map.begin();
            for ( ; iter != m_map.end(); ++iter ) {
                Entry& entry = iter->second;
				if ( entry.addr.getBacking()  ) {
                	::free( entry.addr.getBacking() );
				}
			}
		}
        Hermes::MemAddr malloc( size_t n, bool backed ) {
            Hermes::MemAddr addr( Hermes::MemAddr::Shmem );
            addr.setSimVAddr( m_curAddr );

            m_curAddr += n;
            // allign on cache line
            m_curAddr += 64;
            m_curAddr &= ~(64-1);

            if ( backed ) {
                addr.setBacking( ::malloc(n) );
            }
            m_map[ addr.getSimVAddr() ] = Entry( addr, n );

            return addr; 
        }  
        void free( Hermes::MemAddr& addr ) {
            if ( addr.getBacking() ) {
                ::free( addr.getBacking() );
            }
            m_map.erase( addr.getSimVAddr() );
        }
        void* findBacking( Hermes::Vaddr addr ) {
            std::map<Hermes::Vaddr,Entry>::iterator iter = m_map.begin();
            for ( ; iter != m_map.end(); ++iter ) {
                Entry& entry = iter->second;
                if ( addr >= entry.addr.getSimVAddr() && addr < entry.addr.getSimVAddr() + entry.length ) {
                    unsigned char * ptr = NULL;
                    if ( entry.addr.getBacking() ) {
                        ptr = (unsigned char*) entry.addr.getBacking(); 
                        ptr += addr - entry.addr.getSimVAddr(); 
                    }
                    return ptr;
                }
            } 
            assert(0);
        }
      private:
        size_t m_curAddr;
        std::map<Hermes::Vaddr, Entry > m_map;
    };

  public:
    HadesSHMEM(Component*, Params&);
    ~HadesSHMEM();

    virtual void setup();
    virtual void finish() {}
    virtual std::string getName() { return "HadesSHMEM"; }

    virtual void setOS( OS* os ) {
        m_os = static_cast<Hades*>(os);
        dbg().debug(CALL_INFO,2,0,"\n");
    }

    virtual void init(Shmem::Callback);
    virtual void init2(Shmem::Callback);
    virtual void finalize(Shmem::Callback);
    virtual void finalize2(Shmem::Callback);

    virtual void n_pes(int*, Shmem::Callback);
    virtual void n_pes2(int*, Shmem::Callback);
    virtual void my_pe(int*, Shmem::Callback);
    virtual void my_pe2(int*, Shmem::Callback);

    virtual void fence(Shmem::Callback);
    virtual void fence2(Shmem::Callback);
    virtual void quiet(Shmem::Callback);
    virtual void quiet2(Shmem::Callback);

    virtual void barrier_all(Shmem::Callback);
    virtual void barrier_all2(Shmem::Callback);
    virtual void barrier( int start, int stride, int size, Vaddr, Shmem::Callback);
    virtual void barrier2( int start, int stride, int size, Vaddr, Shmem::Callback);
    virtual void broadcast( Vaddr dest, Vaddr source, size_t nelems, int root, int PE_start,
                        int logPE_stride, int PE_size, Vaddr, Shmem::Callback);
    virtual void broadcast2( Vaddr dest, Vaddr source, size_t nelems, int root, int PE_start,
                        int logPE_stride, int PE_size, Vaddr, Shmem::Callback);
    virtual void fcollect( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
                        int logPE_stride, int PE_size, Vaddr, Shmem::Callback);
    virtual void fcollect2( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
                        int logPE_stride, int PE_size, Vaddr, Shmem::Callback);
    virtual void collect( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
                        int logPE_stride, int PE_size, Vaddr, Shmem::Callback);
    virtual void collect2( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
                        int logPE_stride, int PE_size, Vaddr, Shmem::Callback);
    virtual void alltoall( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
                        int logPE_stride, int PE_size, Vaddr, Shmem::Callback);
    virtual void alltoall2( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
                        int logPE_stride, int PE_size, Vaddr, Shmem::Callback);
    virtual void alltoalls( Vaddr dest, Vaddr source, int dst, int sst, size_t nelems, int elsize, 
                        int PE_start, int logPE_stride, int PE_size, Vaddr, Shmem::Callback);
    virtual void alltoalls2( Vaddr dest, Vaddr source, int dst, int sst, size_t nelems, int elsize, 
                        int PE_start, int logPE_stride, int PE_size, Vaddr, Shmem::Callback);
    virtual void reduction( Vaddr dest, Vaddr source, int nelems, int PE_start,
                        int logPE_stride, int PE_size, Vaddr pSync,
                        Hermes::Shmem::ReduOp, Hermes::Value::Type, Shmem::Callback);
    virtual void reduction2( Vaddr dest, Vaddr source, int nelems, int PE_start,
                        int logPE_stride, int PE_size, Vaddr pSync,
                        Hermes::Shmem::ReduOp, Hermes::Value::Type, Shmem::Callback);

    virtual void malloc(Hermes::MemAddr*,size_t,bool backed, Shmem::Callback);
    virtual void malloc2(Hermes::MemAddr*,size_t,bool backed, Shmem::Callback);
    virtual void free(Hermes::MemAddr*,Shmem::Callback);

    virtual void getv(Hermes::Value&, Hermes::Vaddr src, int pe, Shmem::Callback);
    virtual void getv2(Hermes::Value&, Hermes::Vaddr src, int pe, Shmem::Callback);
    virtual void get_nbi(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe, Shmem::Callback);
    virtual void get(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe, Shmem::Callback);
    virtual void get2(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe, bool blocking, Shmem::Callback, SimTime_t delay);

    virtual void putv(Hermes::Vaddr dest, Hermes::Value&, int pe, Shmem::Callback);
    virtual void putv2(Hermes::Vaddr dest, Hermes::Value&, int pe, Shmem::Callback);
    virtual void put_nbi(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe, Shmem::Callback);
    virtual void put(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe, Shmem::Callback);
    virtual void put2(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe, bool blocking, Shmem::Callback, SimTime_t delay);

    virtual void putOp(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe, 
            Hermes::Shmem::ReduOp, Hermes::Value::Type, Shmem::Callback);
    virtual void putOp2(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe, 
            Hermes::Shmem::ReduOp, Hermes::Value::Type, Shmem::Callback);

    virtual void wait_until(Hermes::Vaddr src, Hermes::Shmem::WaitOp, Hermes::Value&, Shmem::Callback);
    virtual void wait_until2(Hermes::Vaddr src, Hermes::Shmem::WaitOp, Hermes::Value&, Shmem::Callback);

    virtual void cswap( Hermes::Value& result, Hermes::Vaddr, Hermes::Value& cond, Hermes::Value& value, int pe, Shmem::Callback);
    virtual void cswap2( Hermes::Value& result, Hermes::Vaddr, Hermes::Value& cond, Hermes::Value& value, int pe, Shmem::Callback);

    virtual void swap( Hermes::Value& result, Hermes::Vaddr, Hermes::Value& value, int pe, Shmem::Callback);
    virtual void swap2( Hermes::Value& result, Hermes::Vaddr, Hermes::Value& value, int pe, Shmem::Callback);
    virtual void add(  Hermes::Vaddr, Hermes::Value&, int pe, Shmem::Callback);
    virtual void add2(  Hermes::Vaddr, Hermes::Value&, int pe, Shmem::Callback);
    virtual void fadd( Hermes::Value&, Hermes::Vaddr, Hermes::Value&, int pe, Shmem::Callback);
    virtual void fadd2( Hermes::Value&, Hermes::Vaddr, Hermes::Value&, int pe, Shmem::Callback);

    void memcpy( Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, Shmem::Callback callback );

    void delay( Shmem::Callback callback, uint64_t delay, int retval );

    void* getBacking( Hermes::Vaddr addr ) {
        return m_heap->findBacking(addr);
    }

    Output& dbg() { return m_dbg; }
  private:
    virtual void malloc(Hermes::MemAddr*,size_t,bool backed,Callback);
    Output m_dbg;
    Hades*      m_os;

	void delayEnter( Callback callback, SimTime_t delay = 0 );
	void delayEnter( Shmem::Callback callback, SimTime_t delay = 0 );
	void delayReturn( Shmem::Callback callback, SimTime_t delay = 0 );

    void handleToDriver(SST::Event* e) {
        dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
        DelayEvent* event = static_cast<DelayEvent*>(e);
		if ( DelayEvent::One == event->type ) {
        	event->m_callback1();
		} else {
        	event->m_callback2( event->m_retval );
		}
        delete e;
    }

    int calcNetPE( uint32_t pe ) {
		int tmp;
		if ( pe & 1 << 31 ) {
			pe &= ~(1<<31);
			tmp =m_os->getInfo()->getGroup( MP::GroupMachine )->getMapping( pe ) ;
		} else {
			tmp =m_os->getInfo()->getGroup( MP::GroupWorld )->getMapping( pe ) ;
		}
		dbg().debug(CALL_INFO,1,SHMEM_BASE,"%d -> %d\n",pe,tmp);
        return  tmp;
    }

    SST::Link*      m_selfLink;

    Heap* m_heap;

	SimTime_t m_returnLat_ns;
	SimTime_t m_enterLat_ns;
    SimTime_t m_blockingReturnLat_ns;

    ShmemCommon*    m_common;
    ShmemBarrier*   m_barrier;
    ShmemBroadcast* m_broadcast;
    ShmemCollect*   m_collect;
    ShmemFcollect*  m_fcollect;
    ShmemAlltoall*  m_alltoall;
    ShmemAlltoalls* m_alltoalls;
    ShmemReduction* m_reduction;

    int m_num_pes;
    int m_my_pe;
    size_t m_localDataSize;
    Hermes::MemAddr  m_localData;
    Hermes::MemAddr  m_pSync;
    Hermes::MemAddr  m_localScratch;
    Hermes::MemAddr  m_pendingRemoteOps;
    Hermes::Value    m_zero;
};

}
}
#endif
