// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEMH_TIMING_PAGEPOLICY
#define _H_SST_MEMH_TIMING_PAGEPOLICY

#include <sst/core/subcomponent.h>

namespace SST {
namespace MemHierarchy {
namespace TimingDRAM_NS {

class PagePolicy : public SST::SubComponent {
  public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::MemHierarchy::TimingDRAM_NS::PagePolicy)

    PagePolicy( Component* owner, Params& params ) : SubComponent( owner )  { }
    PagePolicy( ComponentId_t id, Params& params ) : SubComponent( id )  { }
    virtual bool shouldClose( SimTime_t current ) = 0;
};

class SimplePagePolicy : public PagePolicy {
  public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(SimplePagePolicy, "memHierarchy", "simplePagePolicy", SST_ELI_ELEMENT_VERSION(1,0,0),
            "static page open or close policy", SST::MemHierarchy::TimingDRAM_NS::PagePolicy)
    
    SST_ELI_DOCUMENT_PARAMS( {"close", "Whether to use a closed (true) or open (false) page policy", "true"} )

/* Begin class definition */
    SimplePagePolicy( Component* owner, Params& params ) : PagePolicy( owner, params )  {
        m_close = params.find<bool>("close", true);
    }
    SimplePagePolicy( ComponentId_t id, Params& params ) : PagePolicy( id, params )  {
        m_close = params.find<bool>("close", true);
    }

    bool shouldClose( SimTime_t current ) {
        return m_close;
    }
  protected:
    bool m_close;
};

class TimeoutPagePolicy : public PagePolicy {
  public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(TimeoutPagePolicy, "memHierarchy", "timeoutPagePolicy", SST_ELI_ELEMENT_VERSION(1,0,0),
            "timeout based page open or close policy", SST::MemHierarchy::TimingDRAM_NS::PagePolicy)
    
    SST_ELI_DOCUMENT_PARAMS( {"timeoutCycles", "Timeout (close page) after this many cycles", "5"} )

/* Begin class definition */
    TimeoutPagePolicy( ComponentId_t id, Params& params ) : PagePolicy( id, params ),
        m_numCyclesLeft(0), m_lastCycle(-2) { 
        m_cycles = params.find<SimTime_t>("timeoutCycles", 5);
    }

    TimeoutPagePolicy( Component* owner, Params& params ) : PagePolicy( owner, params ),
        m_numCyclesLeft(0), m_lastCycle(-2) { 
        m_cycles = params.find<SimTime_t>("timeoutCycles", 5);
    }

    bool shouldClose( SimTime_t current ) {

        if ( 0 == m_numCyclesLeft && current != m_lastCycle + 1 ) {
            m_numCyclesLeft = m_cycles + 1;
        }
        --m_numCyclesLeft;
        m_lastCycle = current;

        return (m_numCyclesLeft == 0);
    }

  protected:
    SimTime_t m_lastCycle;
    SimTime_t m_cycles;
    SimTime_t m_numCyclesLeft;
};

}
}
}

#endif
