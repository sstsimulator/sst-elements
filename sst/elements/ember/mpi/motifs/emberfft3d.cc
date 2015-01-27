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

	m_iterations = (uint32_t) params.find_integer("arg.iterations", 1);
    assert( 1 == m_iterations ); 
}


void EmberFFT3DGenerator::configure()
{
    unsigned int npCol = size() / m_npRow; 

    assert(  (m_nx % npCol) == 0 ); 
    assert(  (m_nz % npCol) == 0 ); 
    assert(  (m_ny % m_npRow) == 0 ); 

    unsigned myRow = rank() % m_npRow; 
    unsigned myCol = rank() / m_npRow; 
    m_output->verbose(CALL_INFO, 2, 0, "%d: nx=%d ny=%d nx=%d npRow=%d "
        "npCol=%d myRow=%d myCol=%d\n", 
            rank(), m_nx, m_ny, m_nz, m_npRow, npCol, myRow, myCol );

    m_rowGrpRanks.resize( npCol);
    m_colGrpRanks.resize(  m_npRow );
    std::ostringstream tmp; 
    for ( unsigned int i = 0; i < m_colGrpRanks.size(); i++ ) {
        m_rowGrpRanks[i] = myRow + i * m_npRow;
        tmp << m_rowGrpRanks[i] << " " ;
    }
    m_output->verbose(CALL_INFO, 2, 0,"row grp [%s]\n", tmp.str().c_str() );
    tmp.str("");
    tmp.clear();
    for ( unsigned int i = 0; i < m_rowGrpRanks.size(); i++ ) {
        m_colGrpRanks[i] = myCol * m_npRow + i;
        tmp << m_colGrpRanks[i] << " " ;
    }
    m_output->verbose(CALL_INFO, 2, 0,"col grp [%s]\n", tmp.str().c_str() );

    std::vector<int> np0loc_row(m_npRow);
    std::vector<int> np1loc_row(m_npRow);
    std::vector<int> np1loc_col(npCol);
    std::vector<int> np2loc_col(npCol);

    int np0loc_;
    int np0half_;
    int np1locf_;
    int np1locb_;
    int np2loc_;
    {
        const int np0half = m_nx/2 + 1;
        np0half_ = np0half;
        const int np0loc = np0half/m_npRow;

        for ( unsigned int i = 0; i < m_npRow; i++ ) {
            np0loc_row[i] = np0loc; 
        }
        for ( unsigned int i = 0; i < np0half % m_npRow; ++i ) {
            ++np0loc_row[i];
        }
        np0loc_ = np0loc_row[ myRow ];
    }

    {
        const int np1loc = m_ny/m_npRow;
        for ( unsigned int i = 0; i < m_npRow; i++ ) {
            np1loc_row[i] = np1loc; 
        }
        for ( unsigned int i = 0; i < m_ny % m_npRow; i++ ) {
            ++np1loc_row[i];
        }
        np1locf_ = np1loc_row[myRow];
    }

    {
        const int np1loc = m_ny/npCol;
        for ( unsigned int i=0; i < npCol; i++ ) {
            np1loc_col[i] = np1loc;
        }
        for ( unsigned int i = 0; i < m_ny % npCol; i++ ) {
            ++np1loc_col[i];
        }
        np1locb_ = np1loc_row[myCol];
    }
    
    {
        const int np2loc = m_nz/npCol;
        for ( unsigned int i=0; i < npCol; i++ ) {
            np2loc_col[i] = np2loc;
        }
        for ( unsigned int i=0; i < m_nz % npCol; i++ ) {
            ++np2loc_col[i];
        }
        np2loc_ = np2loc_col[myCol];
    }

    m_output->verbose(CALL_INFO, 2, 0, "np0half=%d np0loc=%d np1locf_=%d "
            "np1locb_%d np2loc_=%d\n", 
        np0half_, np0loc_, np1locf_, np1locb_, np2loc_ );

    m_rowSendCnts.resize(npCol);
    m_rowSendDsp.resize(npCol);
    m_rowRecvCnts.resize(npCol);
    m_rowRecvDsp.resize(npCol);

    int soffset = 0, roffset = 0;
    for ( unsigned i = 0; i < npCol; i++ ) {

        int sendblk = 2* np0loc_ * np1loc_col[i] * np2loc_;
        int recvblk = 2* np0loc_ * np1locb_ * np2loc_col[i];

        m_output->verbose(CALL_INFO, 2, 0,"row, sendblk=%d soffset=%d "
                "recvblk=%d roffset=%d\n",sendblk,soffset,recvblk,roffset);
        m_rowSendCnts[i] = sendblk;
        m_rowRecvCnts[i] = recvblk;
        m_rowSendDsp[i] = soffset; 
        m_rowRecvDsp[i] = roffset;
        soffset += sendblk;
        roffset += recvblk;
    }

    m_colSendCnts_f.resize( m_npRow );
    m_colSendDsp_f.resize( m_npRow );
    m_colRecvCnts_f.resize( m_npRow );
    m_colRecvDsp_f.resize( m_npRow );

    soffset = roffset = 0;
    for ( unsigned i = 0; i < m_npRow; i++ ) {

        int sendblk = 2 * np0loc_row[i] * np1locf_ * np2loc_;
        int recvblk = 2 * np0loc_ * np1loc_row[i] * np2loc_;
        m_output->verbose(CALL_INFO, 2, 0,"col_f, sendblk=%d soffset=%d "
                "recvblk=%d roffset=%d\n",sendblk,soffset,recvblk,roffset);

        m_colSendCnts_f[i] = sendblk;
        m_colRecvCnts_f[i] = recvblk; 
        m_colSendDsp_f[i] = soffset; 
        m_colRecvDsp_f[i] = roffset;
        soffset += sendblk;
        roffset += recvblk;
    }

    m_colSendCnts_b.resize( m_npRow );
    m_colSendDsp_b.resize( m_npRow );
    m_colRecvCnts_b.resize( m_npRow );
    m_colRecvDsp_b.resize( m_npRow );

    soffset = roffset = 0;
    for ( unsigned i = 0; i < m_npRow; i++ ) {

        int sendblk = 2 * np0loc_ * np1loc_row[i] * np2loc_;
        int recvblk = 2 * np0loc_row[i] * np1locf_ * np2loc_;

        m_output->verbose(CALL_INFO, 2, 0,"col_b, sendblk=%d soffset=%d "
                "recvblk=%d roffset=%d\n",sendblk,soffset,recvblk,roffset);

        m_colSendCnts_b[i] = sendblk;
        m_colRecvCnts_b[i] = recvblk; 
        m_colSendDsp_b[i] = soffset; 
        m_colRecvDsp_b[i] = roffset;
        soffset += sendblk;
        roffset += recvblk;
    }

    int size1 = m_nx * np1locf_ * np2loc_;
    int size2 = m_ny * m_nx * np2loc_  / m_npRow;
    int size3 = m_nz * np1locb_ * m_nx / m_npRow;
    int maxsize = (size1 > size2 ? size1 : size2);
    maxsize = (maxsize > size3 ? maxsize : size3);
    m_sendBuf =   memAlloc( maxsize * COMPLEX );
    m_recvBuf =   memAlloc( maxsize * COMPLEX );
    m_output->verbose(CALL_INFO, 2, 0,"maxsize=%d\n",maxsize);
}

