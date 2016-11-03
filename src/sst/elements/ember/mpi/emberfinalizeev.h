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


#ifndef _H_EMBER_FINALIZE_EVENT
#define _H_EMBER_FINALIZE_EVENT

#include "emberMPIEvent.h"

namespace SST {
namespace Ember {

class EmberFinalizeEvent : public EmberMPIEvent {

public:
	EmberFinalizeEvent( MP::Interface& api, Output* output,
                        EmberEventTimeStatistic* stat ) :
            EmberMPIEvent( api, output, stat ){}
	~EmberFinalizeEvent() {}

    std::string getName() { return "Finalize"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );

        m_api.fini( functor );
    }

  private:

};

}
}

#endif
