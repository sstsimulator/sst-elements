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
#include "shmem/common.h"
#include "shmem/barrier.h"
#include "shmem/broadcast.h"
#include "shmem/alltoall.h"
#include "shmem/reduction.h"

using namespace Hermes;

namespace SST {
namespace Firefly {

class HadesSHMEM : public Shmem::Interface
{
  public:
    typedef std::function<void()> Callback;

  private:
    class DelayEvent : public SST::Event {
      public:

        DelayEvent( Shmem::Callback callback, int retval ) :
            Event(),
            m_callback( callback ),
            m_retval( retval )
        {}

        Shmem::Callback m_callback;
        int m_retval;

        NotSerializable(DelayEvent)
    };

    class Heap {
      public:
        Heap( bool back = false ) : m_curAddr(0x1000), m_back(back) {}
        Hermes::MemAddr malloc( size_t n ) {
            Hermes::MemAddr addr( Hermes::MemAddr::Shmem );
            addr.setSimVAddr( m_curAddr );

            m_curAddr += n;
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
    ~HadesSHMEM() { delete m_heap; }

    virtual void setup();
    virtual void finish() {}
    virtual std::string getName() { return "HadesSHMEM"; }

    virtual void setOS( OS* os ) {
        m_os = static_cast<Hades*>(os);
        dbg().verbose(CALL_INFO,2,0,"\n");
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
    virtual void alltoall( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
                        int logPE_stride, int PE_size, Vaddr, Shmem::Callback);
    virtual void reduction( Vaddr dest, Vaddr source, int nelems, int PE_start,
                        int logPE_stride, int PE_size, Vaddr pWrk, Vaddr pSync,
                        Hermes::Shmem::ReduOp, Hermes::Value::Type, Shmem::Callback);

    virtual void malloc(Hermes::MemAddr*,size_t,Shmem::Callback);
    virtual void malloc(Hermes::MemAddr*,size_t,Callback);
    virtual void free(Hermes::MemAddr*,Shmem::Callback);

    virtual void get(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe, Shmem::Callback);
    virtual void put(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe, Shmem::Callback);
    virtual void putOp(Hermes::Vaddr dest, Hermes::Vaddr src, size_t nelems, int pe, 
            Hermes::Shmem::ReduOp, Hermes::Value::Type, Shmem::Callback);

    virtual void getv(Hermes::Value&, Hermes::Vaddr src, int pe, Shmem::Callback);
    virtual void putv(Hermes::Vaddr dest, Hermes::Value&, int pe, Shmem::Callback);

    virtual void wait_until(Hermes::Vaddr src, Hermes::Shmem::WaitOp, Hermes::Value&, Shmem::Callback);

    virtual void cswap( Hermes::Value& result, Hermes::Vaddr, Hermes::Value& cond, Hermes::Value& value, int pe, Shmem::Callback);
    virtual void swap( Hermes::Value& result, Hermes::Vaddr, Hermes::Value& value, int pe, Shmem::Callback);
    virtual void fadd( Hermes::Value&, Hermes::Vaddr, Hermes::Value&, int pe, Shmem::Callback);

    void delay( Shmem::Callback functor, uint64_t delay, int retval ) {
        //printf("%s() delay=%lu retval=%d\n",__func__,delay,retval);
        m_selfLink->send( delay, new DelayEvent( functor, retval ) );
    } 

  private:
    Output m_dbg;
    Output& dbg() { return m_dbg; }
    Hades*      m_os;

    void handleToDriver(SST::Event* e) {
        DelayEvent* event = static_cast<DelayEvent*>(e);
        event->m_callback( event->m_retval );
        delete e;
    }

    FunctionSM& functionSM() { return m_os->getFunctionSM(); }
    SST::Link*      m_selfLink;

    Heap* m_heap;
    std::map<key_t,Hermes::MemAddr> m_map;

    ShmemCommon*    m_common;
    ShmemBarrier*   m_barrier;
    ShmemBroadcast* m_broadcast;
    ShmemAlltoall*  m_alltoall;
    ShmemReduction* m_reduction;

    int m_num_pes;
    int m_my_pe;
    size_t m_localDataSize;
    Hermes::MemAddr  m_localData;
    Hermes::MemAddr  m_psync;
};

}
}
#endif
