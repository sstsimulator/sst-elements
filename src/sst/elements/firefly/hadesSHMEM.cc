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

using namespace SST::Firefly;
using namespace Hermes;

HadesSHMEM::HadesSHMEM(Component* owner, Params& params) :
    Interface(owner)
{
    m_dbg.init("@t:HadesSHMEM::@p():@l ",
        params.find<uint32_t>("verboseLevel",0),
        params.find<uint32_t>("verboseMask",0),
        Output::STDOUT );

    m_selfLink = configureSelfLink("ShmemToDriver", "1 ps",
            new Event::Handler<HadesSHMEM>(this,&HadesSHMEM::handleToDriver));
    m_heap = new Heap(true);
}


void HadesSHMEM::setup()
{
    char buffer[100];
    snprintf(buffer,100,"@t:%d:%d:HadesSHMEM::@p():@l ",
                    m_os->getNic()->getNodeId(), m_os->getInfo()->worldRank());
    m_dbg.setPrefix(buffer);
}

void HadesSHMEM::init(MP::Functor* functor)
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    m_selfLink->send( 0, new DelayEvent( functor, 0 ) );
}

void HadesSHMEM::finalize(MP::Functor* functor)
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    m_selfLink->send( 0, new DelayEvent( functor, 0 ) );
} 

void HadesSHMEM::n_pes(int* val, MP::Functor* functor )
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    functionSM().start(FunctionSM::Size, functor,
            new SizeStartEvent( Hermes::MP::GroupWorld , (int*) val) );
}

void HadesSHMEM::my_pe(int* val,MP::Functor* functor )
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    functionSM().start(FunctionSM::Rank, functor,
            new RankStartEvent( Hermes::MP::GroupWorld , (int*) val) );
}

void HadesSHMEM::barrier_all(MP::Functor* functor)
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    functionSM().start( FunctionSM::Barrier, functor,
            new BarrierStartEvent( Hermes::MP::GroupWorld) );
}

void HadesSHMEM::fence(MP::Functor* functor)
{
    FunctionSM::Callback callback = [this, functor ] { 

        this->dbg().verbose(CALL_INFO,1,1,"\n");

        this->m_os->getNic()->shmemBlocked( 

            [=]() { 
                this->dbg().verbose(CALL_INFO,1,1,"\n");
                this->m_os->getNic()->shmemFence( 
                    [=]() {
                        this->dbg().verbose(CALL_INFO,1,1,"\n");
                        this->m_selfLink->send( 0, new DelayEvent( functor, 0 ) );
                    }
                );
            }
        );
    };

    functionSM().start( FunctionSM::Barrier, callback,
            new BarrierStartEvent( Hermes::MP::GroupWorld) );
}

void HadesSHMEM::malloc( Hermes::MemAddr* ptr, size_t size, MP::Functor* functor )
{
    dbg().verbose(CALL_INFO,1,1," maddr ptr=%p size=%lu\n",ptr,size);

    *ptr =  m_heap->malloc( size );

    VirtNic::Callback callback = [=]() { 
        this->dbg().verbose(CALL_INFO,1,1,"\n");

        this->m_os->getNic()->shmemRegMem( *ptr, size, 
            [=]() { 
                this->dbg().verbose(CALL_INFO,1,1,"shmemRegMem back from nic simVAddr=%#" PRIx64 " backing=%p\n",
                    ptr->getSimVAddr(), ptr->getBacking());
                this->m_selfLink->send( 0, new DelayEvent( functor, 0 ) );
            }
        );         
    };

    m_os->getNic()->shmemBlocked( callback );
}

void HadesSHMEM::free( Hermes::MemAddr* ptr, MP::Functor* functor)
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    assert(0);
}

void HadesSHMEM::get(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe, MP::Functor* functor)
{
    dbg().verbose(CALL_INFO,1,1,"destSimVAddr=%#" PRIx64 " srcSimVaddr=%#" PRIx64 " length=%lu\n",
                    dest, src, length);

    VirtNic::Callback callback = [=]() { 
        this->dbg().verbose(CALL_INFO,1,1,"\n");
        this->m_os->getNic()->shmemGet( pe, dest, src, length, 

            [=]() {
                this->dbg().verbose(CALL_INFO,1,1,"\n");
                this->m_selfLink->send( 0, new DelayEvent( functor, 0 ) );
            }
        );
    };

    m_os->getNic()->shmemBlocked( callback );
}

void HadesSHMEM::getv( Hermes::Value& value, Hermes::Vaddr src, int pe, MP::Functor* functor)
{
    dbg().verbose(CALL_INFO,1,1,"srcSimVaddr=%#" PRIx64 "\n",src );

    Hermes::Value::Type type = value.getType(); 

    VirtNic::Callback callback = [=]() { 
        this->dbg().verbose(CALL_INFO,1,1,"\n");

        this->m_os->getNic()->shmemGetv( pe, src, type, 

            [=]( Hermes::Value& newValue ) {
                this->dbg().verbose(CALL_INFO,1,1,"\n");

                Hermes::Value _value = value;
                memcpy( _value.getPtr(), newValue.getPtr(), _value.getLength() );

                this->m_selfLink->send( 0, new DelayEvent( functor, 0 ) );
            }
        );
    };

    m_os->getNic()->shmemBlocked( callback );
}

