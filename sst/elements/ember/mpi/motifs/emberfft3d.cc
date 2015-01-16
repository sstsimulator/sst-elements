// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include "emberfft3d.h"

using namespace SST::Ember;

EmberFFT3DGenerator::EmberFFT3DGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params), 
	m_loopIndex(-1)
{
	m_name = "FFT3D";

	m_nx  = (uint32_t) params.find_integer("arg.nx", 100);
	m_ny  = (uint32_t) params.find_integer("arg.ny", 100);
	m_nz  = (uint32_t) params.find_integer("arg.nz", 100);

	m_npRow = (uint32_t) params.find_integer("arg.npRow", 0);
    assert( 0 < m_npRow );

#if 0
	uint64_t pe_flops = (uint64_t) params.find_integer("arg.peflops", 10000000000);

	const uint64_t total_grid_points = (uint64_t) (nx * ny * nz);
	const uint64_t total_flops       = total_grid_points * ((uint64_t) items_per_cell) * ((uint64_t) flops_per_cell);

	// Converts FLOP/s into nano seconds of compute
	const double compute_seconds = ( (double) total_flops / ( (double) pe_flops / 1000000000.0 ) );
	nsCompute  = (uint64_t) params.find_integer("arg.computetime", (uint64_t) compute_seconds);
#endif
	m_nsCopyTime = (uint32_t) params.find_integer("arg.copytime", 0);
	m_iterations = (uint32_t) params.find_integer("arg.iterations", 1);
}


void EmberFFT3DGenerator::configure()
{
    unsigned int npCol = size() / m_npRow; 
    unsigned myRow = rank() / npCol; 
    unsigned myCol = rank() % npCol; 
    printf("%d: nx=%d ny=%d nx=%d npRow=%d npCol=%d myRow=%d myCol=%d\n", 
            rank(),
            m_nx, m_ny, m_nz, m_npRow, npCol, myRow, myCol );

    m_rowGrpRanks.resize(  npCol );

    for ( unsigned col = 0; col < npCol; col++ ) {
        m_rowGrpRanks[col] = myRow * npCol + col;
//        printf("%d: %d\n",rank(), m_rowGrpRanks[col] );
    }

    m_colGrpRanks.resize(  m_npRow );
    for ( unsigned row = 0; row < m_npRow; row++ ) {
        m_colGrpRanks[row] = myCol + row * npCol;
//        printf("%d: %d\n",rank(), m_colGrpRanks[row] );
    }
}

bool EmberFFT3DGenerator::generate( std::queue<EmberEvent*>& evQ ) 
{
    GEN_DBG( 1, "loop=%d\n", m_loopIndex );

    printf("%s()::%d %d\n",__func__,__LINE__,m_loopIndex);
    if (  m_loopIndex < 0 ) {
        enQ_commCreate( evQ, GroupWorld, m_rowGrpRanks, &m_rowComm ); 
        enQ_commCreate( evQ, GroupWorld, m_colGrpRanks, &m_colComm ); 
        ++m_loopIndex;
        return false;
    }

//    enQ_compute( evQ, nsCompute);

    if ( ++m_loopIndex == (signed) m_iterations ) {
        enQ_commDestroy( evQ, m_rowComm ); 
        enQ_commDestroy( evQ, m_colComm ); 
        return true;
    } else {
        return false;
    }
}
