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


#ifndef _H_EMBER_FFT_3D
#define _H_EMBER_FFT_3D

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberFFT3DGenerator : public EmberMessagePassingGenerator {

public:
	EmberFFT3DGenerator(SST::Component* owner, Params& params);
	~EmberFFT3DGenerator() {}
	void configure();
	bool generate( std::queue<EmberEvent*>& evQ );

private:

    struct Data {
        int np0;
        int np1;
        int np2;
        unsigned int nprow;
        unsigned int npcol;
        int np0loc;
        int np0half;
        int np1locf;
        int np1locb;
        int np2loc;
        int ntrans;
        std::vector<int> np0loc_row;
        std::vector<int> np1loc_row;
        std::vector<int> np1loc_col;
        std::vector<int> np2loc_col;
    };
    Data m_data;

    std::vector< uint32_t > m_fwdTime; 
    std::vector< uint32_t > m_bwdTime; 
    uint32_t calcFwdFFT1() { return m_fwdTime[0]; }
    uint32_t calcFwdFFT2() { return m_fwdTime[1]; }
    uint32_t calcFwdFFT3() { return m_fwdTime[2]; }
    uint32_t calcBwdFFT1() { return m_bwdTime[0]; }
    uint32_t calcBwdFFT2() { return m_bwdTime[1]; }
    uint32_t calcBwdFFT3() { return m_bwdTime[2]; }
    void initTime( Data&, std::string, float );

    inline uint32_t calcFwd1Pre( Data&, std::string, float );
    inline uint32_t calcFwd1( Data&, std::string, float );
    inline uint32_t calcFwd1Post( Data&, std::string, float );
    inline uint32_t calcFwd2Pre( Data&, std::string, float );
    inline uint32_t calcFwd2( Data&, std::string, float );
    inline uint32_t calcFwd2Post( Data&, std::string, float );
    inline uint32_t calcFwd3Pre( Data&, std::string, float );
    inline uint32_t calcFwd3( Data&, std::string, float );
    inline uint32_t calcFwd3Post( Data&, std::string, float );

    inline uint32_t calcBwd1Pre( Data&, std::string, float );
    inline uint32_t calcBwd1( Data&, std::string, float );
    inline uint32_t calcBwd1Post( Data&, std::string, float );
    inline uint32_t calcBwd2Pre( Data&, std::string, float );
    inline uint32_t calcBwd2( Data&, std::string, float );
    inline uint32_t calcBwd2Post( Data&, std::string, float );
    inline uint32_t calcBwd3Pre( Data&, std::string, float );
    inline uint32_t calcBwd3( Data&, std::string, float );
    inline uint32_t calcBwd3Post( Data&, std::string, float );

    std::string m_configFileName;

	int32_t m_loopIndex;

	uint32_t m_iterations;

    unsigned char* m_myData; 

    uint64_t    m_forwardStart;
    uint64_t    m_forwardStop;
    uint64_t    m_backwardStop;

    std::vector< uint32_t >  m_sendCnts;
    std::vector< uint32_t >  m_recvCnts;
    std::vector< uint32_t >  m_sendDsp;
    std::vector< uint32_t >  m_recvDsp;

    Communicator    m_rowComm;
    Communicator    m_colComm;
    std::vector<int>    m_rowGrpRanks;
    std::vector<int>    m_colGrpRanks;

    std::vector<int>    m_rowSendCnts;
    std::vector<int>    m_rowSendDsp;
    std::vector<int>    m_rowRecvCnts;
    std::vector<int>    m_rowRecvDsp;

    std::vector<int>    m_colSendCnts_f;
    std::vector<int>    m_colSendDsp_f;
    std::vector<int>    m_colRecvCnts_f;
    std::vector<int>    m_colRecvDsp_f;

    std::vector<int>    m_colSendCnts_b;
    std::vector<int>    m_colSendDsp_b;
    std::vector<int>    m_colRecvCnts_b;
    std::vector<int>    m_colRecvDsp_b;
    void*               m_sendBuf;
    void*               m_recvBuf;
};

uint32_t EmberFFT3DGenerator::calcFwd1Pre( Data& data,
                                        std::string str, float value )
{
    uint32_t tmp = (data.ntrans/2) * m_data.np0;
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );

    return (int) roundf( (float) tmp * value );
}

uint32_t EmberFFT3DGenerator::calcFwd1( Data& data, std::string str, float value ) {
    uint32_t tmp = (data.ntrans/2) * m_data.np0;
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );
    return (int) roundf( (float) tmp * value );
}

uint32_t EmberFFT3DGenerator::calcFwd1Post( Data& data,
                                        std::string str, float value ) {
    if ( ! str.compare( "1" ) ) {
        uint32_t tmp = (data.ntrans*2) * data.np0half;
        printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );
        return (int) roundf( (float) tmp * value );
    } else if ( ! str.compare( "2" ) ) {
        int tmp = data.np2loc * data.np1locf * data.np0half;
        printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );
        return (int) roundf( (float) tmp * value );
    } else {
        assert(0);
    }
}

