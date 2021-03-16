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


#ifndef _H_EMBER_FAM_GET_EVENT
#define _H_EMBER_FAM_GET_EVENT

#include "emberFamEvent.h"

namespace SST {
namespace Ember {

class EmberFamGet_Event : public EmberFamEvent {

public:
	EmberFamGet_Event( Shmem::Interface& api, Output* output,
            Hermes::Vaddr dest, Shmem::Fam_Descriptor fd, uint64_t offset, uint64_t nbytes, bool blocking,
            EmberEventTimeStatistic* stat = NULL ) :
            EmberFamEvent( api, output, stat ),
            m_dest(dest), m_fd(fd), m_offset(offset), m_nbytes(nbytes), m_blocking(blocking) {}
	~EmberFamGet_Event() {}

    std::string getName() { return "FamGet"; }

    void issue( uint64_t time, Shmem::Callback callback ) {

        EmberEvent::issue( time );
        m_api.fam_get( m_dest, m_fd, m_offset, m_nbytes, m_blocking, callback );
    }
private:
    Hermes::Vaddr m_dest;
	Shmem::Fam_Descriptor m_fd;
	uint64_t m_offset;
	uint64_t m_nbytes;
	bool m_blocking;
};

}
}

#endif
