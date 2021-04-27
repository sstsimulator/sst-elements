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

#include <sst_config.h>

#define CALL_INFO_LAMBDA     __LINE__, __FILE__

#include "hadesSHMEM.h"
#include "functionSM.h"
#include "funcSM/event.h"
#include "shmem/barrier.h"

using namespace SST::Firefly;
using namespace Hermes;


HadesSHMEM::HadesSHMEM(ComponentId_t id, Params& params) :
	Interface(id), m_common( NULL ), m_zero((long)0), m_shmemAddrStart(0x1000)
{
    m_dbg.init("@t:HadesSHMEM::@p():@l ",
        params.find<uint32_t>("verboseLevel",0),
        params.find<uint32_t>("verboseMask",0),
        Output::STDOUT );

    m_selfLink = configureSelfLink("ShmemToDriver", "1 ns",
            new Event::Handler<HadesSHMEM>(this,&HadesSHMEM::doCallback));
    m_heap = new Heap();

	m_enterLat_ns = params.find<int>("enterLat_ns",30);
	m_returnLat_ns = params.find<int>("returnLat_ns",30);
	m_blockingReturnLat_ns = params.find<int>("blockingReturnLat_ns",300);

	Params famMapperParams = params.get_scoped_params( "famNodeMapper" );
	if ( famMapperParams.size() ) {
		m_famNodeMapper = dynamic_cast<FamNodeMapper*>( loadModule( famMapperParams.find<std::string>("name"), famMapperParams ) );
		m_famNodeMapper->setDbg( &m_dbg );
	}

	famMapperParams = params.get_scoped_params( "famAddrMapper" );
	if ( famMapperParams.size() ) {
		m_famAddrMapper = dynamic_cast<FamAddrMapper*>( loadModule( famMapperParams.find<std::string>("name"), famMapperParams ) );
		m_famAddrMapper->setDbg( &m_dbg );
	}
}

HadesSHMEM::~HadesSHMEM() {
	delete m_heap;
	if ( m_common ) {
		delete m_common;
		delete m_barrier;
		delete m_broadcast;
		delete m_collect;
		delete m_fcollect;
		delete m_alltoall;
		delete m_alltoalls;
		delete m_reduction;
	}
}

void HadesSHMEM::setup()
{
    char buffer[100];
    snprintf(buffer,100,"@t:%d:%d:HadesSHMEM::@p():@l ",
                    nic().getRealNodeId(), m_os->getInfo()->worldRank());
    m_dbg.setPrefix(buffer);

	m_memHeapLink = m_os->getMemHeapLink();
	if ( m_memHeapLink ) {
    	dbg().debug(CALL_INFO,1,SHMEM_BASE,"using memHeap\n");
	}
}

void HadesSHMEM::delayEnter( Callback callback, SimTime_t delay )
{
	delay = delay ? delay : m_enterLat_ns;
	m_selfLink->send( delay, new DelayEvent( callback ) );
}

void HadesSHMEM::delayReturn( Shmem::Callback callback, SimTime_t delay )
{
	delay = delay ? delay : m_returnLat_ns;
	m_selfLink->send( delay, new DelayEvent( callback, 0 ) );
}

void HadesSHMEM::delay( Shmem::Callback callback, uint64_t delay, int retval ) {
    m_selfLink->send( delay, new DelayEvent( callback, retval ) );
}

void HadesSHMEM::memcpy( Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, Shmem::Callback callback )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"dest=%#" PRIx64 " src=%#" PRIx64 " length=%zu\n",dest,src,length);
    if ( dest != src ) {
    	::memcpy( m_heap->findBacking(dest), m_heap->findBacking(src), length );
	}
    m_selfLink->send( 0, new HadesSHMEM::DelayEvent( callback, 0 ) );
}

#define DO(name,arg)\
[=]() {\
	if ( this->nic().isBlocked() ) {\
		this->nic().setBlockedCallback(\
			[=](){\
				this->name( arg );\
			}\
		);\
	} else {\
		this->name( arg );\
	}\
}

void HadesSHMEM::init(Shmem::Callback callback)
{
	Init* info = new Init( callback );
	delayEnter( DO(init,info) );
}

