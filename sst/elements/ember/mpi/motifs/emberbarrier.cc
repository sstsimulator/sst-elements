// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "emberbarrier.h"

using namespace SST::Ember;

EmberBarrierGenerator::EmberBarrierGenerator(SST::Component* owner,
                                                Params& params) :
	EmberMessagePassingGenerator(owner, params),
    m_loopIndex(0)
{
    m_name = "Barrier";
	m_iterations = (uint32_t) params.find_integer("arg.iterations", 1);
}

bool EmberBarrierGenerator::generate( std::queue<EmberEvent*>& evQ )
{
    if ( m_loopIndex == 0 ) {
        GEN_DBG( 1, "rank=%d size=%d\n", rank(), size());
    }

    enQ_compute( evQ, 11000 );
    enQ_barrier( evQ, GroupWorld ); 

    if ( ++m_loopIndex == m_iterations ) {
        return true; 
    } else {
        return false; 
    }
}
