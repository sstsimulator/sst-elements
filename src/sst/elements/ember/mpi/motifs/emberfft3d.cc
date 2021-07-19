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

#include <sst_config.h>

#include "emberfft3d.h"

#include <iostream>
#include <fstream>

using namespace SST::Ember;

EmberFFT3DGenerator::EmberFFT3DGenerator(SST::ComponentId_t id, Params& params) :
	EmberMessagePassingGenerator(id, params, "FFT3D"),
    m_fwdTime(3,0),
    m_bwdTime(3,0),
	m_loopIndex(-1),
	m_forwardStart(0),
	m_forwardStop(0),
	m_backwardStop(0),
	m_forwardTotal(0),
	m_backwardTotal(0),
    m_transCostPer(6)
{
	m_data.np0 = params.find<uint32_t>("arg.nx", 100);
	m_data.np1  = params.find<uint32_t>("arg.ny", 100);
	m_data.np2  = params.find<uint32_t>("arg.nz", 100);

    assert( m_data.np0 == m_data.np1 );
    assert( m_data.np1 == m_data.np2 );

    m_data.nprow = params.find<uint32_t>("arg.npRow", 0);
    assert( 0 < m_data.nprow );

	m_iterations = params.find<uint32_t>("arg.iterations", 1);

    m_nsPerElement = params.find<float>("arg.nsPerElement",1);

    m_transCostPer[0] = params.find<float>("arg.fwd_fft1",1);
    m_transCostPer[1] = params.find<float>("arg.fwd_fft2",1);
    m_transCostPer[2] = params.find<float>("arg.fwd_fft3",1);
    m_transCostPer[3] = params.find<float>("arg.bwd_fft1",1);
    m_transCostPer[4] = params.find<float>("arg.bwd_fft2",1);
    m_transCostPer[5] = params.find<float>("arg.bwd_fft3",1);

	configure();
}