void HadesSHMEM::init(Init *info )
{
    m_num_pes = m_os->getInfo()->getGroup(MP::GroupWorld)->getSize();
    m_my_pe = m_os->getInfo()->getGroup(MP::GroupWorld)->getMyRank();
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"my_pe=%d num_pes=%d net_pe=%d\n",m_my_pe,m_num_pes, calcNetPE( m_my_pe ));

    m_common = new ShmemCommon( m_my_pe, m_num_pes, 10, 8 );
    m_barrier = new ShmemBarrier( *this, *m_common );
    m_broadcast = new ShmemBroadcast( *this, *m_common );
    m_collect = new ShmemCollect( *this, *m_common );
    m_fcollect = new ShmemFcollect( *this, *m_common );
    m_alltoall = new ShmemAlltoall( *this, *m_common );
    m_alltoalls = new ShmemAlltoalls( *this, *m_common );
    m_reduction = new ShmemReduction( *this, *m_common );

	// need space for global pSync, scratch for collect and pending remote operations count
    m_localDataSize = sizeof(ShmemCollective::pSync_t) + sizeof(long) * 4;

    malloc( &m_localData, m_localDataSize, true,
            [=]() {
                m_pSync = m_localData;
                m_pSync.at<ShmemCollective::pSync_t>(0) = 0;
                m_pendingPutCnt = m_pSync.offset<ShmemCollective::pSync_t>(1);
				m_pendingPutCnt.at<long>(0) = 0;
                m_pendingGetCnt = m_pendingPutCnt.offset<long>(1);
				m_pendingGetCnt.at<long>(0) = 0;
                m_localScratch = m_pendingGetCnt.offset<long>(1);
				dbg().debug(CALL_INFO,1,SHMEM_BASE,"pSync=%" PRIx64 " PutCnt=%" PRIx64 " , GetCnt=%" PRIx64 "\n",
					   m_pSync.getSimVAddr(), m_pendingPutCnt.getSimVAddr() , m_pendingGetCnt.getSimVAddr() );

                this->nic().shmemInit( m_pendingPutCnt.getSimVAddr(),
                	[=]()  {
						this->delayReturn( info->callback );
						delete info;
					}
				);
            }
    );
}

void HadesSHMEM::finalize(Shmem::Callback callback)
{
	Finalize* info = new Finalize(callback);
	delayEnter( DO( finalize, info ) );
}


void HadesSHMEM::finalize(Finalize* info)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    delayReturn( info->callback );
	delete info;
}

void HadesSHMEM::n_pes(int* val, Shmem::Callback callback )
{
	N_Pes* info = new N_Pes( val, callback );
	delayEnter(
			[=]() {
				this->n_pes( info );
		  	}
	);
}

void HadesSHMEM::n_pes( N_Pes* info )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    *info->val = m_num_pes;
	delayReturn( info->callback );
	delete info;
}

void HadesSHMEM::my_pe(int* val, Shmem::Callback callback )
{
	MyPe* info = new MyPe(val, callback);
	delayEnter(
			[=]() {
				this->my_pe( info );
		  	}
	);
}

void HadesSHMEM::my_pe( MyPe* info )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    *info->val = m_my_pe;
	delayReturn( info->callback );
	delete info;
}

void HadesSHMEM::quiet(Shmem::Callback callback)
{
    put_quiet(
			[=](int){
				get_quiet( callback );
			}
	);
}

void HadesSHMEM::put_quiet(Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
	wait_until( m_pendingPutCnt.getSimVAddr(), Shmem::EQ, m_zero,
            [=](int) {
                dbg().debug(CALL_INFO_LAMBDA,"quiet",1,SHMEM_BASE,"returning\n");
                callback(0);
			}
    );
}

void HadesSHMEM::get_quiet(Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
	wait_until( m_pendingGetCnt.getSimVAddr(), Shmem::EQ, m_zero,
            [=](int) {
                dbg().debug(CALL_INFO_LAMBDA,"quiet",1,SHMEM_BASE,"returning\n");
                callback(0);
			}
    );
}

void HadesSHMEM::fence(Shmem::Callback callback)
{
	Fence* info = new Fence( callback );
	delayEnter( DO(fence,info) );
}

void HadesSHMEM::fence( Fence* info )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    nic().shmemFence(
		[=]() {
            this->dbg().debug(CALL_INFO_LAMBDA,"fence",1,SHMEM_BASE,"returning\n");
			this->delayReturn( info->callback );
			delete info;
		}
	);
}

void HadesSHMEM::malloc( Hermes::MemAddr* ptr, size_t size, bool backed, Shmem::Callback callback )
{
	Malloc* info = new Malloc( ptr, size, backed, callback );
	delayEnter( DO(malloc,info) );
}

void HadesSHMEM::malloc( Malloc* info )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE," maddr ptr=%p size=%lu\n",info->addr,info->length);
    malloc( info->addr, info->length, info->backed,
            [=]() {
				this->delayReturn( info->callback );
				delete info;
            }
    );
}