void HadesSHMEM::put(Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe, MP::Functor* functor)
{
    VirtNic::Callback callback = [=]() { 
        this->dbg().verbose(CALL_INFO,1,1,"\n");
        this->m_os->getNic()->shmemPut( pe, dest, src, length, 

            [=]() {
                this->dbg().verbose(CALL_INFO,1,1,"\n");
                this->m_selfLink->send( 0, new DelayEvent( functor, 0 ) );
            }
        );
    };

    m_os->getNic()->shmemBlocked( callback );
}

void HadesSHMEM::putv(Hermes::Vaddr dest, Hermes::Value& value, int pe, MP::Functor* functor)
{
    std::stringstream tmp;
    tmp << value;
    dbg().verbose(CALL_INFO,1,1,"destSimVaddr=%#" PRIx64 " value=%s\n", dest, tmp.str().c_str() );

    VirtNic::Callback callback = [=]() { 
        this->dbg().verbose(CALL_INFO,1,1,"\n");
        Hermes::Value _value = value;

        this->m_os->getNic()->shmemPutv( pe, dest, _value,
            [=]() {
                this->dbg().verbose(CALL_INFO,1,1,"\n");
                this->m_selfLink->send( 0, new DelayEvent( functor, 0 ) );
            }
        );

    };

    m_os->getNic()->shmemBlocked( callback );
}

void HadesSHMEM::wait_until(Hermes::Vaddr addr, Hermes::Shmem::WaitOp op, Hermes::Value& value, MP::Functor* functor)
{
    std::stringstream tmp;
    tmp << value;
    dbg().verbose(CALL_INFO,1,1,"addr=%#" PRIx64 " val=%s\n",addr, tmp.str().c_str());

    VirtNic::Callback callback = [=]() { 
        this->dbg().verbose(CALL_INFO,1,1,"\n");
        Hermes::Value _value = value;
        this->m_os->getNic()->shmemWait( addr, op, _value,
            [=]() {
                this->dbg().verbose(CALL_INFO,1,1,"\n");
                this->m_selfLink->send( 0, new DelayEvent( functor, 0 ) );
            }
        );
    };

    m_os->getNic()->shmemBlocked( callback );
}


void HadesSHMEM::swap(Hermes::Value& result, Hermes::Vaddr addr, Hermes::Value& value, int pe, MP::Functor* functor)
{
    std::stringstream tmp1;
    tmp1 << value;
    dbg().verbose(CALL_INFO,1,1,"addr=%#" PRIx64 " val=%s\n",addr, tmp1.str().c_str());

    VirtNic::Callback callback = [=]() { 
        this->dbg().verbose(CALL_INFO,1,1,"\n");
        Hermes::Value _value = value;
        this->m_os->getNic()->shmemSwap( pe, addr,  _value, 

            [=]( Hermes::Value& newValue ) {
                this->dbg().verbose(CALL_INFO,1,1,"\n");

                Hermes::Value _result = result;
                memcpy( _result.getPtr(), newValue.getPtr(), _value.getLength() );

                this->m_selfLink->send( 0, new DelayEvent( functor, 0 ) );
            }
        );
    };

    m_os->getNic()->shmemBlocked( callback );
}
void HadesSHMEM::cswap(Hermes::Value& result, Hermes::Vaddr addr, Hermes::Value& cond, Hermes::Value& value, int pe, MP::Functor* functor)
{
    std::stringstream tmp1;
    tmp1 << value;
    std::stringstream tmp2;
    tmp2 << cond;
    dbg().verbose(CALL_INFO,1,1,"addr=%#" PRIx64 " val=%s cond=%s\n",addr, tmp1.str().c_str(),tmp2.str().c_str());

    VirtNic::Callback callback = [=]() { 
        this->dbg().verbose(CALL_INFO,1,1,"\n");
        Hermes::Value _value = value;
        Hermes::Value _cond = cond;
        this->m_os->getNic()->shmemCswap( pe, addr, _cond, _value, 

            [=]( Hermes::Value& newValue ) {
                this->dbg().verbose(CALL_INFO,1,1,"\n");

                Hermes::Value _result = result;
                memcpy( _result.getPtr(), newValue.getPtr(), _value.getLength() );

                this->m_selfLink->send( 0, new DelayEvent( functor, 0 ) );
            }
        );
    };

    m_os->getNic()->shmemBlocked( callback );
}

void HadesSHMEM::fadd(Hermes::Value& result, Hermes::Vaddr addr, Hermes::Value& value, int pe, MP::Functor* functor)
{
    std::stringstream tmp;
    tmp << value;
    dbg().verbose(CALL_INFO,1,1,"addr=%#" PRIx64 " val=%s\n",addr, tmp.str().c_str());

    VirtNic::Callback callback = [=]() { 
        this->dbg().verbose(CALL_INFO,1,1,"\n");
        Hermes::Value _value = value;
        this->m_os->getNic()->shmemFadd( pe, addr, _value, 

            [=]( Hermes::Value& newValue ) {
                this->dbg().verbose(CALL_INFO,1,1,"\n");

                Hermes::Value _result = result;
                memcpy( _result.getPtr(), newValue.getPtr(), _result.getLength() );

                this->m_selfLink->send( 0, new DelayEvent( functor, 0 ) );
            }
        );
    };

    m_os->getNic()->shmemBlocked( callback );
}
