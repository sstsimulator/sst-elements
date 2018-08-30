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

#include <sst_config.h>

#define CALL_INFO_LAMBDA     __LINE__, __FILE__

#include "hadesSHMEM.h"
#include "functionSM.h"
#include "funcSM/event.h"
#include "shmem/barrier.h"

using namespace SST::Firefly;
using namespace Hermes;

HadesSHMEM::HadesSHMEM(Component* owner, Params& params) :
	Interface(owner), m_common( NULL ), m_zero((long)0)
{
    m_dbg.init("@t:HadesSHMEM::@p():@l ",
        params.find<uint32_t>("verboseLevel",0),
        params.find<uint32_t>("verboseMask",0),
        Output::STDOUT );

    m_selfLink = configureSelfLink("ShmemToDriver", "1 ns",
            new Event::Handler<HadesSHMEM>(this,&HadesSHMEM::handleToDriver));
    m_heap = new Heap();

	m_enterLat_ns = params.find<int>("enterLat_ns",30);
	m_returnLat_ns = params.find<int>("returnLat_ns",30);
	m_blockingReturnLat_ns = params.find<int>("blockingReturnLat_ns",300);
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
                    m_os->getNic()->getRealNodeId(), m_os->getInfo()->worldRank());
    m_dbg.setPrefix(buffer);
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

void HadesSHMEM::init(Shmem::Callback callback)
{
	delayEnter(
			[=]() {
				this->init2( callback );
		  	}
	);
}

void HadesSHMEM::init2(Shmem::Callback callback)
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
    m_localDataSize = sizeof(ShmemCollective::pSync_t) + sizeof(long) * 3;

    malloc( &m_localData, m_localDataSize, true, 
            [=]() { 
                m_pSync = m_localData;
                m_pSync.at<ShmemCollective::pSync_t>(0) = 0;
                m_pendingRemoteOps = m_pSync.offset<ShmemCollective::pSync_t>(1);
				m_pendingRemoteOps.at<long>(0) = 0;
                m_localScratch = m_pendingRemoteOps.offset<long>(1);

            	this->m_os->getNic()->shmemInit( m_pendingRemoteOps.getSimVAddr(), 
                	[=]()  {
						this->delayReturn( callback );
					} 
				);
            }
    );
}

void HadesSHMEM::finalize(Shmem::Callback callback)
{
	delayEnter(
			[=]() {
				this->finalize2( callback );
		  	}
	);
}

void HadesSHMEM::finalize2(Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    delayReturn( callback );
} 

void HadesSHMEM::n_pes(int* val, Shmem::Callback callback )
{
	delayEnter(
			[=]() {
				this->n_pes2( val, callback );
		  	}
	);
}

void HadesSHMEM::n_pes2(int* val, Shmem::Callback callback )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    *val = m_num_pes;
	delayReturn( callback );
}

void HadesSHMEM::my_pe(int* val, Shmem::Callback callback )
{
	delayEnter(
			[=]() {
				this->my_pe2( val, callback );
		  	}
	);
}

void HadesSHMEM::my_pe2(int* val, Shmem::Callback callback )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    *val = m_my_pe;
	delayReturn( callback );
}

void HadesSHMEM::quiet(Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
	wait_until( m_pendingRemoteOps.getSimVAddr(), Shmem::EQ, m_zero, 
            [=](int) { 
                dbg().debug(CALL_INFO_LAMBDA,"quiet",1,SHMEM_BASE,"returning\n");
                callback(0); } 
            ); 
}

void HadesSHMEM::quiet2(Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
	wait_until( m_pendingRemoteOps.getSimVAddr(), Shmem::EQ, m_zero, 
		[=](int) {
			this->delayReturn(callback);
		}
	);
}

void HadesSHMEM::fence(Shmem::Callback callback)
{
	delayEnter(
			[=]() {
				this->fence2( callback );
		  	}
	);
}

void HadesSHMEM::fence2(Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
	delayReturn( callback );
}