void HadesSHMEM::malloc( Hermes::MemAddr* ptr, size_t size, bool backed, Callback callback )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE," maddr ptr=%p size=%lu\n",ptr,size);


    if ( m_memHeapLink ) {
        uint64_t sharedAddr = allocSpace(size);
        m_memHeapLink->alloc( size,
        [=](uint64_t addr ) {
                this->dbg().debug(CALL_INFO_LAMBDA,"malloc",1,SHMEM_BASE,"addr=%#" PRIx64 " size=%zu\n",addr,size);
                *ptr = m_heap->addAddr( sharedAddr, size, backed );
                nic().shmemRegMem( *ptr, addr, size, callback);
            }
        );
    } else {
        *ptr =  m_heap->malloc( size, backed );
        nic().shmemRegMem( *ptr, ptr->getSimVAddr(), size, callback) ;
    }
}

void HadesSHMEM::free( Hermes::MemAddr& ptr, Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    assert(0);
}

void HadesSHMEM::barrier_all(Shmem::Callback callback)
{
	Barrier_all* info = new Barrier_all(callback);
	delayEnter(
			[=]() {
				this->barrier_all( info );
		  	}
	);
}

void HadesSHMEM::barrier_all( Barrier_all* info )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");

    m_barrier->start( 0, 0, m_num_pes, m_pSync.getSimVAddr(),
            [=](int) {

    			dbg().debug(CALL_INFO,1,SHMEM_BASE,"barrier_all2() complete\n");
				this->delayReturn( info->callback );
				delete info;
            }
	);
}

void HadesSHMEM::barrier( int start, int stride, int size, Vaddr pSync, Shmem::Callback callback )
{
	Barrier* info = new Barrier( start, stride, size, pSync, callback );
	delayEnter(
			[=]() {
				this->barrier( info );
		  	}
	);
}

void HadesSHMEM::barrier( Barrier* info )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");

    m_barrier->start( info->start, info->stride, info->size, info->pSync,
            [=](int) {
				this->delayReturn( info->callback );
				delete info;
            }
	);
}

void HadesSHMEM::broadcast( Vaddr dest, Vaddr source, size_t nelems, int root, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
	Broadcast* info = new Broadcast( dest, source, nelems, root, start, stride, size, pSync, callback );
	delayEnter(
			[=]() {
				this->broadcast( info );
		  	}
	);
}

void HadesSHMEM::broadcast( Broadcast* info )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    m_broadcast->start( info->dest, info->source, info->nelems, info->root, info->PE_start, info->logPE_stride, info->PE_size, info->pSync,
            [=](int) {
				this->delayReturn( info->callback );
				delete info;
            },
	true );
}

void HadesSHMEM::fcollect( Vaddr dest, Vaddr source, size_t nelems, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
	Fcollect* info = new Fcollect( dest, source, nelems, start, stride, size, pSync, callback );
	delayEnter(
			[=]() {
				this->fcollect( info );
		  	}
	);
}

void HadesSHMEM::fcollect( Fcollect* info )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    m_fcollect->start( info->dest, info->source, info->nelems, info->PE_start, info->logPE_stride, info->PE_size, info->pSync,
            [=](int) {
				this->delayReturn( info->callback );
				delete info;
            }
	);
}

void HadesSHMEM::collect( Vaddr dest, Vaddr source, size_t nelems, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
	Collect* info = new Collect( dest, source, nelems, start, stride, size, pSync, callback );
	delayEnter(
			[=]() {
				this->collect( info );
		  	}
	);
}

void HadesSHMEM::collect( Collect* info )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    m_collect->start( info->dest, info->source, info->nelems, info->PE_start, info->logPE_stride, info->PE_size, info->pSync, &m_localScratch,
            [=](int) {
				this->delayReturn( info->callback );
				delete info;
            }
	);
}

void HadesSHMEM::alltoall( Vaddr dest, Vaddr source, size_t nelems, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
	Alltoall* info = new Alltoall( dest, source, nelems, start, stride, size, pSync, callback );
	delayEnter(
			[=]() {
				this->alltoall( info );
		  	}
	);
}

void HadesSHMEM::alltoall( Alltoall* info )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    m_alltoall->start( info->dest, info->source, info->nelems, info->PE_start, info->logPE_stride, info->PE_size, info->pSync,
            [=](int) {
				this->delayReturn( info->callback );
				delete info;
            }
	);
}

void HadesSHMEM::alltoalls( Vaddr dest, Vaddr source, int dst, int sst, size_t nelems, int elsize, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
	Alltoalls* info = new Alltoalls( dest, source, dst, sst, nelems, elsize, start, stride, size, pSync, callback );
	delayEnter(
			[=]() {
				this->alltoalls( info );
		  	}
	);
}

void HadesSHMEM::alltoalls( Alltoalls* info )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    m_alltoalls->start( info->dest, info->source, info->dst, info->sst, info->nelems, info->elsize, info->PE_start,
		info->logPE_stride, info->PE_size, info->pSync,
            [=](int) {
				this->delayReturn( info->callback );
				delete info;
            }
	);
}

