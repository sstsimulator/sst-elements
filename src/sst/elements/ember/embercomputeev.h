// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_COMPUTE_EVENT
#define _H_EMBER_COMPUTE_EVENT

#include "mpi/emberMPIEvent.h"
#include "emberconstdistrib.h"

namespace SST {
namespace Ember {

class EmberComputeEvent : public EmberEvent {

public:
	EmberComputeEvent( Output* output,
                      EmberEventTimeStatistic* stat, uint64_t nanoSecondDelay,
                EmberComputeDistribution* dist) :
        EmberEvent(output, stat),
        m_nanoSecondDelay( nanoSecondDelay ),
        m_computeDistrib(dist),
        m_calcFunc(NULL)
    {}  

	EmberComputeEvent( Output* output,
                      EmberEventTimeStatistic* stat, std::function<uint64_t()> func,
                EmberComputeDistribution* dist) :
        EmberEvent(output, stat),
        m_computeDistrib(dist),
        m_calcFunc(func)
    {}  

	~EmberComputeEvent() {}

    std::string getName() { return "Compute"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );
    
        if ( m_calcFunc ) {
            m_completeDelayNS = (double) m_calcFunc(); 
        } else {
            m_completeDelayNS = (double) m_nanoSecondDelay;
        }
        m_completeDelayNS *= m_computeDistrib->sample(time);

        m_output->verbose(CALL_INFO, 2, 0, "Adjust time by noise "
                "distribution to give: %" PRIu64 "ns\n", m_completeDelayNS );
    }

protected:
	uint64_t m_nanoSecondDelay;
    EmberComputeDistribution* m_computeDistrib;
    std::function<uint64_t()> m_calcFunc; 

};

}
}

#endif
