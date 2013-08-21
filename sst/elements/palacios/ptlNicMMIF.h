// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _ptlNicMMIF_h
#define _ptlNicMMIF_h

#include <sst_config.h>
#include <sst/core/serialization.h>
#include <sst/core/component.h>

#include <pthread.h>
#include "palacios.h"
#include "portals4/ptlNic/cmdQueue.h"
#include "barrier.h"

#define USE_THREAD 0

namespace SST {
namespace Palacios {

class PtlNicMMIF : public SST::Component
{
  public:
    PtlNicMMIF( SST::ComponentId_t, Params& );
    virtual ~PtlNicMMIF();
    void setup(); 
    virtual bool Status();

  private:

    enum { VM_SIM_START = 1, VM_SIM_STOP, VM_SIM_TERM };
    void ptlCmdRespHandler( SST::Event* );
    void dmaHandler( SST::Event* );

    void writeFunc( unsigned long );
    void barrierLeave();
    bool clock( SST::Cycle_t );
    void update(int);
    DerivedFunctor<PtlNicMMIF> m_functor;

    void doSimCtrlCmd(int cmd);
    void simCtrlCmd();

    float adjustPeriod( float a, float b ) {
        float tmp = a;
        while ( tmp < b ) tmp += a; 
        return tmp;
    }

#if USE_THREAD 
    bool sim_mode_start() {
        if ( m_threadRun ) return false; 

        int ret = m_palaciosIF->vm_pause();
        assert( ret == 0 );

        m_threadRun = true;
        ret = pthread_create( &m_thread, NULL, thread1, this );
        assert( ret == 0 );

        return true;
    }

    bool sim_mode_stop() {
        int ret;
        if ( ! m_threadRun ) return false; 

        m_threadRun = false;
        
        ret = pthread_barrier_wait( &m_threadBarrier );
        assert (ret == 0 || ret == PTHREAD_BARRIER_SERIAL_THREAD );

        ret = pthread_join( m_thread, NULL );
        assert( ret == 0 );

        ret = m_palaciosIF->vm_continue();
        assert( ret == 0 );

        return true;
    }
#endif

    void ptlCmd(int);
    void barrierCmd(int);

    SST::Link*                  m_cmdLink;
    SST::Link*                  m_dmaLink;
    PalaciosIF*                 m_palaciosIF;
    
    unsigned long               m_barrierOffset;
    unsigned long               m_simulationCtrlCmd;
    
    std::vector<uint8_t>        m_devMemory;
    Portals4::cmdQueue_t*       m_cmdQueue;

    static const char          *m_cmdNames[];

    BarrierAction               m_barrier;
    BarrierAction::Handler<PtlNicMMIF> m_barrierCallback;

#if USE_THREAD 
    static void* thread1( void* );
    void* thread2();
    bool                        m_threadRun;
    pthread_t                   m_thread;
    pthread_barrier_t           m_threadBarrier;
#else
    bool                        m_simulating;
#endif

    int64_t    m_runCycles;
    int64_t    m_runCyclesJitter;

    uint64_t    m_sstStart;
    uint64_t    m_guestStart;
    uint64_t    m_guestHz;
    time_t      m_simStartWallTime;
};

}}

#endif
