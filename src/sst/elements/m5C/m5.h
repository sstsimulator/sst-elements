// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _m5_h
#define _m5_h

#include <sst/core/introspectedComponent.h>
#include <sst/core/params.h>

#include "barrier.h"

#include <util.h>

// for power modeling
#ifdef M5_WITH_POWER
#include "../power/power.h"
// Notice: there is no using namespace SST, 
// so we should use SST::Power
bool SST::Power::p_hasUpdatedTemp __attribute__((weak));
#endif

class SimLoopExitEvent;
namespace SST {
namespace M5 {

class PortLink;

class M5 : public SST::IntrospectedComponent
{
  public:
    M5( SST::ComponentId_t id, Params& params );
    ~M5();
	void init(unsigned int);
    void setup();
    void finish();

    void registerExit();
    void exit( int status );

    BarrierAction&  barrier() { return *m_barrier; }

    objectMap_t& objectMap() { return m_objectMap; }

    void registerPortLink(const std::string& name, PortLink *pl );

  private:
    bool clock( SST::Cycle_t cycle );

    SST::Link*          m_self;
    int                 m_numRegisterExits;
    BarrierAction*      m_barrier;
    int                 m_m5ticksPerSSTclock;
    objectMap_t         m_objectMap;
	std::string         m_init_link_name;
    PortLink*           m_init_link;

    // flag for fastforwarding
    bool FastForwarding_flag;

    Output* portLinkDbg;
    std::string m_statFile;

    // parameters for power modeling
   Params params;
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

}
}

#endif