void EmberFFT3DGenerator::configure()
{
    m_data.npcol = size() / m_data.nprow;

    assert( 0 == (size() % m_data.nprow) );
#if 0
    assert(  (m_data.np0 % m_data.npcol) == 0 );
    assert(  (m_data.np1 % m_data.npcol) == 0 );
    assert(  (m_data.np2 % m_data.nprow) == 0 );
#endif

    unsigned myRow = rank() % m_data.nprow;
    unsigned myCol = rank() / m_data.nprow;
    verbose(CALL_INFO, 2, 0, "%d: nx=%d ny=%d nx=%d nprow=%d "
        "npcol=%d myRow=%d myCol=%d\n",
            rank(), m_data.np0, m_data.np1, m_data.np2,
                m_data.nprow, m_data.npcol, myRow, myCol );

    initTimes( size(), m_data.np0, m_data.np1, m_data.np2,
                    m_nsPerElement, m_transCostPer );

    m_rowGrpRanks.resize( m_data.npcol );
    m_colGrpRanks.resize( m_data.nprow );

    std::ostringstream tmp;

    for ( unsigned int i = 0; i < m_rowGrpRanks.size(); i++ ) {
        m_rowGrpRanks[i] = myRow + i * m_data.nprow;
        tmp << m_rowGrpRanks[i] << " " ;
    }
    verbose(CALL_INFO, 2, 0,"row grp [%s]\n", tmp.str().c_str() );
    tmp.str("");
    tmp.clear();
    for ( unsigned int i = 0; i < m_colGrpRanks.size(); i++ ) {
        m_colGrpRanks[i] = myCol * m_data.nprow + i;
        tmp << m_colGrpRanks[i] << " " ;
    }
    verbose(CALL_INFO, 2, 0,"col grp [%s]\n", tmp.str().c_str() );

    m_data.np0loc_row.resize(m_data.nprow);
    m_data.np1loc_row.resize(m_data.nprow);
    m_data.np1loc_col.resize(m_data.npcol);
    m_data.np2loc_col.resize(m_data.npcol);

    {
        const int np0half = m_data.np0/2 + 1;
        m_data.np0half = np0half;
        const int np0loc = np0half/m_data.nprow;

        for ( unsigned int i = 0; i < m_data.nprow; i++ ) {
            m_data.np0loc_row[i] = np0loc;
        }
        for ( unsigned int i = 0; i < np0half % m_data.nprow; ++i ) {
            ++m_data.np0loc_row[i];
        }
        m_data.np0loc = m_data.np0loc_row[ myRow ];
    }

    {
        const int np1loc = m_data.np1/m_data.nprow;
        for ( unsigned int i = 0; i < m_data.nprow; i++ ) {
            m_data.np1loc_row[i] = np1loc;
        }
        for ( unsigned int i = 0; i < m_data.np1 % m_data.nprow; i++ ) {
            ++m_data.np1loc_row[i];
        }
        m_data.np1locf = m_data.np1loc_row[myRow];
    }

    {
        const int np1loc = m_data.np1/m_data.npcol;
        for ( unsigned int i=0; i < m_data.npcol; i++ ) {
            m_data.np1loc_col[i] = np1loc;
        }
        for ( unsigned int i = 0; i < m_data.np1 % m_data.npcol; i++ ) {
            ++m_data.np1loc_col[i];
        }
        m_data.np1locb = m_data.np1loc_col[myCol];
    }

    {
        const int np2loc = m_data.np2/m_data.npcol;
        for ( unsigned int i=0; i < m_data.npcol; i++ ) {
            m_data.np2loc_col[i] = np2loc;
        }
        for ( unsigned int i=0; i < m_data.np2 % m_data.npcol; i++ ) {
            ++m_data.np2loc_col[i];
        }
        m_data.np2loc = m_data.np2loc_col[myCol];
    }
    m_data.ntrans = m_data.np1locf * m_data.np2loc;

    verbose(CALL_INFO, 2, 0, "np0half=%d np0loc=%d np1locf_=%d "
            "np1locb_%d np2loc_=%d\n",
        m_data.np0half, m_data.np0loc, m_data.np1locf, m_data.np1locb, m_data.np2loc );

    m_rowSendCnts.resize(m_data.npcol);
    m_rowSendDsp.resize(m_data.npcol);
    m_rowRecvCnts.resize(m_data.npcol);
    m_rowRecvDsp.resize(m_data.npcol);

    int soffset = 0, roffset = 0;
    for ( unsigned i = 0; i < m_data.npcol; i++ ) {

        int sendblk = 2*m_data.np0loc * m_data.np1loc_col[i] *m_data.np2loc;
        int recvblk = 2*m_data.np0loc *m_data.np1locb * m_data.np2loc_col[i];

        verbose(CALL_INFO, 2, 0,"row, sendblk=%d soffset=%d "
                "recvblk=%d roffset=%d\n",sendblk,soffset,recvblk,roffset);
        m_rowSendCnts[i] = sendblk;
        m_rowRecvCnts[i] = recvblk;
        m_rowSendDsp[i] = soffset;
        m_rowRecvDsp[i] = roffset;
        soffset += sendblk;
        roffset += recvblk;
    }

    m_colSendCnts_f.resize( m_data.nprow );
    m_colSendDsp_f.resize( m_data.nprow );
    m_colRecvCnts_f.resize( m_data.nprow );
    m_colRecvDsp_f.resize( m_data.nprow );

    soffset = roffset = 0;
    for ( unsigned i = 0; i < m_data.nprow; i++ ) {

        int sendblk = 2 * m_data.np0loc_row[i] *m_data.np1locf *m_data.np2loc;
        int recvblk = 2 *m_data.np0loc * m_data.np1loc_row[i] *m_data.np2loc;
        verbose(CALL_INFO, 2, 0,"col_f, sendblk=%d soffset=%d "
                "recvblk=%d roffset=%d\n",sendblk,soffset,recvblk,roffset);

        m_colSendCnts_f[i] = sendblk;
        m_colRecvCnts_f[i] = recvblk;
        m_colSendDsp_f[i] = soffset;
        m_colRecvDsp_f[i] = roffset;
        soffset += sendblk;
        roffset += recvblk;
    }

    m_colSendCnts_b.resize( m_data.nprow );
    m_colSendDsp_b.resize( m_data.nprow );
    m_colRecvCnts_b.resize( m_data.nprow );
    m_colRecvDsp_b.resize( m_data.nprow );

    soffset = roffset = 0;
    for ( unsigned i = 0; i < m_data.nprow; i++ ) {

        int sendblk = 2 *m_data.np0loc * m_data.np1loc_row[i] *m_data.np2loc;
        int recvblk = 2 * m_data.np0loc_row[i] *m_data.np1locf *m_data.np2loc;

        verbose(CALL_INFO, 2, 0,"col_b, sendblk=%d soffset=%d "
                "recvblk=%d roffset=%d\n",sendblk,soffset,recvblk,roffset);

        m_colSendCnts_b[i] = sendblk;
        m_colRecvCnts_b[i] = recvblk;
        m_colSendDsp_b[i] = soffset;
        m_colRecvDsp_b[i] = roffset;
        soffset += sendblk;
        roffset += recvblk;
    }

    int size1 = m_data.np0 *m_data.np1locf *m_data.np2loc;
    int size2 = m_data.np1 * m_data.np0 *m_data.np2loc  / m_data.nprow;
    int size3 = m_data.np2 *m_data.np1locb * m_data.np0 / m_data.nprow;

    int maxsize = (size1 > size2 ? size1 : size2);
    maxsize = (maxsize > size3 ? maxsize : size3);
    m_sendBuf =   memAlloc( maxsize * COMPLEX );
    m_recvBuf =   memAlloc( maxsize * COMPLEX );
    verbose(CALL_INFO, 2, 0,"maxsize=%d\n",maxsize);

    verbose(CALL_INFO, 2, 0,"np0=%d np1=%d np2=%d nprow=%d npcol=%d\n",
                m_data.np0, m_data.np1, m_data.np2, m_data.nprow, m_data.npcol );
    verbose(CALL_INFO, 2, 0,"np0half=%d np1loc=%d np1locf=%d no1locb=%d np2loc=%d\n",
        m_data.np0half, m_data.np0loc, m_data.np1locf, m_data.np1locb, m_data.np2loc );

#if 0
    verbose(CALL_INFO, 2, 0,"forward nx=%d dist=%d time=%u\n",
                        nxt_f, m_data.np0, m_fft1_f );
    verbose(CALL_INFO, 2, 0,"forward ny=%d dist=%d time=%u\n",
                        nyt, m_data.np1, m_fft2_f );
    verbose(CALL_INFO, 2, 0,"forward nz=%d dist=%d time=%u\n",
                        nzt, m_data.np2, m_fft3_f );
    verbose(CALL_INFO, 2, 0,"forward nz=%d dist=%d time=%u\n",
                        nzt, m_data.np2, m_fft1_b );
    verbose(CALL_INFO, 2, 0,"forward ny=%d dist=%d time=%u\n",
                        nyt, m_data.np1, m_fft2_b );
    verbose(CALL_INFO, 2, 0,"forward nz=%d dist=%d time=%u\n",
                        nxt_b, m_data.np0, m_fft3_b );
#endif

}