void HadesSHMEM::reduction( Vaddr dest, Vaddr source, int nelems, int PE_start,
                int logPE_stride, int PE_size, Vaddr pSync,
                Hermes::Shmem::ReduOp op, Hermes::Value::Type dataType, Shmem::Callback callback)
{
	Reduction* info = new Reduction( dest, source, nelems, PE_start, logPE_stride, PE_size, pSync, op, dataType, callback );
	delayEnter(
			[=]() {
				this->reduction( info );
		  	}
	);
}

void HadesSHMEM::reduction( Reduction* info )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    m_reduction->start( info->dest, info->source, info->nelems, info->PE_start, info->logPE_stride, info->PE_size, info->pSync,
            info->op, info->type,
            [=](int) {
				this->delayReturn( info->callback );
				delete info;
            }
	);
}

void HadesSHMEM::get(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe, bool blocking, Shmem::Callback callback )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"destSimVAddr=%#" PRIx64 " srcSimVaddr=%#" PRIx64 " length=%lu\n",
                    dest, src, length);

	Get* info = new Get( dest, src, length, pe, blocking, callback, [](int){} );

	delayEnter( DO(get,info) );
}

void HadesSHMEM::get(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe, bool blocking, Shmem::Callback callback, Shmem::Callback& finiCallback )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"destSimVAddr=%#" PRIx64 " srcSimVaddr=%#" PRIx64 " length=%lu\n",
                    dest, src, length);

	Get* info = new Get( dest, src, length, pe, blocking, callback, finiCallback );

	delayEnter( DO(get,info) );
}


void HadesSHMEM::get( Get* info )
{
	Callback callback;
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"destSimVAddr=%#" PRIx64 " srcSimVaddr=%#" PRIx64 " length=%lu blocking=%d\n",
                    info->dest, info->src, info->nelems, info->blocking );

	if ( info->blocking ) {
		callback = [=]() {
            this->dbg().debug(CALL_INFO_LAMBDA,"get",1,SHMEM_BASE,"returning\n");
			this->delayReturn( info->callback, m_blockingReturnLat_ns );
			delete info;
		};
	} else {
		callback = [=]() {
            this->dbg().debug(CALL_INFO_LAMBDA,"get",1,SHMEM_BASE,"returning\n");
            assert( m_pendingGets.find( info ) != m_pendingGets.end() ) ;
            info->finiCallback(0);
            m_pendingGets.erase(info);
            delete info;
		};
	}

    nic().shmemGet( calcNetPE(info->pe), info->dest, info->src, info->nelems, callback );

	if ( ! info->blocking ) {
		this->delayReturn( info->callback, m_returnLat_ns );
		assert( m_pendingGets.find( info ) == m_pendingGets.end() ) ;
        m_pendingGets.insert(info);
	}
}

void HadesSHMEM::getv( Hermes::Value& value, Hermes::Vaddr src, int pe, Shmem::Callback callback)
{
	Getv* info = new Getv( value, src, pe, callback );
	delayEnter( DO(getv,info) );
}

void HadesSHMEM::getv( Getv* info )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"srcSimVaddr=%#" PRIx64 "\n", info->src );

    nic().shmemGetv( calcNetPE(info->pe), info->src, info->result.getType(),

                [=]( Hermes::Value& newValue ) {
                    this->dbg().debug(CALL_INFO_LAMBDA,"getv2",1,SHMEM_BASE,"returning\n");
                    ::memcpy( info->result.getPtr(), newValue.getPtr(), info->result.getLength() );
                   	this->delayReturn( info->callback, m_blockingReturnLat_ns );
					delete info;
                }
            );
}

void HadesSHMEM::put(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe, bool blocking, Shmem::Callback callback)
{
	if ( length == 0 ) {
		delay( callback, 0, 0 );
		return;
	}

	Put* info= new Put( dest, src, length, pe, blocking, callback, [](int){} );
	delayEnter( DO(put,info) );
}

void HadesSHMEM::put(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe, bool blocking, Shmem::Callback callback, Shmem::Callback& fini)
{
	Put* info= new Put( dest, src, length, pe, blocking, callback, fini );
	delayEnter( DO(put,info) );
}

void HadesSHMEM::put( Put* info )
{
	Callback callback;
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");

	// this logic does not make sense and was found during a reorg of hadesSHMEM.cc
	// the resulting timing make may sense so it will stay as is until
	// it can be verified
	if ( info->blocking ) {
		callback = [=]() {
                    this->dbg().debug(CALL_INFO_LAMBDA,"put",1,SHMEM_BASE,"returning\n");
					this->delayReturn( info->callback, m_returnLat_ns  );
					delete info;
				};
	} else {
		callback = [=]() {
                    this->dbg().debug(CALL_INFO_LAMBDA,"put",1,SHMEM_BASE,"returning\n");
					assert( m_pendingPuts.find( info ) != m_pendingPuts.end() ) ;
					info->finiCallback(0);
					m_pendingPuts.erase(info);
					delete info;
				};
	}

    nic().shmemPut( calcNetPE(info->pe), info->dest, info->src, info->nelems, callback  );

	if ( ! info->blocking ) {
		this->delayReturn( info->callback, m_blockingReturnLat_ns );
		assert( m_pendingPuts.find( info ) == m_pendingPuts.end() ) ;
		m_pendingPuts.insert(info);
	}
}

