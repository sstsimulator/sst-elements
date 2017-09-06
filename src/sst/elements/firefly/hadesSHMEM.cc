// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
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

#include "hadesSHMEM.h"
#include "functionSM.h"
#include "funcSM/event.h"
#include "shmem/barrier.h"

using namespace SST::Firefly;
using namespace Hermes;

HadesSHMEM::HadesSHMEM(Component* owner, Params& params) :
    Interface(owner), m_common( NULL )
{
    m_dbg.init("@t:HadesSHMEM::@p():@l ",
        params.find<uint32_t>("verboseLevel",0),
        params.find<uint32_t>("verboseMask",0),
        Output::STDOUT );

    m_selfLink = configureSelfLink("ShmemToDriver", "1 ps",
            new Event::Handler<HadesSHMEM>(this,&HadesSHMEM::handleToDriver));
    m_heap = new Heap(true);
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
                    m_os->getNic()->getNodeId(), m_os->getInfo()->worldRank());
    m_dbg.setPrefix(buffer);
}

void HadesSHMEM::init(Shmem::Callback callback)
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    m_num_pes = m_os->getInfo()->getGroup(MP::GroupWorld)->getSize();
    m_my_pe = m_os->getInfo()->getGroup(MP::GroupWorld)->getMyRank();

    m_common = new ShmemCommon( m_my_pe, m_num_pes, 10, 2 );
    m_barrier = new ShmemBarrier( *this, *m_common );
    m_broadcast = new ShmemBroadcast( *this, *m_common );
    m_collect = new ShmemCollect( *this, *m_common );
    m_fcollect = new ShmemFcollect( *this, *m_common );
    m_alltoall = new ShmemAlltoall( *this, *m_common );
    m_alltoalls = new ShmemAlltoalls( *this, *m_common );
    m_reduction = new ShmemReduction( *this, *m_common );
    m_localDataSize = sizeof(ShmemCollective::pSync_t) + sizeof(long) * 2;

    malloc( &m_localData, m_localDataSize, 
            [=]() { 
                m_pSync = m_localData;
                m_pSync.at<ShmemCollective::pSync_t>(0) = 0;
                m_localScratch = m_pSync.offset<ShmemCollective::pSync_t>(1);
                m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
            }
    );
}

void HadesSHMEM::finalize(Shmem::Callback callback)
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
} 

void HadesSHMEM::n_pes(int* val, Shmem::Callback callback )
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    *val = m_num_pes;
    m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
}

void HadesSHMEM::my_pe(int* val, Shmem::Callback callback )
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    *val = m_my_pe;
    m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
}

void HadesSHMEM::barrier_all(Shmem::Callback callback)
{
    dbg().verbose(CALL_INFO,1,1,"\n");

    m_barrier->start( 0, 0, m_num_pes, m_pSync.getSimVAddr(), callback );
}

void HadesSHMEM::barrier( int start, int stride, int size, Vaddr pSync, Shmem::Callback callback )
{
    dbg().verbose(CALL_INFO,1,1,"\n");

    m_barrier->start( start, stride, size, pSync, callback );
} 

void HadesSHMEM::broadcast( Vaddr dest, Vaddr source, size_t nelems, int root, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    m_broadcast->start( dest, source, nelems, root, start, stride, size, pSync, callback, true );
}

void HadesSHMEM::fcollect( Vaddr dest, Vaddr source, size_t nelems, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    m_fcollect->start( dest, source, nelems, start, stride, size, pSync, callback );
}

void HadesSHMEM::collect( Vaddr dest, Vaddr source, size_t nelems, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    m_collect->start( dest, source, nelems, start, stride, size, pSync, &m_localScratch, callback );
}

void HadesSHMEM::alltoall( Vaddr dest, Vaddr source, size_t nelems, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    m_alltoall->start( dest, source, nelems, start, stride, size, pSync, callback );
}

void HadesSHMEM::alltoalls( Vaddr dest, Vaddr source, int dst, int sst, size_t nelems, int elsize, int start,
                                int stride, int size, Vaddr pSync, Shmem::Callback callback)
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    m_alltoalls->start( dest, source, dst, sst, nelems, elsize, start, stride, size, pSync, callback );
}

void HadesSHMEM::reduction( Vaddr dest, Vaddr source, int nelems, int PE_start,
                int logPE_stride, int PE_size, Vaddr pSync,
                Hermes::Shmem::ReduOp op, Hermes::Value::Type dataType, Shmem::Callback callback) 
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    m_reduction->start( dest, source, nelems, PE_start, logPE_stride, PE_size, pSync,
            op, dataType, callback );
}

void HadesSHMEM::quiet(Shmem::Callback callback)
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
}