void EmberFFT3DGenerator::initTimes( int numPe, int x, int y, int z, float nsPerElement,
                std::vector<float>& transCostPer )
{
#if 0
    printf("%f\n",nsPerElement);
    printf("%f\n",transCostPer[0]);
    printf("%f\n",transCostPer[1]);
    printf("%f\n",transCostPer[2]);
    printf("%f\n",transCostPer[3]);
    printf("%f\n",transCostPer[4]);
    printf("%f\n",transCostPer[5]);
#endif

    double cost = nsPerElement * x * ((y * z)/numPe);
	if ( 0 == rank() ) {
	   	output("%s: nsPerElement=%.5f %.2f %.2f %.2f"
			" %.2f %.2f %.2f\n", getMotifName().c_str(), nsPerElement,
			transCostPer[0], transCostPer[1], transCostPer[2],
			transCostPer[3], transCostPer[4], transCostPer[5]);
	}
	assert( cost > 0.0 );
    m_fwdTime[0] =  m_fwdTime[1] = m_fwdTime[2] = cost;
    m_bwdTime[0] =  m_bwdTime[1] = m_bwdTime[2] = cost;
    m_bwdTime[2] *= 2;

    m_fwdTime[0] *= transCostPer[0];
    m_fwdTime[1] *= transCostPer[1];
    m_fwdTime[2] *= transCostPer[2];

    m_bwdTime[0] *= transCostPer[3];
    m_bwdTime[1] *= transCostPer[4];
    m_bwdTime[2] *= transCostPer[5];
}

