// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
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

#include <sst/core/module.h>

#include <sst/core/params.h>

#include <queue>
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
#include "shmem/famAddrMapper.h"
#include "shmem/famNodeMapper.h"

#define SHMEM_BASE      1<<0
#define SHMEM_BARRIER   1<<1

using namespace Hermes;

namespace SST {
namespace Firefly {

class HadesSHMEM : public Shmem::Interface
{
  public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        HadesSHMEM,
        "firefly",
        "hadesSHMEM",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        SST::Hermes::Interface
    )
    SST_ELI_DOCUMENT_PARAMS(
        {"verboseLevel","Sets the level of debug verbosity",""},
        {"verboseMask","Sets the debug mask",""},
        {"enterLat_ns","Sets the latency of entering a SHMEM call","" },
        {"returnLat_ns","Sets the latency of returning from a SHMEM call","" },
        {"blockingReturnLat_ns","Sets the latency of returning from a SHMEM call that blocked on response","" },

		/* PARAMS passed to another module
			famNodeMapper.*
			famAddrMapper.*
		*/
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


			addAddr( addr, n, backed );
            return addr;
        }

		void addAddr( Hermes::MemAddr& addr, size_t n, bool backed ) {
            if ( backed ) {
                addr.setBacking( ::malloc(n) );
            }
            m_map[ addr.getSimVAddr() ] = Entry( addr, n );

		}

        void free( Hermes::MemAddr& addr ) {
            if ( addr.getBacking() ) {
                ::free( addr.getBacking() );
            }
            m_map.erase( addr.getSimVAddr() );
        }

