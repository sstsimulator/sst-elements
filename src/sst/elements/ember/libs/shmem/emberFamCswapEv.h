// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_FAM_CSWAP_EVENT
#define _H_EMBER_FAM_CSWAP_EVENT

#include "emberFamEvent.h"

namespace SST {
namespace Ember {

class EmberFamCswapEvent : public EmberFamEvent {

public:
	EmberFamCswapEvent( Shmem::Interface& api, Output* output,
			Hermes::Value result, Shmem::Fam_Descriptor fd, uint64_t dest, Hermes::Value oldValue, Hermes::Value newValue,
            EmberEventTimeStatistic* stat = NULL ) :
            EmberFamEvent( api, output, stat ),
            m_result(result), m_fd(fd), m_dest(dest), m_oldValue(oldValue), m_newValue(newValue) {}
	~EmberFamCswapEvent() {}

    std::string getName() { return "Fam_Add"; }

    void issue( uint64_t time, Shmem::Callback callback ) {
        EmberEvent::issue( time );
        m_api.fam_cswap( m_result, m_fd, m_dest, m_oldValue, m_newValue, callback );
    }

private:
	Shmem::Fam_Descriptor m_fd;
    uint64_t m_dest;
    Hermes::Value m_result;
    Hermes::Value m_oldValue;
    Hermes::Value m_newValue;
};

}
}

#endif
