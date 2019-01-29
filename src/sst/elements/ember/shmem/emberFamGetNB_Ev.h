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


#ifndef _H_EMBER_FAM_GET_NB_EVENT
#define _H_EMBER_FAM_GET_NB_EVENT

#include "emberFamEvent.h"

namespace SST {
namespace Ember {

class EmberFamGetNB_Event : public EmberFamEvent {

public:
	EmberFamGetNB_Event( Shmem::Interface& api, Output* output,
            Hermes::Vaddr dest, Shmem::Fam_Region_Descriptor rd, uint64_t offset, uint64_t nbytes,
            EmberEventTimeStatistic* stat = NULL ) :
            EmberFamEvent( api, output, stat ), 
            m_dest(dest), m_rd(rd), m_offset(offset), m_nbytes(nbytes) {}
	~EmberFamGetNB_Event() {}

    std::string getName() { return "FamGetNB"; }

    void issue( uint64_t time, Shmem::Callback callback ) {

        EmberEvent::issue( time );
        m_api.fam_get_nb( m_dest, m_rd, m_offset, m_nbytes, callback );
    }
private:
    Hermes::Vaddr m_dest;
	Shmem::Fam_Region_Descriptor m_rd;
	uint64_t m_offset;
	uint64_t m_nbytes;
};

}
}

#endif
