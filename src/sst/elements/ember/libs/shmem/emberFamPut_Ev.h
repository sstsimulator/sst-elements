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


#ifndef _H_EMBER_FAM_PUT_EVENT
#define _H_EMBER_FAM_PUT_EVENT

#include "emberFamEvent.h"

namespace SST {
namespace Ember {

class EmberFamPut_Event : public EmberFamEvent {

public:
	EmberFamPut_Event( Shmem::Interface& api, Output* output,
            Shmem::Fam_Descriptor fd, uint64_t offset, Hermes::Vaddr src, uint64_t nbytes, bool blocking,
            EmberEventTimeStatistic* stat = NULL ) :
            EmberFamEvent( api, output, stat ),
            m_src(src), m_fd(fd), m_offset(offset), m_nbytes(nbytes), m_blocking(blocking) {}
	~EmberFamPut_Event() {}

    std::string getName() { return "FamPut"; }

    void issue( uint64_t time, Shmem::Callback callback ) {

        EmberEvent::issue( time );
        m_api.fam_put( m_fd, m_offset, m_src, m_nbytes, m_blocking, callback );
    }
private:
    Hermes::Vaddr m_src;
	Shmem::Fam_Descriptor m_fd;
	uint64_t m_offset;
	uint64_t m_nbytes;
	bool m_blocking;
};

}
}

#endif
