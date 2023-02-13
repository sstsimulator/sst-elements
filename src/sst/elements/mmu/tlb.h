// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef TLB_H
#define TLB_H

#include <sst/core/sst_types.h>
#include <sst/core/subcomponent.h>
#include "mmuTypes.h"

namespace SST {

namespace MMU_Lib {

class TLB : public SubComponent {

  public:

    typedef std::function<void(uint64_t,uint64_t)> Callback;

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::MMU_Lib::TLB)
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS()
    SST_ELI_DOCUMENT_PARAMS()
    SST_ELI_DOCUMENT_STATISTICS()

    TLB(SST::ComponentId_t id, SST::Params& params) : SubComponent(id) {}
    virtual ~TLB() {}

    virtual void init(unsigned int phase) {};

    virtual void registerCallback( Callback& callback  ) = 0;
    virtual void getVirtToPhys( RequestID reqId, int hwThreadId, uint64_t virtAddr, uint32_t perms, uint64_t instPtr ) = 0;

  protected:
    Callback m_callback;
    Output m_dbg;
};

class PassThroughTLB : public TLB {
  public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(PassThroughTLB, "mmu", "passThroughTLB",
                                          SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                          "Pass-through TLB, allways hits and returns virtAddr as physAddr",
                                          SST::MMU_Lib::PassThroughTLB)

    PassThroughTLB(SST::ComponentId_t id, SST::Params& params) : TLB(id,params) {}

    void registerCallback( Callback& callback ) {
        m_callback = callback; 
    }

    void getVirtToPhys( RequestID reqId, int hwThreadId, uint64_t virtAddr, uint32_t perms, uint64_t instPtr  ) {
        m_callback( reqId, virtAddr );
    }
};

} //namespace MMU_Lib
} //namespace SST

#endif /* TLB_H */