void HadesSHMEM::malloc( Hermes::MemAddr* ptr, size_t size, bool backed, Shmem::Callback callback )
{
	delayEnter(
			[=]() {
				this->malloc2( ptr, size, backed, callback );
		  	}
	);
}

void HadesSHMEM::malloc2( Hermes::MemAddr* ptr, size_t size, bool backed, Shmem::Callback callback )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE," maddr ptr=%p size=%lu\n",ptr,size);
    malloc( ptr, size, backed, 
            [=]() { 
				this->delayReturn( callback );
            }
    );
}

void HadesSHMEM::malloc( Hermes::MemAddr* ptr, size_t size, bool backed, Callback callback )
{
    *ptr =  m_heap->malloc( size, backed );

    dbg().debug(CALL_INFO,1,SHMEM_BASE," maddr ptr=%p size=%lu\n",ptr,size);

	m_os->getNic()->shmemRegMem( *ptr, size, callback) ; 
}

void HadesSHMEM::free( Hermes::MemAddr* ptr, Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    assert(0);
}

void HadesSHMEM::barrier_all(Shmem::Callback callback)
{
	delayEnter(
			[=]() {
				this->barrier_all2( callback );
		  	}
	);
}

void HadesSHMEM::barrier_all2(Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");

    m_barrier->start( 0, 0, m_num_pes, m_pSync.getSimVAddr(), 
            [=](int) { 

    			dbg().debug(CALL_INFO,1,SHMEM_BASE,"barrier_all2() complete\n");
				this->delayReturn( callback );
            }
	);
}

void HadesSHMEM::barrier( int start, int stride, int size, Vaddr pSync, Shmem::Callback callback )
{
	delayEnter(
			[=]() {
				this->barrier2( start, stride, size, pSync, callback );
		  	}
	);
}

void HadesSHMEM::barrier2( int start, int stride, int size, Vaddr pSync, Shmem::Callback callback )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");

    m_barrier->start( start, stride, size, pSync,
            [=](int) { 
				this->delayReturn( callback );
            }
	);
} 

void HadesSHMEM::broadcast( Vaddr dest, Vaddr source, size_t nelems, int root, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
	delayEnter(
			[=]() {
				this->broadcast2( dest, source, nelems, root, start, stride, size, pSync, callback );
		  	}
	);
}

void HadesSHMEM::broadcast2( Vaddr dest, Vaddr source, size_t nelems, int root, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    m_broadcast->start( dest, source, nelems, root, start, stride, size, pSync, 
            [=](int) { 
				this->delayReturn( callback );
            },
	true );
}

void HadesSHMEM::fcollect( Vaddr dest, Vaddr source, size_t nelems, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
	delayEnter(
			[=]() {
				this->fcollect2( dest, source, nelems, start, stride, size, pSync, callback );
		  	}
	);
}

void HadesSHMEM::fcollect2( Vaddr dest, Vaddr source, size_t nelems, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    m_fcollect->start( dest, source, nelems, start, stride, size, pSync,
            [=](int) { 
				this->delayReturn( callback );
            }
	);
}

void HadesSHMEM::collect( Vaddr dest, Vaddr source, size_t nelems, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
	delayEnter(
			[=]() {
				this->collect2( dest, source, nelems, start, stride, size, pSync, callback );
		  	}
	);
}

void HadesSHMEM::collect2( Vaddr dest, Vaddr source, size_t nelems, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    m_collect->start( dest, source, nelems, start, stride, size, pSync, &m_localScratch, 
            [=](int) { 
				this->delayReturn( callback );
            }
	);
}

void HadesSHMEM::alltoall( Vaddr dest, Vaddr source, size_t nelems, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
	delayEnter(
			[=]() {
				this->alltoall2( dest, source, nelems, start, stride, size, pSync, callback );
		  	}
	);
}

void HadesSHMEM::alltoall2( Vaddr dest, Vaddr source, size_t nelems, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    m_alltoall->start( dest, source, nelems, start, stride, size, pSync,
            [=](int) { 
				this->delayReturn( callback );
            }
	);
}

