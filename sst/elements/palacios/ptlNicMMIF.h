#ifndef _ptlNicMMIF_h
#define _ptlNicMMIF_h

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/component.h>

#include <pthread.h>
#include "palacios.h"
#include "portals4/ptlNic/cmdQueue.h"
#include "barrier.h"

class PtlNicMMIF : public SST::Component
{
  public:
    PtlNicMMIF( SST::ComponentId_t, Params_t& );
    virtual ~PtlNicMMIF();
    int Setup();
    virtual bool Status();

  private:

    enum { VM_SIM_START = 1, VM_SIM_STOP, VM_SIM_TERM };
    void ptlCmdRespHandler( SST::Event* );
    void dmaHandler( SST::Event* );

    void writeFunc( unsigned long );
    void barrierLeave();
    bool clock( SST::Cycle_t );

    void doSimCtrlCmd(int cmd);
    void simCtrlCmd();

    float adjustPeriod( float a, float b ) {
        float tmp = a;
        while ( tmp < b ) tmp += a; 
        return tmp;
    }

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
        if ( ! m_threadRun ) return false; 

        m_threadRun = false;
        int ret = pthread_join( m_thread, NULL );
        assert( ret == 0 );
    
        ret = m_palaciosIF->vm_continue();
        assert( ret == 0 );

        return true;
    }

    void ptlCmd();
    void barrierCmd();

    SST::Link*                  m_cmdLink;
    SST::Link*                  m_dmaLink;
    PalaciosIF*                 m_palaciosIF;
    
    unsigned long               m_barrierOffset;
    unsigned long               m_simulationCtrlCmd;
    
    std::vector<uint8_t>        m_devMemory;
    cmdQueue_t*                 m_cmdQueue;

    static const char          *m_cmdNames[];

    BarrierAction               m_barrier;
    BarrierAction::Handler<PtlNicMMIF> m_barrierCallback;

    static void* thread1( void* );
    void* thread2();
    pthread_t                   m_thread;
    bool                        m_threadRun;
    pthread_cond_t              m_threadCond;
    pthread_mutex_t             m_threadMutex;

    uint64_t m_sstStart;
    uint64_t m_guestStart;
    uint64_t m_guestHz;

    int m_mult;
    int m_count;
    uint64_t m_runCycles;

};

#endif