uint32_t EmberFFT3DGenerator::calcFwd2Pre( Data& data,
                                        std::string str, float value ) {
    uint32_t count = 0;
    for (int i=0; i<m_data.np0loc; i++) {
        for (int k=0; k<m_data.np2loc; k++) {
            for (unsigned int tr=0; tr<m_data.nprow; tr++) {
                for (int jloc=0; jloc<m_data.np1loc_row[tr]; jloc++) {
                    ++count;
                }
            }
        }
    }
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), count, value );
    return (int) roundf( (float) count * value );
}

uint32_t EmberFFT3DGenerator::calcFwd2( Data& data, std::string str, float value ) {
    uint32_t tmp =  m_data.np1 * data.np0loc * data.np2loc;
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );
    return (int) roundf( (float) tmp * value );
}
uint32_t EmberFFT3DGenerator::calcFwd2Post( Data& data,
                                        std::string str, float value ) {
    uint32_t tmp = data.np2loc * data.np0loc * m_data.np1;
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );
    return (int) roundf( (float) tmp * value );
}

uint32_t EmberFFT3DGenerator::calcFwd3Pre( Data& data,
                                        std::string str, float value ) {
    int count = 0;
    for (int i=0; i<m_data.np0loc; i++) {
        for (int j=0; j<m_data.np1locb; j++) {
            for (unsigned int tc=0; tc<m_data.npcol; tc++) {
                for (int kloc=0; kloc < m_data.np2loc_col[tc]; kloc++) {
                    ++count;
                }
            }
        }
    }
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), count, value );
    return count * round( value );
}

uint32_t EmberFFT3DGenerator::calcFwd3( Data& data, std::string str, float value ) {
    uint32_t tmp = data.np0loc * data.np1locb * data.np2;
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );
    return (int) roundf( (float) tmp * value );
}

uint32_t EmberFFT3DGenerator::calcFwd3Post( Data& data,
                                        std::string str, float value ) {
    uint32_t tmp = data.np2 * data.np1locb * data.np0loc;
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );
    return (int) roundf( (float) tmp * value );
}


uint32_t EmberFFT3DGenerator::calcBwd1Pre( Data& data, std::string str, float value ) {
    uint32_t tmp = m_data.np2 * m_data.np1locb * m_data.np0loc;
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );
    return (int) roundf( (float) tmp * value );
}

uint32_t EmberFFT3DGenerator::calcBwd1( Data& data, std::string str, float value ) {
    uint32_t tmp = m_data.np0loc * m_data.np1locb * m_data.np2;
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );
    return (int) roundf( (float) tmp * value );
}

uint32_t EmberFFT3DGenerator::calcBwd1Post( Data& data, std::string str, float value ) {
    uint32_t tmp = m_data.np0loc * m_data.np1locb * m_data.np2;
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );
    return (int) roundf( (float) tmp * value );
}

uint32_t EmberFFT3DGenerator::calcBwd2Pre( Data& data, std::string str, float value ) {
    uint32_t tmp = 0;
  for (int k=0; k < m_data.np2loc; k++)
    for (int i=0; i < m_data.np0loc; i++) {
      for ( unsigned int tc=0; tc< m_data.npcol; tc++) {
        for (int jloc=0; jloc < m_data.np1loc_col[tc]; jloc++) {
            ++tmp;
        }
      }
    }
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );
    return (int) roundf( (float) tmp * value );
}

uint32_t EmberFFT3DGenerator::calcBwd2( Data& data, std::string str, float value ) {
    uint32_t tmp = m_data.np0loc * m_data.np2loc * m_data.np1;
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );
    return (int) roundf( (float) tmp * value );
}

uint32_t EmberFFT3DGenerator::calcBwd2Post( Data& data, std::string str, float value ) {
    uint32_t tmp = m_data.np0loc*m_data.np2loc*m_data.np1;
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );
    return (int) roundf( (float) tmp * value );
}

uint32_t EmberFFT3DGenerator::calcBwd3Pre( Data& data, std::string str, float value ) {
    uint32_t tmp = 0;
  for (int k=0; k < m_data.np2loc; k++)
    for (int j=0; j < m_data.np1locf; j++)
    {
      for ( unsigned int tr=0; tr < m_data.nprow; tr++)
      {
        for ( int iloc=0; iloc < m_data.np0loc_row[tr]; iloc++)
        {
            ++tmp;
        }
      }
    }
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );
    return (int) roundf( (float) tmp * value );
}

uint32_t EmberFFT3DGenerator::calcBwd3( Data& data, std::string str, float value ) {
    uint32_t tmp = m_data.np2loc*m_data.np1locf*m_data.np0;;
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );
    return (int) roundf( (float) tmp * value );
}

uint32_t EmberFFT3DGenerator::calcBwd3Post( Data& data, std::string str, float value ) {
    uint32_t tmp = m_data.np0*m_data.np1locf*m_data.np2loc ;
    printf("%s() %s cnt=%d val=%f\n",__func__,str.c_str(), tmp, value );
    return (int) roundf( (float) tmp * value );
}


}
}

#endif
