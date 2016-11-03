// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_DETAILED_COMPUTE_EVENT
#define _H_EMBER_DETAILED_COMPUTE_EVENT

#include "emberevent.h"
#include "sst/elements/thornhill/detailedCompute.h"

namespace SST {
namespace Ember {

class EmberDetailedComputeEvent : public EmberEvent {

public:
	EmberDetailedComputeEvent( Output* output,
                        Thornhill::DetailedCompute& api,
                        std::string& name,
                        Params& params ) :
        EmberEvent(output),
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

        std::deque< std::pair< std::string, SST::Params> > tmp;
        tmp.push_back( std::make_pair( m_name, m_params ) );
        m_api.start( tmp, foo );
    }

protected:
    Thornhill::DetailedCompute&  m_api;
    std::string     m_name;
    Params          m_params;
};

}
}

#endif