bool EmberFFT3DGenerator::generate( std::queue<EmberEvent*>& evQ ) 
{
    GEN_DBG( 1, "loop=%d\n", m_loopIndex );

    if (  m_loopIndex == (signed) m_iterations ) {
        if ( 0 == rank() ) {
            m_output->output("%s: fwd time %f sec\n", m_name.c_str(), 
                (double)(m_forwardStop - m_forwardStart) / 1000000000.0 );
            m_output->output("%s: bwd time %f sec\n", m_name.c_str(), 
                (double)(m_backwardStop - m_forwardStop) / 1000000000.0 );
        }
        return true;
    }

    if (  m_loopIndex < 0 ) {
        enQ_commCreate( evQ, GroupWorld, m_rowGrpRanks, &m_rowComm ); 
        enQ_commCreate( evQ, GroupWorld, m_colGrpRanks, &m_colComm ); 
        ++m_loopIndex;
        return false;
    }

    enQ_getTime( evQ, &m_forwardStart );

    enQ_compute( evQ, calcFwdFFT1() );

    enQ_alltoallv( evQ, 
                m_sendBuf, &m_colSendCnts_f[0], &m_colSendDsp_f[0], DOUBLE,
                m_recvBuf, &m_colRecvCnts_f[0], &m_colRecvDsp_f[0], DOUBLE,
                m_colComm );
  
    enQ_compute( evQ, calcFwdFFT2() );
    
    enQ_alltoallv( evQ, m_sendBuf, &m_rowSendCnts[0], &m_rowSendDsp[0], DOUBLE,
                        m_recvBuf, &m_rowRecvCnts[0], &m_rowRecvDsp[0], DOUBLE,
                        m_rowComm );
    
    enQ_compute( evQ, calcFwdFFT3() );
    
    enQ_barrier( evQ, GroupWorld ); 
    enQ_getTime( evQ, &m_forwardStop );

    enQ_compute( evQ, calcBwdFFT1() );

    enQ_alltoallv( evQ, m_sendBuf, &m_rowSendCnts[0], &m_rowSendDsp[0], DOUBLE,
                        m_recvBuf, &m_rowRecvCnts[0], &m_rowRecvDsp[0], DOUBLE,
                        m_rowComm );
    
    enQ_compute( evQ, calcBwdFFT2() );

    enQ_alltoallv( evQ, 
                m_sendBuf, &m_colSendCnts_b[0], &m_colSendDsp_b[0], DOUBLE,
                m_recvBuf, &m_colRecvCnts_b[0], &m_colRecvDsp_b[0], DOUBLE,
                m_colComm );

    enQ_compute( evQ, calcBwdFFT3() );

    enQ_barrier( evQ, GroupWorld ); 
    enQ_getTime( evQ, &m_backwardStop );

    if ( ++m_loopIndex == (signed) m_iterations ) {
        enQ_commDestroy( evQ, m_rowComm ); 
        enQ_commDestroy( evQ, m_colComm ); 
    }

    return false;
}