void HadesSHMEM::alltoalls( Vaddr dest, Vaddr source, int dst, int sst, size_t nelems, int elsize, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
	delayEnter(
			[=]() {
				this->alltoalls2( dest, source, dst, sst, nelems, elsize, start, stride, size, pSync, callback );
		  	}
	);
}

void HadesSHMEM::alltoalls2( Vaddr dest, Vaddr source, int dst, int sst, size_t nelems, int elsize, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    m_alltoalls->start( dest, source, dst, sst, nelems, elsize, start, stride, size, pSync,
            [=](int) { 
				this->delayReturn( callback );
            }
	);
}

void HadesSHMEM::reduction( Vaddr dest, Vaddr source, int nelems, int PE_start,
                int logPE_stride, int PE_size, Vaddr pSync,
                Hermes::Shmem::ReduOp op, Hermes::Value::Type dataType, Shmem::Callback callback) 
{
	delayEnter(
			[=]() {
				this->reduction2( dest, source, nelems, PE_start, logPE_stride, PE_size, pSync, op, dataType, callback );
		  	}
	);
}

void HadesSHMEM::reduction2( Vaddr dest, Vaddr source, int nelems, int PE_start,
                int logPE_stride, int PE_size, Vaddr pSync,
                Hermes::Shmem::ReduOp op, Hermes::Value::Type dataType, Shmem::Callback callback) 
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    m_reduction->start( dest, source, nelems, PE_start, logPE_stride, PE_size, pSync,
            op, dataType, 
            [=](int) { 
				this->delayReturn( callback );
            }
	);	
}

void HadesSHMEM::get(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe, Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"destSimVAddr=%#" PRIx64 " srcSimVaddr=%#" PRIx64 " length=%lu\n",
                    dest, src, length);
	delayEnter(
			[=]() {
				this->get2( dest, src, length, pe, true, callback, m_blockingReturnLat_ns );
		  	}
	);
}

void HadesSHMEM::get_nbi(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe, Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"destSimVAddr=%#" PRIx64 " srcSimVaddr=%#" PRIx64 " length=%lu\n",
                    dest, src, length);
	delayEnter(
			[=]() {
				this->get2( dest, src, length, pe, false, callback, m_returnLat_ns );
		  	}
	);
}

void HadesSHMEM::get2(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe, bool blocking, Shmem::Callback callback, SimTime_t delay)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"destSimVAddr=%#" PRIx64 " srcSimVaddr=%#" PRIx64 " length=%lu\n",
                    dest, src, length);

    m_os->getNic()->shmemGet( calcNetPE(pe), dest, src, length, blocking, 
                [=]() {
                    this->dbg().debug(CALL_INFO_LAMBDA,"get",1,SHMEM_BASE,"returning\n");
                    this->delayReturn( callback, delay );
                }
            );
}

void HadesSHMEM::getv( Hermes::Value& value, Hermes::Vaddr src, int pe, Shmem::Callback callback)
{
	delayEnter(
			[=]() {
                Hermes::Value _value = value;
				this->getv2( _value, src, pe, callback );
		  	}
	);
}

void HadesSHMEM::getv2( Hermes::Value& value, Hermes::Vaddr src, int pe, Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"srcSimVaddr=%#" PRIx64 "\n",src );

    Hermes::Value::Type type = value.getType(); 

    m_os->getNic()->shmemGetv( calcNetPE(pe), src, type, 

                [=]( Hermes::Value& newValue ) {
                    this->dbg().debug(CALL_INFO_LAMBDA,"getv2",1,SHMEM_BASE,"returning\n");

                    Hermes::Value _value = value;
                    ::memcpy( _value.getPtr(), newValue.getPtr(), _value.getLength() );

                    this->delayReturn( callback, m_blockingReturnLat_ns );
                }
            );
}

void HadesSHMEM::put(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe, Shmem::Callback callback)
{
	if ( length == 0 ) {
		
		delay( callback, 0, 0 );
		return; 
	}
	delayEnter( 
			[=]() {
				this->put2( dest, src, length, pe, true, callback, m_returnLat_ns );
		  	}
		 );
}