void HadesSHMEM::putOp(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe,
                Hermes::Shmem::ReduOp op, Hermes::Value::Type dataType, Shmem::Callback callback)
{
	PutOp* info = new PutOp( dest, src, length, pe, op, dataType, callback );
	delayEnter( DO( putOp, info ) );
}

void HadesSHMEM::putOp( PutOp* info )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    nic().shmemPutOp( calcNetPE(info->pe), info->dest, info->src, info->nelems, info->op, info->type,
                [=]() {
                    this->dbg().debug(CALL_INFO_LAMBDA,"putOp2",1,SHMEM_BASE,"returning\n");
                    this->delayReturn( info->callback );
					delete info;
                }
            );
}

void HadesSHMEM::putv(Hermes::Vaddr dest, Hermes::Value& value, int pe, Shmem::Callback callback)
{
	Putv* info = new Putv( dest, value, pe, callback );
	delayEnter( DO( putv, info ) );
}

void HadesSHMEM::putv( Putv* info )
{
    std::stringstream tmp;
    tmp << info->value;
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"destSimVaddr=%#" PRIx64 " value=%s\n", info->dest, tmp.str().c_str() );

    nic().shmemPutv( calcNetPE(info->pe), info->dest, info->value );

	delayReturn( info->callback );
	delete info;
}

void HadesSHMEM::wait_until(Hermes::Vaddr addr, Hermes::Shmem::WaitOp op, Hermes::Value& value, Shmem::Callback callback)
{
	WaitUntil* info = new WaitUntil( addr, op, value, callback );

	delayEnter( DO( wait_until, info ) );
}
void HadesSHMEM::wait_until( WaitUntil* info )
{
    std::stringstream tmp;
    tmp << info->value;
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"addr=%#" PRIx64 " val=%s\n", info->addr, tmp.str().c_str());

    nic().shmemWait( info->addr, info->op, info->value,
                [=]() {
                    this->dbg().debug(CALL_INFO_LAMBDA,"wait_until2",1,SHMEM_BASE,"addr=%#" PRIx64 " returning\n",info->addr);
                    this->delayReturn( info->callback );
					delete info;
                }
            );
}

void HadesSHMEM::swap(Hermes::Value& result, Hermes::Vaddr addr, Hermes::Value& value, int pe, Shmem::Callback callback)
{
	Swap* info = new Swap( result, addr, value, pe, callback );
	delayEnter( DO( swap, info ) );
}

void HadesSHMEM::swap( Swap* info )
{
    std::stringstream tmp1;
    tmp1 << info->value;
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"addr=%#" PRIx64 " val=%s\n",info->addr, tmp1.str().c_str());

    nic().shmemSwap( calcNetPE(info->pe), info->addr,  info->value,

                [=]( Hermes::Value& newValue ) {
                    this->dbg().debug(CALL_INFO_LAMBDA,"swap2",1,SHMEM_BASE,"returning\n");

                    ::memcpy( info->result.getPtr(), newValue.getPtr(), info->value.getLength() );

                    this->delayReturn( info->callback, m_blockingReturnLat_ns );
					delete info;
                }
            );
}

void HadesSHMEM::cswap(Hermes::Value& result, Hermes::Vaddr addr, Hermes::Value& cond, Hermes::Value& value, int pe, Shmem::Callback callback)
{
	Cswap* info = new Cswap( result, addr, cond, value, pe, callback );

	delayEnter( DO( cswap, info ) );
}

void HadesSHMEM::cswap( Cswap* info )
{
    std::stringstream tmp1;
    tmp1 << info->value;
    std::stringstream tmp2;
    tmp2 << info->cond;
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"addr=%#" PRIx64 " val=%s cond=%s\n",info->addr, tmp1.str().c_str(),tmp2.str().c_str());

    nic().shmemCswap( calcNetPE(info->pe), info->addr, info->cond, info->value,

                [=]( Hermes::Value& newValue ) {
                    this->dbg().debug(CALL_INFO_LAMBDA,"cswap2",1,SHMEM_BASE,"returning\n");

                    ::memcpy( info->result.getPtr(), newValue.getPtr(), info->value.getLength() );

                    this->delayReturn( info->callback, m_blockingReturnLat_ns );
					delete info;
                }
            );
}

