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


#ifndef _H_EMBER_SHMEM_REDUCTION
#define _H_EMBER_SHMEM_REDUCTION

#include <strings.h>
#include "shmem/emberShmemGen.h"

namespace SST {
namespace Ember {

class EmberShmemReductionGenerator : public EmberShmemGenerator {

    enum { AND, OR, XOR, SUM, PROD, MAX, MIN } m_op;
public:
	EmberShmemReductionGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemReduction" ), m_phase(0) 
	{ 
        m_seed = 100100;
        m_opName = params.find<std::string>("arg.op", "AND");
        m_nelems = 1;
        if ( 0 == m_opName.compare("AND") ) {
            m_op = AND;
            m_result = m_seed * 1 & m_seed * 2; 
        } else if ( 0 == m_opName.compare("OR") ) {
            m_op = OR;
            m_result = m_seed * 1 | m_seed * 2; 
        } else if ( 0 == m_opName.compare("XOR") ) {
            m_op = XOR;
            m_result = m_seed * 1 ^ m_seed * 2; 
        } else if ( 0 == m_opName.compare("SUM") ) {
            m_op = SUM;
            m_result = m_seed * 1 + m_seed * 2; 
        } else if ( 0 == m_opName.compare("PROD") ) {
            m_op = PROD;
            m_result = m_seed * 1 * m_seed * 2; 
        } else if ( 0 == m_opName.compare("MAX") ) {
            m_op = MAX;
            m_result = m_seed * 2;
        } else if ( 0 == m_opName.compare("MIN") ) {
            m_op = MIN;
            m_result = m_seed * 1;
        } else {
            assert(0);
        }
    }

    bool generate( std::queue<EmberEvent*>& evQ) 
	{
        bool ret = false;
        switch ( m_phase ) {
          case 0: 
            enQ_init( evQ );
            enQ_my_pe( evQ, &m_my_pe );
            enQ_n_pes( evQ, &m_num_pes );
            // need 3 longs for psync
            enQ_malloc( evQ, &m_memory, 24 + 16 );
            break;
          case 1:
            m_pSync = m_memory;
            m_src = m_memory.offset<long>(3);
            m_dest = m_memory.offset<long>(4);

            m_pSync.at<long>(0) = 0;
            m_pSync.at<long>(1) = 0;
            m_pSync.at<long>(2) = 0;

            m_src.at<int>(0) = m_seed * (m_my_pe + 1);
            m_dest.at<int>(0) = 0;

            enQ_barrier_all( evQ );
            break;

          case 2:
            switch ( m_op ) { 
            case AND:
                enQ_int_and_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pWrk, m_pSync );
                break;
            case OR:
                enQ_int_or_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pWrk, m_pSync );
                break;
            case XOR:
                enQ_int_xor_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pWrk, m_pSync );
                break;
            case SUM:
                enQ_int_sum_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pWrk, m_pSync );
                break;
            case PROD:
                enQ_int_prod_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pWrk, m_pSync );
                break;
            case MIN:
                enQ_int_min_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pWrk, m_pSync );
                break;
            case MAX:
                enQ_int_max_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pWrk, m_pSync );
                break;
            }
            break;

          case 3:
            printf("%d:%s: %d\n",m_my_pe, getMotifName().c_str(), m_dest.at<int>(0));
            assert( m_dest.at<int>(0) == m_result );
            ret = true;

        }
        ++m_phase;

        return ret;
	}

  private:
    std::string m_opName;
    Hermes::MemAddr m_memory;
    Hermes::MemAddr m_pSync;
    Hermes::MemAddr m_src;
    Hermes::MemAddr m_dest;
    Hermes::MemAddr m_pWrk;
    int m_seed;
    int m_result;
    int m_nelems;
    int m_my_pe;
    int m_num_pes;
    int m_phase;
};

}
}

#endif
