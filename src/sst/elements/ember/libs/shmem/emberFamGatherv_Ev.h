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


#ifndef _H_EMBER_FAM_GATHERV_EVENT
#define _H_EMBER_FAM_GATHERV_EVENT

#include "emberFamEvent.h"

namespace SST {
namespace Ember {

class EmberFamGatherv_Event : public EmberFamEvent {

public:
	EmberFamGatherv_Event( Shmem::Interface& api, Output* output,
			Hermes::Vaddr dest, Shmem::Fam_Descriptor fd, uint64_t nblocks, std::vector<uint64_t> indexes,
			uint64_t blockSize, bool blocking, EmberEventTimeStatistic* stat = NULL ) :
			EmberFamEvent( api, output, stat ), m_dest(dest), m_fd(fd), m_nblocks(nblocks), m_indexes(indexes),
			m_blockSize(blockSize), m_blocking(blocking) {}

	~EmberFamGatherv_Event() {}

    std::string getName() { return "FamGatherv"; }

    void issue( uint64_t time, Shmem::Callback callback ) {

        EmberEvent::issue( time );
        m_api.fam_gatherv( m_dest, m_fd, m_nblocks, m_indexes, m_blockSize, m_blocking, callback );
    }
private:
    Hermes::Vaddr m_dest;
	Shmem::Fam_Descriptor m_fd;
	uint64_t m_nblocks;
	std::vector<uint64_t> m_indexes;
	uint64_t m_blockSize;
	bool m_blocking;
};

}
}

#endif