void HadesSHMEM::add( Hermes::Vaddr addr, Hermes::Value& value, int pe, Shmem::Callback callback)
{
	Add* info = new Add( addr, value, pe, callback );

    dbg().debug(CALL_INFO,1,SHMEM_BASE,"addr=%#" PRIx64 "\n",addr);

	delayEnter( DO( add ,info ) );
}

void HadesSHMEM::add( Add* info )
{
    std::stringstream tmp;
    tmp << info->value;
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"addr=%#" PRIx64 " val=%s\n",info->addr, tmp.str().c_str());

    nic().shmemAdd( calcNetPE(info->pe), info->addr, info->value );

    delayReturn( info->callback );
    delete info;
}

void HadesSHMEM::fadd(Hermes::Value& result, Hermes::Vaddr addr, Hermes::Value& value, int pe, Shmem::Callback callback)
{
	Fadd* info = new Fadd( result, addr, value, pe, callback );

	delayEnter( DO( fadd, info ) );
}

void HadesSHMEM::fadd( Fadd* info )
{
    std::stringstream tmp;
    tmp << info->value;
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"addr=%#" PRIx64 " val=%s\n",info->addr, tmp.str().c_str());

    nic().shmemFadd( calcNetPE(info->pe), info->addr, info->value,
                [=]( Hermes::Value& newValue ) {
                    this->dbg().debug(CALL_INFO_LAMBDA,"fadd2",1,SHMEM_BASE,"returning\n");

                    ::memcpy( info->result.getPtr(), newValue.getPtr(), info->result.getLength() );

                    this->delayReturn( info->callback, m_blockingReturnLat_ns );
					delete info;
                }
            );
}

void HadesSHMEM::fam_add( Shmem::Fam_Descriptor fd, uint64_t offset, Hermes::Value& value, Shmem::Callback& callback )
{
	uint64_t localOffset;
	int      node;

	dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");

	getFamNetAddr( offset, node, localOffset );

	Hermes::MemAddr target( localOffset, NULL );

	add( target.getSimVAddr(), value, node, callback );
}

void HadesSHMEM::fam_cswap( Value& result, Shmem::Fam_Descriptor fd, uint64_t offset, Hermes::Value& oldValue, Hermes::Value& newValue, Shmem::Callback callback )
{
	uint64_t localOffset;
	int      node;

	dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");

	getFamNetAddr( offset, node, localOffset );

	Hermes::MemAddr target( localOffset, NULL );

	cswap( result, target.getSimVAddr(), oldValue, newValue, node, callback );
}

void HadesSHMEM::fam_get( Hermes::Vaddr dest, Shmem::Fam_Descriptor fd, uint64_t offset, uint64_t nbytes,
	bool blocking, Shmem::Callback& callback )
{
	if ( blocking ) {
		fam_get2( dest,fd, offset, nbytes, [](int){}, callback );
	} else {
		fam_get2( dest,fd, offset, nbytes, callback, [](int){} );
	}
}

void HadesSHMEM::fam_get2( Hermes::Vaddr dest, Shmem::Fam_Descriptor fd, uint64_t offset, uint64_t nbytes,
	Shmem::Callback callback, Shmem::Callback finiCallback )
{
	FamWork* work = new FamWork( dest, callback, finiCallback );

	m_dbg.debug(CALL_INFO,1,SHMEM_BASE,"dest=%#" PRIx64" globalOffset=%#" PRIx64 " nbytes=%" PRIu64 "\n",
				work->addr, offset, nbytes );

	createWorkList( offset, nbytes, work->work );
	work->pending = work->work.size();
	doOneFamGet( work );
}

void HadesSHMEM::doOneFamGet( FamWork* work ) {
	uint64_t localOffset;
	int      node;

	getFamNetAddr( work->work.front().first, node, localOffset );

	Hermes::MemAddr target( localOffset, NULL );

	Hermes::Vaddr dest = work->addr;
	uint64_t nbytes = work->work.front().second;
	Shmem::Callback callback;
	Shmem::Callback finiCallback =
				[=](int){
					--work->pending;
                	dbg().debug(CALL_INFO_LAMBDA,"doOneFamGet",1,SHMEM_BASE,"pending=%zu\n",work->pending);
					if ( 0 == work->pending ) {
						work->finiCallback(0);
						delete work;
					}
				};

	work->addr += work->work.front().second;
	work->work.pop();

	if ( work->work.empty() ) {
		m_dbg.debug(CALL_INFO,1,SHMEM_BASE,"all work issued\n");
		callback = work->callback;
	} else {
		callback = [=](int) {
			doOneFamGet(work);
	  	};
	}

	m_dbg.debug(CALL_INFO,1,SHMEM_BASE,"dest=%#" PRIx64" target=%#" PRIx64 " nbytes=%" PRIu64 " node=%x\n",
				dest, target.getSimVAddr() , nbytes, node );

	get( dest, target.getSimVAddr(), nbytes, node, false, callback, finiCallback );
}

