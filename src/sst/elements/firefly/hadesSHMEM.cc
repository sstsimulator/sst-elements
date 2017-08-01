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

            [=](uint64_t) { 
                this->dbg().verbose(CALL_INFO,1,1,"\n");
                this->m_os->getNic()->shmemFence( 
                    [=](uint64_t) {
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
    dbg().verbose(CALL_INFO,1,1," maddr ptr=%p\n",ptr);

    *ptr =  m_heap->malloc( size );

    VirtNic::Callback callback = [=](uint64_t) { 
        this->dbg().verbose(CALL_INFO,1,1,"\n");

        this->m_os->getNic()->shmemRegMem( *ptr, size, 
            [=](uint64_t) { 
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

void HadesSHMEM::get(Hermes::MemAddr dest, Hermes::MemAddr src, size_t length, int pe, MP::Functor* functor)
{
    dbg().verbose(CALL_INFO,1,1,"destSimVAddr=%#" PRIx64 " srcSimVaddr=%#" PRIx64 " length=%lu\n",
                    dest.getSimVAddr(), src.getSimVAddr(), length);

    VirtNic::Callback callback = [=](uint64_t) { 
        this->dbg().verbose(CALL_INFO,1,1,"\n");
        Hermes::MemAddr _dest = dest;
        Hermes::MemAddr _src = src;
        this->m_os->getNic()->shmemGet( pe, _dest, _src, length, 

            [=](uint64_t) {
                this->dbg().verbose(CALL_INFO,1,1,"\n");
                this->m_selfLink->send( 0, new DelayEvent( functor, 0 ) );
            }
        );
    };

    m_os->getNic()->shmemBlocked( callback );
}

void HadesSHMEM::getv( void* dest, Hermes::MemAddr src, int length, int pe, MP::Functor* functor)
{
    dbg().verbose(CALL_INFO,1,1,"srcSimVaddr=%#" PRIx64 " length=%d\n",src.getSimVAddr(), length);

    VirtNic::Callback callback = [=]( uint64_t) { 
        this->dbg().verbose(CALL_INFO,1,1,"\n");
        Hermes::MemAddr _src = src;
        this->m_os->getNic()->shmemGet( pe, _src, length, 

            [=](uint64_t value ) {
                this->dbg().verbose(CALL_INFO,1,1,"value=%#" PRIx64 "\n",value);
                memcpy( dest, &value, length );
                this->m_selfLink->send( 0, new DelayEvent( functor, 0 ) );
            }
        );
    };

    m_os->getNic()->shmemBlocked( callback );
}

void HadesSHMEM::put(Hermes::MemAddr dest, Hermes::MemAddr src, size_t length, int pe, MP::Functor* functor)
{
    VirtNic::Callback callback = [=](uint64_t) { 
        this->dbg().verbose(CALL_INFO,1,1,"\n");
        Hermes::MemAddr _dest = dest;
        Hermes::MemAddr _src = src;
        this->m_os->getNic()->shmemPut( pe, _dest, _src, length, 

            [=](uint64_t) {
                this->dbg().verbose(CALL_INFO,1,1,"\n");
                this->m_selfLink->send( 0, new DelayEvent( functor, 0 ) );
            }
        );
    };

    m_os->getNic()->shmemBlocked( callback );
}

void HadesSHMEM::putv(Hermes::MemAddr dest, uint64_t value, int size, int pe, MP::Functor* functor)
{
    dbg().verbose(CALL_INFO,1,1,"val=%" PRIu64 ", size=%d\n",value,size);

    VirtNic::Callback callback = [=](uint64_t) { 
        this->dbg().verbose(CALL_INFO,1,1,"\n");
        Hermes::MemAddr tmp = dest;
        this->m_os->getNic()->shmemPut( pe, tmp, value, size, 

            [=](uint64_t) {
                this->dbg().verbose(CALL_INFO,1,1,"\n");
                this->m_selfLink->send( 0, new DelayEvent( functor, 0 ) );
            }
        );
    };

    m_os->getNic()->shmemBlocked( callback );
}