void HadesSHMEM::fence(Shmem::Callback callback)
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
#if 0
    FunctionSM::Callback callback = [this, callback ] { 

        this->dbg().verbose(CALL_INFO,1,1,"\n");

        this->m_os->getNic()->shmemBlocked( 

            [=]() { 
                this->dbg().verbose(CALL_INFO,1,1,"\n");
                this->m_os->getNic()->shmemFence( 
                    [=]() {
                        this->dbg().verbose(CALL_INFO,1,1,"\n");
                        this->m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
                    }
                );
            }
        );
    };

    functionSM().start( FunctionSM::Barrier, callback,
            new BarrierStartEvent( Hermes::MP::GroupWorld) );
#endif
}

void HadesSHMEM::malloc( Hermes::MemAddr* ptr, size_t size, Shmem::Callback callback )
{
    dbg().verbose(CALL_INFO,1,1," maddr ptr=%p size=%lu\n",ptr,size);
    malloc( ptr, size, 
            [=]() { 
                m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
            }
    );
}

void HadesSHMEM::malloc( Hermes::MemAddr* ptr, size_t size, Callback callback )
{
    dbg().verbose(CALL_INFO,1,1," maddr ptr=%p size=%lu\n",ptr,size);

    *ptr =  m_heap->malloc( size );

    m_os->getNic()->shmemBlocked( 
        [=]() { 
            this->dbg().verbose(CALL_INFO,1,1,"\n");

            this->m_os->getNic()->shmemRegMem( *ptr, size, callback) ; 
        }
    );  
}

void HadesSHMEM::free( Hermes::MemAddr* ptr, Shmem::Callback callback)
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    assert(0);
}

void HadesSHMEM::get(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe, Shmem::Callback callback)
{
    dbg().verbose(CALL_INFO,1,1,"destSimVAddr=%#" PRIx64 " srcSimVaddr=%#" PRIx64 " length=%lu\n",
                    dest, src, length);

    m_os->getNic()->shmemBlocked(
        [=]() { 
            this->dbg().verbose(CALL_INFO,1,1,"\n");
            this->m_os->getNic()->shmemGet( pe, dest, src, length, 

                [=]() {
                    this->dbg().verbose(CALL_INFO,1,1,"\n");
                    this->m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
                }
            );
        }
    ); 

}

void HadesSHMEM::getv( Hermes::Value& value, Hermes::Vaddr src, int pe, Shmem::Callback callback)
{
    dbg().verbose(CALL_INFO,1,1,"srcSimVaddr=%#" PRIx64 "\n",src );

    Hermes::Value::Type type = value.getType(); 

    m_os->getNic()->shmemBlocked(
        [=]() { 
            this->dbg().verbose(CALL_INFO,1,1,"\n");

            this->m_os->getNic()->shmemGetv( pe, src, type, 

                [=]( Hermes::Value& newValue ) {
                    this->dbg().verbose(CALL_INFO,1,1,"\n");

                    Hermes::Value _value = value;
                    ::memcpy( _value.getPtr(), newValue.getPtr(), _value.getLength() );

                    this->m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
                }
            );
        }
    );
}

void HadesSHMEM::put(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe, Shmem::Callback callback)
{
    m_os->getNic()->shmemBlocked(
        [=]() { 
            this->dbg().verbose(CALL_INFO,1,1,"\n");
            this->m_os->getNic()->shmemPut( pe, dest, src, length, 

                [=]() {
                    this->dbg().verbose(CALL_INFO,1,1,"\n");
                    this->m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
                }
            );
        } 
    );
}

void HadesSHMEM::putOp(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe,
                Hermes::Shmem::ReduOp op, Hermes::Value::Type dataType, Shmem::Callback callback)
{
    m_os->getNic()->shmemBlocked(
        [=]() { 
            this->dbg().verbose(CALL_INFO,1,1,"\n");
            this->m_os->getNic()->shmemPutOp( pe, dest, src, length, op, dataType, 

                [=]() {
                    this->dbg().verbose(CALL_INFO,1,1,"\n");
                    this->m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
                }
            );
        } 
    );
}

void HadesSHMEM::putv(Hermes::Vaddr dest, Hermes::Value& value, int pe, Shmem::Callback callback)
{
    std::stringstream tmp;
    tmp << value;
    dbg().verbose(CALL_INFO,1,1,"destSimVaddr=%#" PRIx64 " value=%s\n", dest, tmp.str().c_str() );

    m_os->getNic()->shmemBlocked(
        [=]() { 
            this->dbg().verbose(CALL_INFO,1,1,"\n");
            Hermes::Value _value = value;

            this->m_os->getNic()->shmemPutv( pe, dest, _value,
                [=]() {
                    this->dbg().verbose(CALL_INFO,1,1,"\n");
                    this->m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
                }
            );

        }
    );
}

