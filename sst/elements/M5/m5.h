#ifndef _m5_h
#define _m5_h

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/introspectedComponent.h>
#include "barrier.h"

// for power modeling
////#ifdef M5_WITH_POWER
#include "../power/power.h"
// Notice: there is no using namespace SST, 
// so we should use SST::Power
bool SST::Power::p_hasUpdatedTemp __attribute__((weak));
////#endif


class SimLoopExitEvent;
class M5 : public SST::IntrospectedComponent
{
  public:
    M5( SST::ComponentId_t id, Params_t& params );
    ~M5();
    int Setup();
    int Finish();
    bool catchup( );

    void registerExit();
    void exit( int status );

    BarrierAction&  barrier() { return *m_barrier; }

  private:
    bool clock( SST::Cycle_t cycle );

    SST::Link*          m_self;
    int                 m_numRegisterExits;
    BarrierAction*      m_barrier;
    int                 m_m5ticksPerSSTclock;
    SST::Cycle_t        m_fooTicks;

    // flag for fastforwarding
    bool FastForwarding_flag;

    // parameters for power modeling
   Params_t params;
   #ifdef M5_WITH_POWER
	// For power & introspection
 	SST::Pdissipation_t pdata, pstats;
 	SST::Power *power;
	std::string frequency;
	unsigned int machineType; //0: OOO, 1: inorder
	// Over-specified struct that holds usage counts of its sub-components
 	SST::usagecounts_t mycounts, tempcounts; //M5 statistics are accumulative, so we need tempcounts to get the real statistics for a time step
	bool pushData(SST::Cycle_t);
    void Init_Power();
    void Setup_Power();
    void Finish_Power();
   #endif

};

#endif