		Hermes::MemAddr addAddr( uint64_t addr, size_t n, bool backed ) {
            Hermes::MemAddr memAddr( Hermes::MemAddr::Shmem );
            memAddr.setSimVAddr( addr );
			addAddr( memAddr, n, backed );
			return memAddr;
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

	struct Base {
		Base( Shmem::Callback callback ) :  callback( callback ) {}
		Shmem::Callback callback;
	};
	struct Init : public Base {
		Init( Shmem::Callback callback ) : Base(callback) {}
	};
	struct Finalize : public Base {
		Finalize( Shmem::Callback callback ) : Base(callback) {}
	};
	struct N_Pes : public Base {
		N_Pes( int* val, Shmem::Callback callback ) : Base(callback), val( val ) {}
		int* val;
	};
	struct MyPe : public Base {
		MyPe( int* val, Shmem::Callback callback ) : Base(callback), val( val ) {}
		int* val;
	};
	struct Fence : public Base {
		Fence( Shmem::Callback callback ) : Base(callback) {}
	};
	struct Quiet : public Base {
		Quiet( Shmem::Callback callback ) : Base(callback) {}
	};
	struct Barrier_all : public Base {
		Barrier_all( Shmem::Callback callback ) : Base(callback) {}
	};
	struct Barrier : public Base {
		Barrier( int start, int stride, int size, Vaddr addr, Shmem::Callback callback ) :
			Base(callback), start(start), stride(stride), size(size), pSync(addr) {}
		int start;
		int stride;
		int size;
		Vaddr pSync;
	};
	struct Broadcast : public Base {
		Broadcast( Vaddr dest, Vaddr source, size_t nelems, int root, int PE_start,
            int logPE_stride, int PE_size, Vaddr addr, Shmem::Callback callback ) :
			Base(callback), dest(dest), source(source), nelems(nelems), root(root), PE_start(PE_start),
	   		logPE_stride(logPE_stride), PE_size(PE_size), pSync(addr)	{}
		Vaddr dest;
		Vaddr source;
		size_t nelems;
		int root;
		int PE_start;
        int logPE_stride;
		 int PE_size;
		Vaddr pSync;
	};
	struct Fcollect : public Base {
		Fcollect( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
            int logPE_stride, int PE_size, Vaddr addr, Shmem::Callback callback ) :
			Base(callback), dest(dest), source(source), nelems(nelems), PE_start(PE_start),
	  		logPE_stride(logPE_stride), PE_size(PE_size), pSync(addr)	{}
		Vaddr dest;
		Vaddr source;
		size_t nelems;
	   	int PE_start;
        int logPE_stride;
	   	int PE_size;
	   	Vaddr pSync;
	};
	struct Collect : public Base {
		Collect( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
            int logPE_stride, int PE_size, Vaddr pSync, Shmem::Callback callback ) :
			Base(callback), dest(dest), source(source), nelems(nelems), PE_start(PE_start),
	   		logPE_stride(logPE_stride), PE_size(PE_size), pSync(pSync)	{}
		Vaddr dest;
		Vaddr source;
		size_t nelems;
		int PE_start;
        int logPE_stride;
		int PE_size;
		Vaddr pSync;
	};

	struct Alltoall : public Base {
		Alltoall( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
            int logPE_stride, int PE_size, Vaddr pSync, Shmem::Callback callback ) :
			Base(callback), dest(dest), source(source), nelems(nelems), PE_start(PE_start),
	   		logPE_stride(logPE_stride), PE_size(PE_size), pSync(pSync) {}
		Vaddr dest;
		Vaddr source;
	   	size_t nelems;
		int PE_start;
        int logPE_stride;
	   	int PE_size;
		Vaddr pSync;
	};

	struct Alltoalls : public Base {
		Alltoalls( Vaddr dest, Vaddr source, int dst, int sst, size_t nelems, int elsize,
            int PE_start, int logPE_stride, int PE_size, Vaddr pSync, Shmem::Callback callback ) :
			Base(callback), dest(dest), source(source), dst(dst), sst(sst), nelems(nelems),
	   		elsize(elsize), PE_start(PE_start), logPE_stride(logPE_stride), PE_size(PE_size),
			pSync(pSync)	{}
		Vaddr dest;
		Vaddr source;
		int dst;
		int sst;
		size_t nelems;
	   	int elsize;
        int PE_start;
	   	int logPE_stride;
	   	int PE_size;
		Vaddr pSync;
	};
	struct Reduction : public Base {
		Reduction( Vaddr dest, Vaddr source, int nelems, int PE_start, int logPE_stride,
			int PE_size, Vaddr pSync, Shmem::ReduOp op, Hermes::Value::Type type, Shmem::Callback callback ) :
			Base(callback), dest(dest), source(source), nelems(nelems), PE_start(PE_start),
			logPE_stride(logPE_stride), PE_size(PE_size), pSync(pSync), op(op), type(type) {}
		Vaddr dest;
		Vaddr source;
		int nelems;
		int PE_start;
        int logPE_stride;
		int PE_size;
		Vaddr pSync;
        Hermes::Shmem::ReduOp op ;
	   	Hermes::Value::Type type;
	};

	struct  Malloc : public Base {
		Malloc( MemAddr* addr, size_t length, bool backed, Shmem::Callback callback ) :
			Base(callback), addr(addr), length(length), backed(backed) {}
		Hermes::MemAddr* addr ;
		size_t length;
		bool backed;
	};

	struct Free : public Base {
		Free( MemAddr& addr, Shmem::Callback callback ) :
			Base(callback), addr(addr)  {}
		Hermes::MemAddr addr;
	};

	struct Getv : public Base {
		Getv( Value& result, Vaddr src, int pe, Shmem::Callback callback ) :
			Base(callback), result(result), src(src), pe(pe) {}
		Hermes::Value result;
	   	Hermes::Vaddr src;
		int pe;
	};

	struct Get : public Base {
		Get( Vaddr dest, Vaddr src, size_t nelems, int pe, bool blocking, Shmem::Callback callback, Shmem::Callback fini ) :
			Base(callback), dest(dest), src(src), nelems(nelems), pe(pe), blocking(blocking), finiCallback(fini)  {}
		Hermes::Vaddr dest;
		Hermes::Vaddr src;
		size_t nelems;
	   	int pe;
		bool blocking;
		Shmem::Callback finiCallback;
	};
	struct Putv  : public Base {
		Putv( Vaddr dest, Value& value, int pe, Shmem::Callback callback ) :
			Base(callback), dest(dest), value(value), pe(pe) {}
		Hermes::Vaddr dest;
	   	Hermes::Value value;
	   	int pe;
	};
	struct Put : public Base {
		Put( Vaddr dest, Vaddr src, size_t nelems, int pe, bool blocking, Shmem::Callback callback, Shmem::Callback fini ) :
			Base(callback), dest(dest), src(src), nelems(nelems), pe(pe), blocking(blocking), finiCallback(fini) {}
		Hermes::Vaddr dest;
	   	Hermes::Vaddr src;
		size_t nelems;
		int pe;
		bool blocking;
		Shmem::Callback finiCallback;
	};
	struct PutOp : public Base {
    	PutOp(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe,
				Hermes::Shmem::ReduOp op, Hermes::Value::Type type, Shmem::Callback callback) :
			Base(callback), dest(dest), src(src), nelems(nelems), pe(pe), op(op), type(type) {}

    	Hermes::Vaddr dest;
	 	Hermes::Vaddr src;
	   	size_t nelems;
	   	int pe;
        Hermes::Shmem::ReduOp op;
		Hermes::Value::Type type;
	};

	struct WaitUntil : public Base {
		WaitUntil( Vaddr addr, Shmem::WaitOp op, Value& value, Shmem::Callback callback ) :
			Base(callback), addr(addr), op(op), value(value) {}
		Hermes::Vaddr addr;
		Hermes::Shmem::WaitOp op;
		Hermes::Value value;
	};
	struct Cswap : public Base {
		Cswap( Value& result, Vaddr addr, Value& cond, Value& value, int pe, Shmem::Callback callback ) :
			Base(callback), result(result), addr(addr), cond(cond), value(value), pe(pe)  {}
		Value result;
	   	Vaddr addr;
	   	Value cond;
		Value value;
		int pe;
	};
	struct Swap : public Base {
		Swap( Value& result, Vaddr addr, Value& value, int pe, Shmem::Callback callback ) :
			Base(callback), result(result), addr(addr), value(value), pe(pe)  {}
		Value result;
		Vaddr addr;
	   	Value value;
		int pe;
	};
	struct Fadd : public Base {
		Fadd( Value& result, Vaddr addr, Value& value, int pe, Shmem::Callback callback ) :
			Base(callback), result(result), addr(addr), value(value), pe(pe) {}
		Value result;
		Vaddr addr;
		Value value;
        int pe;
	};
	struct Add : public Base {
		Add( Vaddr addr, Value& value , int pe, Shmem::Callback callback ) :
			Base(callback), addr(addr), value(value), pe(pe) {}
		Vaddr addr;
		Value value;
	   	int pe;
	};
	struct Fam_Get : public Base {
		Fam_Get( Hermes::Vaddr dest, Shmem::Fam_Descriptor rd, uint64_t offset, uint64_t nbytes,
				bool blocking, Shmem::Callback callback ) :
			Base(callback), dest(dest), rd(rd), offset(offset), nbytes(nbytes), blocking(blocking)  {}
		Hermes::Vaddr dest;
		Shmem::Fam_Descriptor rd;
		uint64_t offset;
	   	uint64_t nbytes;
		bool blocking;
	};
	struct Fam_Add : public Base {
		Fam_Add( uint64_t addr, Value& value, Shmem::Callback callback ) :
			Base(callback), addr(addr), value(value) {}
		uint64_t addr;
	   	Value value;
	};

  public:
    HadesSHMEM(ComponentId_t, Params&);
    ~HadesSHMEM();

    virtual void setup();
    virtual void finish() {}
    virtual std::string getName() { return "HadesSHMEM"; }
    virtual std::string getType() { return "shmem"; }

    virtual void setOS( OS* os ) {
        m_os = static_cast<Hades*>(os);
        dbg().debug(CALL_INFO,2,0,"\n");
    }

    virtual void init(Shmem::Callback);
    virtual void finalize(Shmem::Callback);

    virtual void n_pes(int*, Shmem::Callback);
    virtual void my_pe(int*, Shmem::Callback);

    virtual void fence(Shmem::Callback);
    virtual void quiet(Shmem::Callback);

    virtual void barrier_all(Shmem::Callback);
    virtual void barrier( int start, int stride, int size, Vaddr, Shmem::Callback);

    virtual void broadcast( Vaddr dest, Vaddr source, size_t nelems, int root, int PE_start,
                        int logPE_stride, int PE_size, Vaddr, Shmem::Callback);
    virtual void fcollect( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
                        int logPE_stride, int PE_size, Vaddr, Shmem::Callback);
    virtual void collect( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
                        int logPE_stride, int PE_size, Vaddr, Shmem::Callback);
    virtual void alltoall( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
                        int logPE_stride, int PE_size, Vaddr, Shmem::Callback);
    virtual void alltoalls( Vaddr dest, Vaddr source, int dst, int sst, size_t nelems, int elsize,
                        int PE_start, int logPE_stride, int PE_size, Vaddr, Shmem::Callback);
    virtual void reduction( Vaddr dest, Vaddr source, int nelems, int PE_start,
                        int logPE_stride, int PE_size, Vaddr pSync,
                        Hermes::Shmem::ReduOp, Hermes::Value::Type, Shmem::Callback);

    virtual void malloc(Hermes::MemAddr*,size_t,bool backed, Shmem::Callback);
    virtual void free(Hermes::MemAddr&,Shmem::Callback);

    virtual void getv(Hermes::Value&, Hermes::Vaddr src, int pe, Shmem::Callback);
    virtual void get(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe, bool blocking, Shmem::Callback);

    virtual void putv(Hermes::Vaddr dest, Hermes::Value&, int pe, Shmem::Callback);
    virtual void put(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe, bool blocking, Shmem::Callback);

    virtual void putOp(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe,
            Hermes::Shmem::ReduOp, Hermes::Value::Type, Shmem::Callback);

    virtual void wait_until(Hermes::Vaddr src, Hermes::Shmem::WaitOp, Hermes::Value&, Shmem::Callback);

    virtual void cswap( Hermes::Value& result, Hermes::Vaddr, Hermes::Value& cond, Hermes::Value& value, int pe, Shmem::Callback);

    virtual void swap( Hermes::Value& result, Hermes::Vaddr, Hermes::Value& value, int pe, Shmem::Callback);
    virtual void add(  Hermes::Vaddr, Hermes::Value&, int pe, Shmem::Callback);
    virtual void fadd( Hermes::Value&, Hermes::Vaddr, Hermes::Value&, int pe, Shmem::Callback);

	virtual void fam_add( Shmem::Fam_Descriptor fd, uint64_t, Hermes::Value&, Shmem::Callback& );
    virtual void fam_cswap( Hermes::Value& result, Shmem::Fam_Descriptor fd, uint64_t, Hermes::Value& oldValue , Hermes::Value& newValue, Shmem::Callback);

    virtual void fam_get( Hermes::Vaddr dest, Shmem::Fam_Descriptor fd, uint64_t offset, uint64_t nbytes,
			bool blocking, Shmem::Callback&);

    virtual void fam_put( Shmem::Fam_Descriptor fd, uint64_t offset, Hermes::Vaddr src, uint64_t nbytes,
			bool blocking, Shmem::Callback&);

	virtual void fam_scatter( Hermes::Vaddr src, Shmem::Fam_Descriptor rd, uint64_t nElements,
			uint64_t firstElement, uint64_t stride, uint64_t elementSize, bool blocking, Shmem::Callback& );
    virtual void fam_scatterv( Hermes::Vaddr src, Shmem::Fam_Descriptor fd, uint64_t nElements,
			std::vector<uint64_t> elementIndex, uint64_t elementSize, bool blocking, Shmem::Callback& );

    virtual void fam_gather( Hermes::Vaddr dest, Shmem::Fam_Descriptor fd, uint64_t nElements,
			uint64_t firstElement, uint64_t stride, uint64_t elmentSize, bool blocking, Shmem::Callback& );
    virtual void fam_gatherv( Hermes::Vaddr dest, Shmem::Fam_Descriptor fd, uint64_t nElements,
			std::vector<uint64_t> elementIndex, uint64_t elmentSize, bool blocking, Shmem::Callback& );

    void memcpy( Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, Shmem::Callback callback );

    void delay( Shmem::Callback callback, uint64_t delay, int retval );

    void* getBacking( Hermes::Vaddr addr ) {
        return m_heap->findBacking(addr);
    }

    Output& dbg() { return m_dbg; }
  private:
    virtual void put_quiet(Shmem::Callback);
    virtual void get_quiet(Shmem::Callback);
    virtual void get(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe, bool blocking, Shmem::Callback, Shmem::Callback& fini );
    virtual void put(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe, bool blocking, Shmem::Callback, Shmem::Callback& fini );

    virtual void fam_put2( Shmem::Fam_Descriptor fd, uint64_t offset, Hermes::Vaddr src, uint64_t nbytes, Shmem::Callback, Shmem::Callback );
    virtual void fam_get2( Hermes::Vaddr dest, Shmem::Fam_Descriptor fd, uint64_t offset, uint64_t nbytes, Shmem::Callback, Shmem::Callback );

	void init( Init* );
	void finalize( Finalize* );
	void n_pes( N_Pes* );
	void my_pe( MyPe* );
	void fence( Fence* );
	void barrier_all( Barrier_all* );
	void barrier( Barrier* );
	void broadcast( Broadcast* );
	void fcollect( Fcollect * );
	void collect( Collect * );
	void alltoall( Alltoall* );
	void alltoalls( Alltoalls* );
	void reduction( Reduction * );
	void malloc( Malloc * );
	void free( Free* );
	void getv( Getv* );
	void get( Get* );
	void putv( Putv* );
	void put( Put* );
	void putOp( PutOp* );
	void wait_until( WaitUntil* );
	void cswap( Cswap* );
	void swap( Swap* );
	void fadd( Fadd* );
	void add( Add* );
	void fam_get( Fam_Get* );
	void fam_add( Fam_Add* );

    virtual void malloc(Hermes::MemAddr*,size_t,bool backed,Callback);
    Output m_dbg;
    Hades*      m_os;

	void delayEnter( Callback callback, SimTime_t delay = 0 );
	void delayEnter( Shmem::Callback callback, SimTime_t delay = 0 );
	void delayReturn( Shmem::Callback callback, SimTime_t delay = 0 );

    void doCallback(SST::Event* e) {
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
			tmp = pe;
		} else {
			tmp =m_os->getInfo()->getGroup( MP::GroupWorld )->getMapping( pe ) ;
		}
		dbg().debug(CALL_INFO,2,SHMEM_BASE,"%x -> %x\n",pe,tmp);
        return  tmp;
    }

