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


#ifndef _H_EMBER_SHMEM_TEST
#define _H_EMBER_SHMEM_TEST

#include <strings.h>
#include "shmem/emberShmemGen.h"

namespace SST {
namespace Ember {

class EmberShmemTestGenerator : public EmberShmemGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemTestGenerator,
        "ember",
        "ShmemTestMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM test",
        "SST::Ember::EmberGenerator"
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "time-Init", "Time spent in Init event",          "ns",  0},
        { "time-Finalize", "Time spent in Finalize event",  "ns", 0},
        { "time-Rank", "Time spent in Rank event",          "ns", 0},
        { "time-Size", "Time spent in Size event",          "ns", 0},
        { "time-Send", "Time spent in Recv event",          "ns", 0},
        { "time-Recv", "Time spent in Recv event",          "ns", 0},
        { "time-Irecv", "Time spent in Irecv event",        "ns", 0},
        { "time-Isend", "Time spent in Isend event",        "ns", 0},
        { "time-Wait", "Time spent in Wait event",          "ns", 0},
        { "time-Waitall", "Time spent in Waitall event",    "ns", 0},
        { "time-Waitany", "Time spent in Waitany event",    "ns", 0},
        { "time-Compute", "Time spent in Compute event",    "ns", 0},
        { "time-Barrier", "Time spent in Barrier event",    "ns", 0},
        { "time-Alltoallv", "Time spent in Alltoallv event", "ns", 0},
        { "time-Alltoall", "Time spent in Alltoall event",  "ns", 0},
        { "time-Allreduce", "Time spent in Allreduce event", "ns", 0},
        { "time-Reduce", "Time spent in Reduce event",      "ns", 0},
        { "time-Bcast", "Time spent in Bcast event",        "ns", 0},
        { "time-Gettime", "Time spent in Gettime event",    "ns", 0},
        { "time-Commsplit", "Time spent in Commsplit event", "ns", 0},
        { "time-Commcreate", "Time spent in Commcreate event", "ns", 0},
    )

public:
	EmberShmemTestGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemTest" ), m_phase(0) 
	{ }

    bool generate( std::queue<EmberEvent*>& evQ) 
	{
        bool ret = false;
        switch ( m_phase ) {
        case 0:
            enQ_init( evQ );
            break;
        case 1:
            enQ_n_pes( evQ, &m_n_pes );
            enQ_my_pe( evQ, &m_my_pe );
            break;

        case 2:

            printf("%d:%s: %d\n",m_my_pe, getMotifName().c_str(),m_n_pes);
            enQ_malloc( evQ, &m_addr, 1000*2 );
            break;

        case 3:
            
            memset( &m_addr[0], 12, 1000 );
            memset( &m_addr[1000], 0, 1000 );
            ((int*)&m_addr[32])[0] = 0xf00d0000 + m_my_pe;
            printf("%d:%s: simVAddr %#" PRIx64 " backing %p\n",m_my_pe, getMotifName().c_str(),m_addr.getSimVAddr(),m_addr.getBacking());
            enQ_barrier_all( evQ );
            enQ_putv( evQ, 
                    m_addr,
                    (int) (0xdead0000 + m_my_pe + 1), 
                    (m_my_pe + 1) % m_n_pes );
            enQ_barrier_all( evQ );
            break;

        case 4:
            printf("%d:%s: PUT value=%#" PRIx32 "\n",m_my_pe, getMotifName().c_str(), ((int*)&m_addr[0])[0]);
            enQ_get( evQ, 
                    m_addr.offset(16),
                    m_addr.offset(32), 
                    4,
                    (m_my_pe + 1) % m_n_pes );
            enQ_barrier_all( evQ );
            break;

        case 5:
            printf("%d:%s: GET value=%#" PRIx32 "\n",m_my_pe, getMotifName().c_str(), ((int*)&m_addr[16])[0]);
            enQ_get( evQ, 
                    m_addr.offset(1000),
                    m_addr.offset(0), 
                    1000,
                    (m_my_pe + 1) % m_n_pes );
            enQ_barrier_all( evQ );
            break;

        case 6:
            for ( int i = 0; i < 10; i++ ) {
                printf("%d:%s: GET value=%#" PRIx32 "\n",m_my_pe, getMotifName().c_str(), ((int*)&m_addr[1000])[i]);
            }
            memset( &m_addr[0], 0xfe, 1000 );
            enQ_barrier_all( evQ );
            enQ_put( evQ, 
                    m_addr.offset(1000),
                    m_addr.offset(0), 
                    1000,
                    (m_my_pe + 1) % m_n_pes );
            enQ_barrier_all( evQ );
            break;

        case 7: 
            for ( int i = 0; i < 10; i++ ) {
                printf("%d:%s: GET value=%#" PRIx32 "\n",m_my_pe, getMotifName().c_str(), ((int*)&m_addr[1000])[i]);
            }
            enQ_getv( evQ, 
                    &m_local,
                    m_addr,
                    (m_my_pe + 1) % m_n_pes );
            enQ_barrier_all( evQ );
            break;
        case 8: 
            printf("%d:%s: GET value=%#" PRIx32 "\n",m_my_pe, getMotifName().c_str(), m_local );

        default:
		    ret = true;
            break;
        }
        ++m_phase;
        return ret;
	}
  private:
    Hermes::MemAddr m_addr;
    int m_local;
    int m_phase;
    int m_my_pe;
    int m_n_pes;
};

}
}

#endif
