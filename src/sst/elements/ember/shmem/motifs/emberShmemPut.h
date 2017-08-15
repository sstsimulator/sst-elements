// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_SHMEM_PUT
#define _H_EMBER_SHMEM_PUT

#include <strings.h>
#include "shmem/emberShmemGen.h"

namespace SST {
namespace Ember {

class EmberShmemPutGenerator : public EmberShmemGenerator {

public:
	EmberShmemPutGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemPut" ), m_phase(0), m_numInts(100) 
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

            printf("%d:%s: num_pes=%d\n",m_my_pe, getMotifName().c_str(),m_n_pes);
            m_other_pe = (m_my_pe + 1) % m_n_pes;
            enQ_malloc( evQ, &m_addr, m_numInts * sizeof(int) * 2);
            break;

        case 3:
            
            m_from = &m_addr.at<int>(0);
            m_to =   &m_addr.at<int>(m_numInts);
            printf("%d:%s: %p %p\n",m_my_pe, getMotifName().c_str(),m_from,m_to);
            printf("%d:%s: %#" PRIx64 " %#" PRIx64 "\n",m_my_pe, getMotifName().c_str(),
                    m_addr.getSimVAddr(),m_addr.getSimVAddr<int>(m_numInts));
            bzero( m_to, m_numInts * sizeof(int) );

            for ( int i = 0; i < m_numInts; i++ ) {
                m_from[i] = m_my_pe + i;
            }

            enQ_barrier_all( evQ );
            break;

        case 4:
            enQ_put( evQ, 
                    m_addr.offset<int>(m_numInts),
                    m_addr.offset(0), 
                    m_numInts*sizeof(int),
                    m_other_pe );
            enQ_barrier_all( evQ );
            break;

        case 5: 
            for ( int i = 0; i < m_numInts; i++ ) {
                //printf("%d:%s: %d %d\n",m_my_pe, getMotifName().c_str(),i + m_other_pe, m_to[i]);
                assert( i + m_other_pe == m_to[i] );
            }
		    ret = true;
        }
        ++m_phase;
        return ret;
	}
  private:
    Hermes::MemAddr m_addr;
    int m_numInts;
    int* m_from;
    int* m_to;
    int m_phase;
    int m_my_pe;
    int m_other_pe;
    int m_n_pes;
};

}
}

#endif