 	void getFamNetAddr( uint64_t globalOffset, int& node, uint64_t& localOffset ) {
	    if( ! m_famNodeMapper || ! m_famAddrMapper ) {
			dbg().fatal(CALL_INFO, -1,"FAM mapping module not configured\n");
		}
    	m_famAddrMapper->getAddr( globalOffset, node, localOffset );
		node = m_famNodeMapper->calcNode( node );
		node |= 1<<31;
	}

	struct FamWork {
		FamWork( Hermes::Vaddr addr, Shmem::Callback callback, Shmem::Callback finiCallback) :
			addr(addr),callback(callback),finiCallback(finiCallback) {}
    	std::queue< std::pair< uint64_t, uint64_t > > work;
		Hermes::Vaddr addr;
    	Shmem::Callback callback;
    	Shmem::Callback finiCallback;
		size_t pending;
	};

	void doOneFamGet( FamWork* );
	void doOneFamPut( FamWork* );

	struct FamVectorWork {
		Hermes::Vaddr addr;
		Shmem::Fam_Descriptor fd;
		uint64_t nElements;
		std::vector<uint64_t> indexes;
		uint64_t firstElement;
		uint64_t stride;
		uint64_t elementSize;
		Shmem::Callback callback;
		int currentVector;
		bool blocking;
		size_t pending;
		Shmem::Callback finiCallback;
	};
	void fam_gather( FamVectorWork* work, Hermes::Vaddr dest, Shmem::Fam_Descriptor fd, uint64_t nElements,
    		uint64_t elementSize, bool blocking, Shmem::Callback& callback );
	void fam_scatter( FamVectorWork* work, Hermes::Vaddr src, Shmem::Fam_Descriptor fd, uint64_t nElements,
    		uint64_t elementSize, bool blocking, Shmem::Callback& callback );

