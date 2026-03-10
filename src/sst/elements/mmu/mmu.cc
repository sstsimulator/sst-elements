// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <math.h>
#include "mmu.h"
#include "mmuEvents.h"
#include "utils.h"

using namespace SST;
using namespace SST::MMU_Lib;

MMU::MMU(SST::ComponentId_t id, SST::Params& params) : SubComponent(id), nic_tlb_link_(nullptr)
{
    num_cores_ = params.find<uint32_t>("num_cores", 1);
    if ( 0 == num_cores_ ) {
        dbg_.fatal(CALL_INFO, -1, "Error: %s, num_cores is zero\n",getName().c_str());
    }

    num_hw_threads_ = params.find<uint32_t>("num_threads", 1);
    if ( 0 == num_hw_threads_ ) {
        dbg_.fatal(CALL_INFO, -1, "Error: %s, num_threads is zero\n",getName().c_str());
    }

    uint32_t page_size = params.find<uint32_t>("page_size", 4096);
    if ( 0 == page_size ) {
        dbg_.fatal(CALL_INFO, -1, "Error: %s, page_size is zero\n",getName().c_str());
    } else if ( !isPowerOfTwo( page_size) ) {
        dbg_.fatal(CALL_INFO, -1, "Error: %s, page_size must be a power of two. Got '%" PRIu32 "'\n", getName().c_str(), page_size);
    }
    page_shift_ = log2( page_size );

    bool found;
    auto use_nic_tlb = params.find<bool>("use_nic_tlb",false, found);
    if (!found) {
        use_nic_tlb = params.find<bool>("useNicTlb",false, found);
        if (found) {
            dbg_.output("Warning: %s, The parameter 'useNicTlb' has been renamed to 'use_nic_tlb' and the old parameter name is deprecated. Update your input file.\n", getName().c_str());
        }
    }

    for ( int i = 0; i < num_cores_; i++ ) {
        std::string name;
        std::ostringstream core;
        core << i;

        name = "core" + core.str() + ".dtlb";
        Link* dtlb = configureLink( name, new Event::Handler2<MMU,&MMU::handleTlbEvent,int>(this, i * 2 + 0 ) );
        if ( nullptr == dtlb ) {
            dbg_.fatal(CALL_INFO, -1, "Error: %s was unable to configure dtlb link `%s`\n",getName().c_str(),name.c_str());
        }

        name = "core" + core.str() + ".itlb";
        Link* itlb = configureLink( name, new Event::Handler2<MMU,&MMU::handleTlbEvent,int>(this, i * 2 + 1 ) );
        if ( nullptr == itlb ) {
            dbg_.fatal(CALL_INFO, -1, "Error: %s was unable to configure itlb link `%s`\n",getName().c_str(),name.c_str());
        }
        core_links_.push_back( new CoreTlbLinks( dtlb, itlb ));
    }

    if ( use_nic_tlb ) {
        std::string name = "nicTlb";
        nic_tlb_link_ = configureLink( name, new Event::Handler2<MMU,&MMU::handleNicTlbEvent>(this) );
        if ( nullptr == nic_tlb_link_ ) {
            dbg_.fatal(CALL_INFO, -1, "Error: %s was unable to configure itlb link `%s`\n",getName().c_str(),name.c_str());
        }
    }
}

void MMU::init( unsigned int phase )
{
    if ( 0 == phase ) {
        for ( int i=0; i < core_links_.size(); i++ ) {
            dbg_.debug(CALL_INFO_LONG,1,0,"send Init event to %d, page_shift=%" PRIu32 "\n",i, page_shift_);
            core_links_[i]->dtlb->sendUntimedData( new TlbInitEvent( page_shift_ ) );
            core_links_[i]->itlb->sendUntimedData( new TlbInitEvent( page_shift_ ) );
        }
        if ( nic_tlb_link_ ) {
            nic_tlb_link_->sendUntimedData( new TlbInitEvent( page_shift_ ) );
        }
    }
}