void HadesSHMEM::fam_put( Shmem::Fam_Descriptor fd, uint64_t offset, Hermes::Vaddr src, uint64_t nbytes,
	bool blocking, Shmem::Callback& callback )
{
	if ( blocking ) {
		fam_put2( fd, offset, src, nbytes, [](int){}, callback );
	} else {
		fam_put2( fd, offset, src, nbytes, callback, [](int){} );
	}
}

void HadesSHMEM::fam_put2( Shmem::Fam_Descriptor fd, uint64_t offset, Hermes::Vaddr src, uint64_t nbytes,
	Shmem::Callback callback, Shmem::Callback finiCallback )
{
	FamWork* work = new FamWork( src, callback, finiCallback );

	m_dbg.debug(CALL_INFO,1,SHMEM_BASE,"src=%#" PRIx64" globalOffset=%#" PRIx64 " nbytes=%" PRIu64 "\n",
				work->addr, offset, nbytes );

	createWorkList( offset, nbytes, work->work );
	work->pending = work->work.size();
	doOneFamPut( work );
}

void HadesSHMEM::doOneFamPut( FamWork* work ) {
	uint64_t localOffset;
	int      node;

	getFamNetAddr( work->work.front().first, node, localOffset );

	Hermes::MemAddr target( localOffset, NULL );

	Hermes::Vaddr src = work->addr;
	uint64_t nbytes = work->work.front().second;
	Shmem::Callback callback;
	Shmem::Callback finiCallback =
				[=](int){
					--work->pending;
                	dbg().debug(CALL_INFO_LAMBDA,"doOneFamPut",1,SHMEM_BASE,"pending=%zu\n",work->pending);
					if ( 0 == work->pending ) {
						work->finiCallback(0);
						delete work;
					}
				};

	work->addr += work->work.front().second;
	work->work.pop();

	if ( work->work.empty() ) {
		m_dbg.debug(CALL_INFO,1,SHMEM_BASE,"all work issued\n");
		callback = work->callback;
	} else {
		callback = [=](int) {
			doOneFamPut(work);
	  	};
	}

	m_dbg.debug(CALL_INFO,1,SHMEM_BASE,"src=%#" PRIx64" target=%#" PRIx64 " nbytes=%" PRIu64 " node=%x\n",
				src, target.getSimVAddr() , nbytes, node );

	put( target.getSimVAddr(), src, nbytes, node, false, callback, finiCallback );
}

void HadesSHMEM::fam_scatter( Hermes::Vaddr src, Shmem::Fam_Descriptor fd, uint64_t nElements,
	uint64_t firstElement, uint64_t stride, uint64_t elementSize, bool blocking, Shmem::Callback& callback )
{
	m_dbg.debug(CALL_INFO,1,SHMEM_BASE,"src=%#" PRIx64 "nElements=%" PRIu64 " elementSize=%" PRIu64 " %s\n",
			src, nElements, elementSize, blocking ? "blocking" : "" );

	FamVectorWork* work = new FamVectorWork;
	work->firstElement = firstElement;
	work->stride = stride;

	fam_scatter( work, src, fd, nElements, elementSize, blocking, callback );
}

void HadesSHMEM::fam_scatterv( Hermes::Vaddr src, Shmem::Fam_Descriptor fd, uint64_t nElements,
	std::vector<uint64_t> elementIndexes, uint64_t elementSize, bool blocking, Shmem::Callback& callback )
{
	m_dbg.debug(CALL_INFO,1,SHMEM_BASE,"src=%#" PRIx64 "nElements=%" PRIu64 " elementSize=%" PRIu64 " %s\n",
			src, nElements, elementSize, blocking ? "blocking" : "" );

	FamVectorWork* work = new FamVectorWork;
	work->indexes = elementIndexes;

	fam_scatter( work, src, fd, nElements, elementSize, blocking, callback );
}

void HadesSHMEM::fam_scatter( FamVectorWork* work, Hermes::Vaddr src, Shmem::Fam_Descriptor fd, uint64_t nElements,
	uint64_t elementSize, bool blocking, Shmem::Callback& callback )
{
	work->addr = src;
	work->fd =fd;
	work->nElements = nElements;
	work->pending = nElements;
	work->elementSize = elementSize;
	work->currentVector = 0;
	work->blocking = blocking;

	if ( work->blocking ) {
		work->callback = [](int){};;
		work->finiCallback = callback;
	} else {
		work->callback = callback;
		work->finiCallback = [](int){};
	}

	doFamVectorPut( work );
}