	void doFamVectorPut( FamVectorWork* );
	void doFamVectorGet( FamVectorWork* );

	void createWorkList( uint64_t addr, uint64_t nbytes, std::queue< std::pair< uint64_t, uint64_t > >& list ) {
	    if( ! m_famAddrMapper ) {
			dbg().fatal(CALL_INFO, -1,"FAM mapping module not configured\n");
		}
		uint32_t blockSize = m_famAddrMapper->blockSize();
		dbg().debug(CALL_INFO,2,SHMEM_BASE,"addr=%#" PRIx64 " nbytes=%" PRIu64"\n", addr, nbytes);

		while ( nbytes ) {
			uint64_t alignment = addr & (blockSize-1);
			uint64_t seg_nbytes = blockSize;

			if ( alignment ) {
				seg_nbytes -= alignment;
			} else {
				if ( nbytes < blockSize ) {
					seg_nbytes = nbytes;
				}
			}

			dbg().debug(CALL_INFO,3,SHMEM_BASE,"seg_addr=%#" PRIx64 " seg_nbytes=%" PRIu64"\n", addr, seg_nbytes);

			list.push( std::make_pair(addr,seg_nbytes) );

			nbytes -= seg_nbytes;
			addr += seg_nbytes;
		}
	}

	VirtNic& nic() {
		return *m_os->getNic();
	}

    SST::Link*      m_selfLink;

    Heap* m_heap;

	SimTime_t m_returnLat_ns;
	SimTime_t m_enterLat_ns;
    SimTime_t m_blockingReturnLat_ns;

	uint64_t m_shmemAddrStart;

	uint64_t allocSpace( size_t length ) {
		uint64_t base = m_shmemAddrStart;
        m_shmemAddrStart += length;
        // allign on cache line
        m_shmemAddrStart += 64;
        m_shmemAddrStart &= ~(64-1);
		return base;
	}

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
    Hermes::MemAddr  m_pendingPutCnt;
    Hermes::MemAddr  m_pendingGetCnt;
    Hermes::Value    m_zero;

	FamNodeMapper* m_famNodeMapper;
	FamAddrMapper* m_famAddrMapper;
	Thornhill::MemoryHeapLink* m_memHeapLink;

	std::set< Put* > m_pendingPuts;
	std::set< Get* > m_pendingGets;
};

}
}
#endif
