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


#ifndef _H_EMBER_DETAILED_COMPUTE_EVENT
#define _H_EMBER_DETAILED_COMPUTE_EVENT

#include "mpi/emberMPIEvent.h"
#include "sst/elements/thornhill/detailedCompute.h"

namespace SST {
namespace Ember {

class EmberDetailedComputeEvent : public EmberEvent {

public:
	EmberDetailedComputeEvent( Output* output,
                        EmberEventTimeStatistic* stat,
                        Thornhill::DetailedCompute& api,
                        std::string& name,
                        Params& params ) :
        EmberEvent(output, stat),
        m_api(api),
        m_name(name),
        m_params(params)
    {
        m_state = IssueFunctor;
    }  

	~EmberDetailedComputeEvent() {}

    std::string getName() { return "DetailedCompute"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );
    
        std::function<int()> foo = [=](){ 
            (*functor)(0);
            return 0;
        }; 

        m_api.start( m_name, m_params, foo );
    }

protected:
    Thornhill::DetailedCompute&  m_api;
    std::string     m_name;
    Params          m_params;
};

}
}

#endif