void HadesSHMEM::wait_until(Hermes::Vaddr addr, Hermes::Shmem::WaitOp op, Hermes::Value& value, Shmem::Callback callback)
{
    std::stringstream tmp;
    tmp << value;
    dbg().verbose(CALL_INFO,1,1,"addr=%#" PRIx64 " val=%s\n",addr, tmp.str().c_str());

    m_os->getNic()->shmemBlocked(
        [=]() { 
            this->dbg().verbose(CALL_INFO,1,1,"\n");
            Hermes::Value _value = value;
            this->m_os->getNic()->shmemWait( addr, op, _value,
                [=]() {
                    this->dbg().verbose(CALL_INFO,1,1,"\n");
                    this->m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
                }
            );
        }
    );
}


void HadesSHMEM::swap(Hermes::Value& result, Hermes::Vaddr addr, Hermes::Value& value, int pe, Shmem::Callback callback)
{
    std::stringstream tmp1;
    tmp1 << value;
    dbg().verbose(CALL_INFO,1,1,"addr=%#" PRIx64 " val=%s\n",addr, tmp1.str().c_str());

    m_os->getNic()->shmemBlocked(
        [=]() { 
            this->dbg().verbose(CALL_INFO,1,1,"\n");
            Hermes::Value _value = value;
            this->m_os->getNic()->shmemSwap( pe, addr,  _value, 

                [=]( Hermes::Value& newValue ) {
                    this->dbg().verbose(CALL_INFO,1,1,"\n");

                    Hermes::Value _result = result;
                    ::memcpy( _result.getPtr(), newValue.getPtr(), _value.getLength() );

                    this->m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
                }
            );
        }
    );
}

void HadesSHMEM::cswap(Hermes::Value& result, Hermes::Vaddr addr, Hermes::Value& cond, Hermes::Value& value, int pe, Shmem::Callback callback)
{
    std::stringstream tmp1;
    tmp1 << value;
    std::stringstream tmp2;
    tmp2 << cond;
    dbg().verbose(CALL_INFO,1,1,"addr=%#" PRIx64 " val=%s cond=%s\n",addr, tmp1.str().c_str(),tmp2.str().c_str());

    m_os->getNic()->shmemBlocked(
        [=]() { 
            this->dbg().verbose(CALL_INFO,1,1,"\n");
            Hermes::Value _value = value;
            Hermes::Value _cond = cond;
            this->m_os->getNic()->shmemCswap( pe, addr, _cond, _value, 

                [=]( Hermes::Value& newValue ) {
                    this->dbg().verbose(CALL_INFO,1,1,"\n");

                    Hermes::Value _result = result;
                    ::memcpy( _result.getPtr(), newValue.getPtr(), _value.getLength() );

                    this->m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
                }
            );
        }
    );
}

void HadesSHMEM::add( Hermes::Vaddr addr, Hermes::Value& value, int pe, Shmem::Callback callback)
{
    std::stringstream tmp;
    tmp << value;
    dbg().verbose(CALL_INFO,1,1,"addr=%#" PRIx64 " val=%s\n",addr, tmp.str().c_str());

    m_os->getNic()->shmemBlocked(
        [=]() { 
            this->dbg().verbose(CALL_INFO,1,1,"\n");
            Hermes::Value _value = value;
            this->m_os->getNic()->shmemAdd( pe, addr, _value, 

                [=]( ) {
                    this->dbg().verbose(CALL_INFO,1,1,"\n");

                    this->m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
                }
            );
        }
    );
}
void HadesSHMEM::fadd(Hermes::Value& result, Hermes::Vaddr addr, Hermes::Value& value, int pe, Shmem::Callback callback)
{
    std::stringstream tmp;
    tmp << value;
    dbg().verbose(CALL_INFO,1,1,"addr=%#" PRIx64 " val=%s\n",addr, tmp.str().c_str());

    m_os->getNic()->shmemBlocked(
        [=]() { 
            this->dbg().verbose(CALL_INFO,1,1,"\n");
            Hermes::Value _value = value;
            this->m_os->getNic()->shmemFadd( pe, addr, _value, 

                [=]( Hermes::Value& newValue ) {
                    this->dbg().verbose(CALL_INFO,1,1,"\n");

                    Hermes::Value _result = result;
                    ::memcpy( _result.getPtr(), newValue.getPtr(), _result.getLength() );

                    this->m_selfLink->send( 0, new DelayEvent( callback, 0 ) );
                }
            );
        }
    );
}