void HadesSHMEM::put_nbi(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe, Shmem::Callback callback)
{
	if ( length == 0 ) {
		
		delay( callback, 0, 0 );
		return; 
	}
	delayEnter( 
			[=]() {
				this->put2( dest, src, length, pe, false, callback, m_blockingReturnLat_ns );
		  	}
		 );
}

void HadesSHMEM::put2(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe, bool blocking, Shmem::Callback callback, SimTime_t delay )
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    m_os->getNic()->shmemPut( calcNetPE(pe), dest, src, length, blocking, 
                [=]() {
                    this->dbg().debug(CALL_INFO_LAMBDA,"put2",1,SHMEM_BASE,"returning\n");
					this->delayReturn( callback, delay );
                }
            );
}

void HadesSHMEM::putOp(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe,
                Hermes::Shmem::ReduOp op, Hermes::Value::Type dataType, Shmem::Callback callback)
{
	delayEnter(
			[=]() {
				this->putOp2( dest, src, length, pe, op, dataType, callback );
		  	}
	);
}

void HadesSHMEM::putOp2(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe,
                Hermes::Shmem::ReduOp op, Hermes::Value::Type dataType, Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"\n");
    m_os->getNic()->shmemPutOp( calcNetPE(pe), dest, src, length, op, dataType, 
                [=]() {
                    this->dbg().debug(CALL_INFO_LAMBDA,"putOp2",1,SHMEM_BASE,"returning\n");
                    this->delayReturn( callback );
                }
            );
}

void HadesSHMEM::putv(Hermes::Vaddr dest, Hermes::Value& value, int pe, Shmem::Callback callback)
{
	delayEnter(
			[=]() {
                Hermes::Value _value = value;
				this->putv2( dest, _value, pe, callback );
		  	}
	);
}

void HadesSHMEM::putv2(Hermes::Vaddr dest, Hermes::Value& value, int pe, Shmem::Callback callback)
{
    std::stringstream tmp;
    tmp << value;
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"destSimVaddr=%#" PRIx64 " value=%s\n", dest, tmp.str().c_str() );

    m_os->getNic()->shmemPutv( calcNetPE(pe), dest, value,
                [=]() {
                    this->dbg().debug(CALL_INFO_LAMBDA,"putv2",1,SHMEM_BASE,"returning\n");
                    this->delayReturn( callback );
                }
            );
}

void HadesSHMEM::wait_until(Hermes::Vaddr addr, Hermes::Shmem::WaitOp op, Hermes::Value& value, Shmem::Callback callback)
{
	delayEnter(
			[=]() {
                Hermes::Value _value = value;
				this->wait_until2( addr, op, _value, callback );
		  	}
	);
}
void HadesSHMEM::wait_until2(Hermes::Vaddr addr, Hermes::Shmem::WaitOp op, Hermes::Value& value, Shmem::Callback callback)
{
    std::stringstream tmp;
    tmp << value;
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"addr=%#" PRIx64 " val=%s\n",addr, tmp.str().c_str());

    m_os->getNic()->shmemWait( addr, op, value,
                [=]() {
                    this->dbg().debug(CALL_INFO_LAMBDA,"wait_until2",1,SHMEM_BASE,"addr=%#" PRIx64 " returning\n",addr);
                    this->delayReturn( callback );
                }
            );
}

void HadesSHMEM::swap(Hermes::Value& result, Hermes::Vaddr addr, Hermes::Value& value, int pe, Shmem::Callback callback)
{
	delayEnter(
			[=]() {
                Hermes::Value _value = value;
                Hermes::Value _result = result;
				this->swap2( _result, addr, _value, pe, callback );
		  	}
	);
}

