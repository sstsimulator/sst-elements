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

#ifndef _SWM_WORKLOAD_H
#define _SWM_WORKLOAD_H

#include <thread>

#include "lammps.h"
#include "nekbone_swm_user_code.h"
#include "nearest_neighbor_swm_user_code.h"
#include "all_to_one_swm_user_code.h"
#include "many_to_many_swm_user_code.h"
#include "milc_swm_user_code.h"

#include "sst/elements/swm/convert.h"

#if 0
#define WorkloadDBG( rank, format, ... ) \
    printf( "%d:Workload::%s():%d " format, rank, __func__, __LINE__, ##__VA_ARGS__)

#else
#define WorkloadDBG( rank, format, ... )

#endif

namespace SST {
namespace Swm {

class Workload {

  public:
    Workload( Convert* convert, std::string path, std::string name, int rank, uint32_t verboseLevel, uint32_t verboseMask );
    enum { Lammps,Nekbone,NN,MM,MILC,Incast } m_type;
	void start();
	void stop();
    void call() {
        switch ( m_type ) {
            case Lammps: m_lammps->call(); break;
            case Nekbone: m_nekbone->call(); break;
            case NN: m_nn->call(); break;
            case Incast: m_incast->call(); break;
            case MM: m_mm->call(); break;
            case MILC: m_milc->call(); break;
            default: assert(0);
        }
    }
	int rank()         { return m_rank; }
	Convert& convert() { return *m_convert; }

    double calcComputeTime( long cycle_count ) {
        double cpu_freq_hz = m_cpuFreq * 1000.0 * 1000.0 * 1000.0;
        double delay_in_seconds = cycle_count / cpu_freq_hz;
        return delay_in_seconds * 1000.0 * 1000.0 * 1000.0;
    }


  private:

    LAMMPS_SWM*                 m_lammps;
    NEKBONESWMUserCode*         m_nekbone;
    AllToOneSWMUserCode*        m_incast;
    NearestNeighborSWMUserCode* m_nn;
	ManyToManySWMUserCode*      m_mm;
	MilcSWMUserCode*			m_milc;

	std::thread m_thread;
	Convert*    m_convert;
    Output      m_output;
	int         m_rank;
    int         m_numRanks;
    double      m_cpuFreq;
    bool        m_readConfig;
};

}
}

#endif