void HadesSHMEM::doFamVectorPut( FamVectorWork* work )
{
	uint64_t offset;
	int index = work->currentVector++;
	Shmem::Callback callback;
	Shmem::Callback finiCallback =
		[=](int){
			--work->pending;
           	dbg().debug(CALL_INFO_LAMBDA,"doFamVectorPut",1,SHMEM_BASE,"pending=%zu\n",work->pending);
			if (  0 == work->pending ) {
           		dbg().debug(CALL_INFO_LAMBDA,"doFamVectorPut",1,SHMEM_BASE,"callback\n");
				work->finiCallback(0);
				delete work;
			}
		};

	if ( work->currentVector == work->nElements ) {
		callback = work->callback;
	} else {
		callback = [=](int) {
			doFamVectorPut( work );
		};
	}

	if ( ! work->indexes.empty() ) {
		offset = work->indexes[index] * work->elementSize;
	} else {
		offset = work->firstElement + index * work->stride * work->elementSize;
	}

	m_dbg.debug(CALL_INFO,1,SHMEM_BASE,"src=%#" PRIx64 " offset=%" PRIu64 " size=%" PRIu64 "\n",
			work->addr, offset, work->elementSize );

	fam_put2( work->fd, offset, work->addr, work->elementSize, callback, finiCallback );
}

void HadesSHMEM::fam_gather( Hermes::Vaddr dest, Shmem::Fam_Descriptor fd, uint64_t nElements,
	uint64_t firstElement, uint64_t stride, uint64_t elementSize, bool blocking, Shmem::Callback& callback )
{
	m_dbg.debug(CALL_INFO,1,SHMEM_BASE,"dest=%#" PRIx64 "nElements=%" PRIu64 " elementSize=%" PRIu64 " %s\n",
		dest, nElements, elementSize, blocking ? "blocking":"" );

	FamVectorWork* work = new FamVectorWork;
	work->firstElement = firstElement;
	work->stride = stride;
	fam_gather( work, dest, fd, nElements, elementSize, blocking, callback );
}

void HadesSHMEM::fam_gatherv( Hermes::Vaddr dest, Shmem::Fam_Descriptor fd, uint64_t nElements,
	std::vector<uint64_t> elementIndexes, uint64_t elementSize, bool blocking, Shmem::Callback& callback )
{
	m_dbg.debug(CALL_INFO,1,SHMEM_BASE,"dest=%#" PRIx64 "nElements=%" PRIu64 " elementSize=%" PRIu64 " %s\n",
		dest, nElements, elementSize,  blocking ? "blocking" : "" );

	FamVectorWork* work = new FamVectorWork;
	work->indexes = elementIndexes;
	fam_gather( work, dest, fd, nElements, elementSize, blocking, callback );
}


void HadesSHMEM::fam_gather( FamVectorWork* work, Hermes::Vaddr dest, Shmem::Fam_Descriptor fd, uint64_t nElements,
	uint64_t elementSize, bool blocking, Shmem::Callback& callback )
{
	work->addr = dest;
	work->fd = fd;
	work->nElements = nElements;
	work->pending = nElements;
	work->elementSize = elementSize;
	work->currentVector = 0;
	work->blocking = blocking;

	if ( work->blocking ) {
		work->callback = [](int){};;
		work->finiCallback = callback;
	} else {
		work->callback = callback;
		work->finiCallback = [](int){};
	}

	doFamVectorGet( work );
}

void HadesSHMEM::doFamVectorGet( FamVectorWork* work )
{
	uint64_t offset;
	int index = work->currentVector++;
	Shmem::Callback callback;
	Shmem::Callback finiCallback =
		[=](int){
			--work->pending;
           	dbg().debug(CALL_INFO_LAMBDA,"doFamVectorGet",1,SHMEM_BASE,"pending=%zu\n",work->pending);
			if (  0 == work->pending ) {
           		dbg().debug(CALL_INFO_LAMBDA,"doFamVectorGet",1,SHMEM_BASE,"callback\n");
				work->finiCallback(0);
				delete work;
			}
		};

	if ( work->currentVector == work->nElements ) {
		m_dbg.debug(CALL_INFO,1,SHMEM_BASE,"all gets made\n");
		callback = work->callback;
	} else {
		m_dbg.debug(CALL_INFO,1,SHMEM_BASE,"sched next vector\n");
		callback = [=](int) {
			doFamVectorGet( work );
		};
	}

	if ( ! work->indexes.empty() ) {
		offset = work->indexes[index] * work->elementSize;
	} else {
		offset = work->firstElement + index * work->stride * work->elementSize;
	}

	m_dbg.debug(CALL_INFO,1,SHMEM_BASE,"dest=%#" PRIx64 " offset=%" PRIu64 " size=%" PRIu64 "\n", work->addr, offset, work->elementSize );
	fam_get2( work->addr + index* work->elementSize, work->fd, offset, work->elementSize, callback, finiCallback );
}