void HadesSHMEM::swap2(Hermes::Value& result, Hermes::Vaddr addr, Hermes::Value& value, int pe, Shmem::Callback callback)
{
    std::stringstream tmp1;
    tmp1 << value;
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"addr=%#" PRIx64 " val=%s\n",addr, tmp1.str().c_str());

    m_os->getNic()->shmemSwap( calcNetPE(pe), addr,  value, 

                [=]( Hermes::Value& newValue ) {
                    this->dbg().debug(CALL_INFO_LAMBDA,"swap2",1,SHMEM_BASE,"returning\n");

                    Hermes::Value _result = result;
                    ::memcpy( _result.getPtr(), newValue.getPtr(), value.getLength() );

                    this->delayReturn( callback, m_blockingReturnLat_ns );
                }
            );
}

void HadesSHMEM::cswap(Hermes::Value& result, Hermes::Vaddr addr, Hermes::Value& cond, Hermes::Value& value, int pe, Shmem::Callback callback)
{
	delayEnter(
			[=]() {
                Hermes::Value _result = result;
                Hermes::Value _cond = cond;
                Hermes::Value _value = value;
				this->cswap2( _result, addr, _cond, _value, pe, callback );
		  	}
	);
}

void HadesSHMEM::cswap2(Hermes::Value& result, Hermes::Vaddr addr, Hermes::Value& cond, Hermes::Value& value, int pe, Shmem::Callback callback)
{
    std::stringstream tmp1;
    tmp1 << value;
    std::stringstream tmp2;
    tmp2 << cond;
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"addr=%#" PRIx64 " val=%s cond=%s\n",addr, tmp1.str().c_str(),tmp2.str().c_str());

    m_os->getNic()->shmemCswap( calcNetPE(pe), addr, cond, value, 

                [=]( Hermes::Value& newValue ) {
                    this->dbg().debug(CALL_INFO_LAMBDA,"cswap2",1,SHMEM_BASE,"returning\n");

                    Hermes::Value _result = result;
                    ::memcpy( _result.getPtr(), newValue.getPtr(), value.getLength() );

                    this->delayReturn( callback, m_blockingReturnLat_ns );
                }
            );
}

void HadesSHMEM::add( Hermes::Vaddr addr, Hermes::Value& value, int pe, Shmem::Callback callback)
{
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"addr=%#" PRIx64 "\n",addr);
	delayEnter(
			[=]() {
                Hermes::Value _value = value;
				this->add2( addr, _value, pe, callback );
		  	}
	);
}

void HadesSHMEM::add2( Hermes::Vaddr addr, Hermes::Value& value, int pe, Shmem::Callback callback)
{
    std::stringstream tmp;
    tmp << value;
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"addr=%#" PRIx64 " val=%s\n",addr, tmp.str().c_str());

    m_os->getNic()->shmemAdd( calcNetPE(pe), addr, value, 
                [=]( ) {
                    this->dbg().debug(CALL_INFO_LAMBDA,"add2",1,SHMEM_BASE,"addr=%#" PRIx64 " returning\n", addr );

                    this->delayReturn( callback );
                }
            );
}

void HadesSHMEM::fadd(Hermes::Value& result, Hermes::Vaddr addr, Hermes::Value& value, int pe, Shmem::Callback callback)
{
	delayEnter(
			[=]() {
                Hermes::Value _value = value;
                Hermes::Value _result = result;
				this->fadd2( _result, addr, _value, pe, callback );
		  	}
	);
}

void HadesSHMEM::fadd2(Hermes::Value& result, Hermes::Vaddr addr, Hermes::Value& value, int pe, Shmem::Callback callback)
{
    std::stringstream tmp;
    tmp << value;
    dbg().debug(CALL_INFO,1,SHMEM_BASE,"addr=%#" PRIx64 " val=%s\n",addr, tmp.str().c_str());

    m_os->getNic()->shmemFadd( calcNetPE(pe), addr, value, 
                [=]( Hermes::Value& newValue ) {
                    this->dbg().debug(CALL_INFO_LAMBDA,"fadd2",1,SHMEM_BASE,"returning\n");

                    Hermes::Value _result = result;
                    ::memcpy( _result.getPtr(), newValue.getPtr(), _result.getLength() );

                    this->delayReturn( callback, m_blockingReturnLat_ns );
                }
            );
}