bool EmberFFT3DGenerator::generate( std::queue<EmberEvent*>& evQ )
{
    verbose(CALL_INFO, 1, 0, "loop=%d\n", m_loopIndex );

    m_forwardTotal += (m_forwardStop - m_forwardStart);
    m_backwardTotal += (m_backwardStop - m_forwardStop);

    if (  m_loopIndex == (signed) m_iterations ) {
        if ( 0 == rank() ) {
            output("%s: nRanks=%d fwd time %f sec\n", getMotifName().c_str(), size(),
                ((double) m_forwardTotal / 1000000000.0) / m_iterations );
            output("%s: rRanks=%d bwd time %f sec\n", getMotifName().c_str(), size(),
                ((double) m_backwardTotal / 1000000000.0) / m_iterations );
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

    enQ_compute( evQ, (uint64_t) ((double) calcFwdFFT1() ) );

    enQ_alltoallv( evQ,
                m_sendBuf, &m_colSendCnts_f[0], &m_colSendDsp_f[0], DOUBLE,
                m_recvBuf, &m_colRecvCnts_f[0], &m_colRecvDsp_f[0], DOUBLE,
                m_colComm );

    enQ_compute( evQ, (uint64_t) ((double) calcFwdFFT2() ) );

    enQ_alltoallv( evQ, m_sendBuf, &m_rowSendCnts[0], &m_rowSendDsp[0], DOUBLE,
                        m_recvBuf, &m_rowRecvCnts[0], &m_rowRecvDsp[0], DOUBLE,
                        m_rowComm );

    enQ_compute( evQ, (uint64_t) ((double) calcFwdFFT3()  ) );

    enQ_barrier( evQ, GroupWorld );
    enQ_getTime( evQ, &m_forwardStop );

    enQ_compute( evQ, (uint64_t) ((double) calcBwdFFT1() ) );

    enQ_alltoallv( evQ, m_sendBuf, &m_rowSendCnts[0], &m_rowSendDsp[0], DOUBLE,
                        m_recvBuf, &m_rowRecvCnts[0], &m_rowRecvDsp[0], DOUBLE,
                        m_rowComm );

    enQ_compute( evQ, (uint64_t) ((double) calcBwdFFT2() ) );

    enQ_alltoallv( evQ,
                m_sendBuf, &m_colSendCnts_b[0], &m_colSendDsp_b[0], DOUBLE,
                m_recvBuf, &m_colRecvCnts_b[0], &m_colRecvDsp_b[0], DOUBLE,
                m_colComm );

    enQ_compute( evQ, (uint64_t) ((double) calcBwdFFT3() ) );

    enQ_barrier( evQ, GroupWorld );
    enQ_getTime( evQ, &m_backwardStop );

    if ( ++m_loopIndex == (signed) m_iterations ) {
        enQ_commDestroy( evQ, m_rowComm );
        enQ_commDestroy( evQ, m_colComm );
    }

    return false;
}
