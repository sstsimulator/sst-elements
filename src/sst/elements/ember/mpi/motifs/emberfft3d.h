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
    } m_data;

    std::vector< uint64_t > m_fwdTime; 
    std::vector< uint64_t > m_bwdTime; 
    uint64_t calcFwdFFT1() { return m_fwdTime[0]; }
    uint64_t calcFwdFFT2() { return m_fwdTime[1]; }
    uint64_t calcFwdFFT3() { return m_fwdTime[2]; }
    uint64_t calcBwdFFT1() { return m_bwdTime[0]; }
    uint64_t calcBwdFFT2() { return m_bwdTime[1]; }
    uint64_t calcBwdFFT3() { return m_bwdTime[2]; }

    void initTimes( int numPe, int x, int y, int z, float nsPerElement,
                std::vector<float>& transCostPer );

	int32_t m_loopIndex;

	uint32_t m_iterations;

    uint64_t    m_forwardStart;
    uint64_t    m_forwardStop;
    uint64_t    m_backwardStop;
    uint64_t    m_forwardTotal;
    uint64_t    m_backwardTotal;

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
    float              m_nsPerElement;
    std::vector<float> m_transCostPer;
};

}
}

#endif
