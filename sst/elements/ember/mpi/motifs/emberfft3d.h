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
    uint32_t calcFwdFFT1() {
        return 10000;
    }
    uint32_t calcFwdFFT2() {
        return 10000;
    }
    uint32_t calcFwdFFT3() {
        return 1000*20;
    }

    uint32_t calcBwdFFT1() {
        return 1000;
    }
    uint32_t calcBwdFFT2() {
        return 1000;
    }
    uint32_t calcBwdFFT3() {
        return 1000;
    }

	int32_t m_loopIndex;

	uint32_t m_iterations;

	uint32_t m_nx;
	uint32_t m_ny;
	uint32_t m_nz;

	uint32_t m_npRow;

	uint32_t m_nsCompute;
	uint32_t m_nsCopyTime;

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

}
}

#endif
