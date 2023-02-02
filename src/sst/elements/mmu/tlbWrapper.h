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


#ifndef TLB_WRAPPER_H
#define TLB_WRAPPER_H

#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include "sst/elements/memHierarchy/memEvent.h"
#include "tlb.h"

namespace SST {

namespace MMU_Lib {

class TLB_Wrapper : public SST::Component {
  public:

    SST_ELI_REGISTER_COMPONENT(
        TLB_Wrapper,                    // Component class
        "mmu",                          // Component library (for Python/library lookup)
        "tlb_wrapper",                  // Component name (for Python/library lookup)
        SST_ELI_ELEMENT_VERSION(1,0,0), // Version of the component (not related to SST version)
        "holds a TLB",                  // Description
        COMPONENT_CATEGORY_UNCATEGORIZED// Category
    )

    SST_ELI_DOCUMENT_STATISTICS()

    SST_ELI_DOCUMENT_PARAMS(
        {"dbg_level", "Level of verbosity in debug","1"},
        {"exe", "instruction TLB","0"},
    )

    SST_ELI_DOCUMENT_PORTS(
        { "cpu_if", "Interface to cpu", {} },
        { "cache_if", "Interface to cache", {} },
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    TLB_Wrapper(SST::ComponentId_t id, SST::Params& params);
    ~TLB_Wrapper() {}

    void init(unsigned int phase);

  private:

    size_t roundDown( size_t num, size_t step ) {
        return num & ~( step - 1 );
    }

    uint32_t getPerms( MemHierarchy::MemEvent* ev ) {
        uint32_t perms = m_exe;
        switch( ev->getCmd() ) {
          case MemHierarchy::Command::GetS:
            perms |= 1<<2;
            break;
          case MemHierarchy::Command::Write:
            perms |= 1<<1;
            break;
          default:
            assert(0);
        }
        return perms;
    }

    void tlbCallback( RequestID reqId, uint64_t physAddr );

    void handleCpuEvent( Event* );
    void handleCacheEvent( Event* );

    Link* m_cpu_if;
    Link* m_cache_if;
    TLB* m_tlb;
    uint32_t m_exe;

    SST::Output m_dbg;
    int m_pending;

    /* Record noncacheable regions (e.g., MMIO device addresses) */
    std::multimap<MemHierarchy::Addr, MemHierarchy::MemRegion> noncacheableRegions;

};

} //namespace MMU_Lib
} //namespace SST

#endif /* TLB_WRAPPER_H */
