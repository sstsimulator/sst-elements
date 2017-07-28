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


#ifndef COMPONENTS_FIREFLY_HADESSHMEM_H
#define COMPONENTS_FIREFLY_HADESSHMEM_H

#include <sst/core/params.h>

#include <stdlib.h>
#include <string.h>
#include "sst/elements/hermes/shmemapi.h"
#include "hades.h"


using namespace Hermes;

namespace SST {
namespace Firefly {

class HadesSHMEM : public Shmem::Interface
{
    class DelayEvent : public SST::Event {
      public:

        DelayEvent( MP::Functor* functor, int retval ) :
            Event(),
            m_functor( functor ),
            m_retval( retval )
        {}

        MP::Functor* m_functor;
        int m_retval;

        NotSerializable(DelayEvent)
    };

    class Heap {
      public:
        Heap( bool back = false ) : m_curAddr(0x1000), m_back(back) {}
        Hermes::MemAddr malloc( size_t n ) {
            Hermes::MemAddr addr( Hermes::MemAddr::Shmem );
            addr.setSimVAddr( m_curAddr );

            if ( m_back ) {
                addr.setBacking( ::malloc(n) );
            }
            return addr; 
        }  
        void free( Hermes::MemAddr& addr ) {
            if ( addr.getBacking() ) {
                ::free( addr.getBacking() );
            }
        }
      private:
        size_t m_curAddr;
        bool   m_back;
    };

  public:
    HadesSHMEM(Component*, Params&);
    ~HadesSHMEM() {}

    virtual void setup();
    virtual void finish() {}
    virtual std::string getName() { return "HadesSHMEM"; }

    virtual void setOS( OS* os ) {
        m_os = static_cast<Hades*>(os);
        dbg().verbose(CALL_INFO,2,0,"\n");
    }

    virtual void init(MP::Functor*);
    virtual void finalize(MP::Functor*);

    virtual void n_pes(int*, MP::Functor*);
    virtual void my_pe(int*,MP::Functor*);

    virtual void barrier_all(MP::Functor*);
    virtual void fence(MP::Functor*);

    virtual void malloc(Hermes::MemAddr*,size_t,MP::Functor*);
    virtual void free(Hermes::MemAddr*,MP::Functor*);

    virtual void get(Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems, int pe, MP::Functor*);
    virtual void put(Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems, int pe, MP::Functor*);

    virtual void getv(void* dest, Hermes::MemAddr, int size, int pe, MP::Functor*);
    virtual void putv(Hermes::MemAddr dest, uint64_t value, int size, int pe, MP::Functor*);

  private:
    Output m_dbg;
    Output& dbg() { return m_dbg; }
    Hades*      m_os;

    void handleToDriver(SST::Event* e) {
        DelayEvent* event = static_cast<DelayEvent*>(e);
        (*event->m_functor)( event->m_retval );
        delete e;
    }
    void delay( MP::Functor* functor, uint64_t delay, int retval ) {
        //printf("%s() delay=%lu retval=%d\n",__func__,delay,retval);
        m_selfLink->send( delay, new DelayEvent( functor, retval ) );
    } 

    FunctionSM& functionSM() { return m_os->getFunctionSM(); }
    SST::Link*      m_selfLink;

    Heap* m_heap;
    std::map<key_t,Hermes::MemAddr> m_map;
};

}
}
#endif
