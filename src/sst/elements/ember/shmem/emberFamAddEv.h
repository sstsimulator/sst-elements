// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_FAM_ADD_EVENT
#define _H_EMBER_FAM_ADD_EVENT

#include "emberFamEvent.h"

namespace SST {
namespace Ember {

class EmberFamAddEvent : public EmberFamEvent {

public:
	EmberFamAddEvent( Shmem::Interface& api, Output* output,
            uint64_t dest, Hermes::Value value, 
            EmberEventTimeStatistic* stat = NULL ) :
            EmberFamEvent( api, output, stat ), 
            m_dest(dest), m_value(value) {}
	~EmberFamAddEvent() {}

    std::string getName() { return "Fam_Add"; }

    void issue( uint64_t time, Shmem::Callback callback ) {

        EmberEvent::issue( time );
        m_api.fam_add( m_dest, m_value, callback );
    }

private:
    uint64_t m_dest;
    Hermes::Value m_value;
};

}
}

#endif
