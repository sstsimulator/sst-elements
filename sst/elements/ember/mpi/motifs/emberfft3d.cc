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

#include <iostream>
#include <fstream>

using namespace SST::Ember;

EmberFFT3DGenerator::EmberFFT3DGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params), 
    m_fwdTime(3,0),
    m_bwdTime(3,0),
	m_loopIndex(-1),
	m_forwardStart(0),
	m_forwardStop(0),
	m_backwardStop(0),
	m_forwardTotal(0),
	m_backwardTotal(0)
{
	m_name = "FFT3D";

	m_data.np0 = (uint32_t) params.find_integer("arg.nx", 100);
	m_data.np1  = (uint32_t) params.find_integer("arg.ny", 100);
	m_data.np2  = (uint32_t) params.find_integer("arg.nz", 100);

    m_data.nprow = (uint32_t) params.find_integer("arg.npRow", 0);
    assert( 0 < m_data.nprow );

    m_configFileName = params.find_string("arg.configFile");
    assert( ! m_configFileName.empty() );

	m_iterations = (uint32_t) params.find_integer("arg.iterations", 1);
}


void EmberFFT3DGenerator::configure()
{
    m_data.npcol = size() / m_data.nprow; 

    assert(  (m_data.np0 % m_data.npcol) == 0 ); 
    assert(  (m_data.np1 % m_data.npcol) == 0 ); 
    assert(  (m_data.np2 % m_data.nprow) == 0 ); 

    unsigned myRow = rank() % m_data.nprow; 
    unsigned myCol = rank() / m_data.nprow; 
    m_output->verbose(CALL_INFO, 2, 0, "%d: nx=%d ny=%d nx=%d nprow=%d "
        "npcol=%d myRow=%d myCol=%d\n", 
            rank(), m_data.np0, m_data.np1, m_data.np2, 
                m_data.nprow, m_data.npcol, myRow, myCol );

    m_rowGrpRanks.resize( m_data.npcol );
    m_colGrpRanks.resize( m_data.nprow );

    std::ostringstream tmp; 

    for ( unsigned int i = 0; i < m_rowGrpRanks.size(); i++ ) {
        m_rowGrpRanks[i] = myRow + i * m_data.nprow;
        tmp << m_rowGrpRanks[i] << " " ;
    }
    m_output->verbose(CALL_INFO, 2, 0,"row grp [%s]\n", tmp.str().c_str() );
    tmp.str("");
    tmp.clear();
    for ( unsigned int i = 0; i < m_colGrpRanks.size(); i++ ) {
        m_colGrpRanks[i] = myCol * m_data.nprow + i;
        tmp << m_colGrpRanks[i] << " " ;
    }
    m_output->verbose(CALL_INFO, 2, 0,"col grp [%s]\n", tmp.str().c_str() );

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

    m_output->verbose(CALL_INFO, 2, 0, "np0half=%d np0loc=%d np1locf_=%d "
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

        m_output->verbose(CALL_INFO, 2, 0,"row, sendblk=%d soffset=%d "
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
        m_output->verbose(CALL_INFO, 2, 0,"col_f, sendblk=%d soffset=%d "
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

        m_output->verbose(CALL_INFO, 2, 0,"col_b, sendblk=%d soffset=%d "
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
    m_output->verbose(CALL_INFO, 2, 0,"maxsize=%d\n",maxsize);

    m_output->verbose(CALL_INFO, 2, 0,"%s\n",m_configFileName.c_str());
    m_output->verbose(CALL_INFO, 2, 0,"np0=%d np1=%d np2=%d nprow=%d npcol=%d\n",
                m_data.np0, m_data.np1, m_data.np2, m_data.nprow, m_data.npcol );
    m_output->verbose(CALL_INFO, 2, 0,"np0half=%d np1loc=%d np1locf=%d no1locb=%d np2loc=%d\n",
        m_data.np0half, m_data.np0loc, m_data.np1locf, m_data.np1locb, m_data.np2loc );

#if 0
    m_output->verbose(CALL_INFO, 2, 0,"forward nx=%d dist=%d time=%u\n", 
                        nxt_f, m_data.np0, m_fft1_f );
    m_output->verbose(CALL_INFO, 2, 0,"forward ny=%d dist=%d time=%u\n",
                        nyt, m_data.np1, m_fft2_f ); 
    m_output->verbose(CALL_INFO, 2, 0,"forward nz=%d dist=%d time=%u\n", 
                        nzt, m_data.np2, m_fft3_f ); 
    m_output->verbose(CALL_INFO, 2, 0,"forward nz=%d dist=%d time=%u\n", 
                        nzt, m_data.np2, m_fft1_b );
    m_output->verbose(CALL_INFO, 2, 0,"forward ny=%d dist=%d time=%u\n", 
                        nyt, m_data.np1, m_fft2_b );
    m_output->verbose(CALL_INFO, 2, 0,"forward nz=%d dist=%d time=%u\n", 
                        nxt_b, m_data.np0, m_fft3_b );
#endif

    std::ifstream file;
    file.open(m_configFileName.c_str()); 
    assert( file.is_open() );
    std::string line;
    while (  getline( file, line) ) {
        std::string::iterator end_pos = std::remove(
                                    line.begin(), line.end(),' ');

        line.erase(end_pos, line.end());
        if ( line.empty() || ! line.compare(0,1,"#") ) {
            continue;
        }
        std::size_t pos = line.find(','); 
        std::string name = line.substr( 0, pos );
        float value;
        istringstream buffer( line.substr( pos + 1 ) );
        buffer >> value;
        initTime(m_data, name, value );
    }
    file.close();
}

void EmberFFT3DGenerator::initTime( Data& data, std::string name, float value )
{
    std::string xx = name.substr(0,7); 
    int yy; 
    istringstream buffer( name.substr( 7, 7 ) ); 
    buffer >> yy;
    std::string zz; 
    --yy;

    if ( name.size() > 8 ) {
        zz = name.substr( 9 );
    }        

    if ( ! xx.compare("fwd_fft") ) {
        switch ( yy ) {
          case 0:
            if ( ! zz.compare( 0,3, "pre" )  ) {
                m_fwdTime[yy] += calcFwd1Pre( data, zz.substr(3), value );
            } else if ( ! zz.compare( 0,4, "post" )  ) {
                m_fwdTime[yy] += calcFwd1Post( data, zz.substr(4), value );
            } else {
                m_fwdTime[yy] += calcFwd1( data, zz, value );
            } 
            break;
          case 1:
            if ( ! zz.compare( 0,3, "pre" )  ) {
                m_fwdTime[yy] += calcFwd2Pre( data, zz.substr(3), value );
            } else if ( ! zz.compare( 0,4, "post" )  ) {
                m_fwdTime[yy] += calcFwd2Post( data, zz.substr(4), value );
            } else {
                m_fwdTime[yy] += calcFwd2( data, zz, value );
            }
            break;
          case 2:
            if ( ! zz.compare( 0,3, "pre" )  ) {
                m_fwdTime[yy] += calcFwd3Pre( data, zz.substr(3), value );
            } else if ( ! zz.compare( 0,4, "post" )  ) {
                m_fwdTime[yy] += calcFwd3Post( data, zz.substr(4), value );
            } else {
                m_fwdTime[yy] += calcFwd3( data, zz, value );
            }
            break;
          break;
        }
    } else {
        switch ( yy ) {
          case 0:
            if ( ! zz.compare( 0,3, "pre" )  ) {
                m_bwdTime[yy] += calcBwd1Pre( data, zz.substr(3), value );
            } else if ( ! zz.compare( 0,4, "post" )  ) {
                m_bwdTime[yy] += calcBwd1Post( data, zz.substr(4), value );
            } else {
                m_bwdTime[yy] += calcBwd1( data, zz, value );
            } 
            break;
          case 1:
            if ( ! zz.compare( 0,3, "pre" )  ) {
                m_bwdTime[yy] += calcBwd2Pre( data, zz.substr(3), value );
            } else if ( ! zz.compare( 0,4, "post" )  ) {
                m_bwdTime[yy] += calcBwd2Post( data, zz.substr(4), value );
            } else {
                m_bwdTime[yy] += calcBwd2( data, zz, value );
            }
            break;
          case 2:
            if ( ! zz.compare( 0,3, "pre" )  ) {
                m_bwdTime[yy] += calcBwd3Pre( data, zz.substr(3), value );
            } else if ( ! zz.compare( 0,4, "post" )  ) {
                m_bwdTime[yy] += calcBwd3Post( data, zz.substr(4), value );
            } else {
                m_bwdTime[yy] += calcBwd3( data, zz, value );
            }
            break;
          break;
        }
    }    
}

bool EmberFFT3DGenerator::generate( std::queue<EmberEvent*>& evQ ) 
{
    GEN_DBG( 1, "loop=%d\n", m_loopIndex );

    m_forwardTotal += (m_forwardStop - m_forwardStart);
    m_backwardTotal += (m_backwardStop - m_forwardStop);

    if (  m_loopIndex == (signed) m_iterations ) {
        if ( 0 == rank() ) {
            m_output->output("%s: fwd time %f sec\n", m_name.c_str(), 
                ((double) m_forwardTotal / 1000000000.0) / m_iterations );
            m_output->output("%s: bwd time %f sec\n", m_name.c_str(), 
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
