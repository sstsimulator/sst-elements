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


#ifndef _H_EMBER_FFT_3D
#define _H_EMBER_FFT_3D

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberFFT3DGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberFFT3DGenerator,
        "ember",
        "FFT3DMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Models an FFT",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "arg.nx",         "Sets the size of a block in X", "8" },
        { "arg.ny",         "Sets the size of a block in Y", "8" },
        { "arg.nz",         "Sets the size of a block in Z", "8" },
        { "arg.npRow",      "Sets the number of rows in the PE decomposition", "0" },
        { "arg.iterations", "Sets the number of FFT iterations to perform",   "1"},
        { "arg.nsPerElement",  "", "" },
        { "arg.fwd_fft1",  "", "" },
        { "arg.fwd_fft2",  "", "" },
        { "arg.fwd_fft3",  "", "" },
        { "arg.bwd_fft1",  "", "" },
        { "arg.bwd_fft2",  "", "" },
        { "arg.bwd_fft3",  "", "" },
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "time-Init", "Time spent in Init event",          "ns",  0},
        { "time-Finalize", "Time spent in Finalize event",  "ns", 0},
        { "time-Rank", "Time spent in Rank event",          "ns", 0},
        { "time-Size", "Time spent in Size event",          "ns", 0},
        { "time-Send", "Time spent in Recv event",          "ns", 0},
        { "time-Recv", "Time spent in Recv event",          "ns", 0},
        { "time-Irecv", "Time spent in Irecv event",        "ns", 0},
        { "time-Isend", "Time spent in Isend event",        "ns", 0},
        { "time-Wait", "Time spent in Wait event",          "ns", 0},
        { "time-Waitall", "Time spent in Waitall event",    "ns", 0},
        { "time-Waitany", "Time spent in Waitany event",    "ns", 0},
        { "time-Compute", "Time spent in Compute event",    "ns", 0},
        { "time-Barrier", "Time spent in Barrier event",    "ns", 0},
        { "time-Alltoallv", "Time spent in Alltoallv event", "ns", 0},
        { "time-Alltoall", "Time spent in Alltoall event",  "ns", 0},
        { "time-Allreduce", "Time spent in Allreduce event", "ns", 0},
        { "time-Reduce", "Time spent in Reduce event",      "ns", 0},
        { "time-Bcast", "Time spent in Bcast event",        "ns", 0},
        { "time-Gettime", "Time spent in Gettime event",    "ns", 0},
        { "time-Commsplit", "Time spent in Commsplit event", "ns", 0},
        { "time-Commcreate", "Time spent in Commcreate event", "ns", 0},
    )

public:
	EmberFFT3DGenerator(SST::ComponentId_t, Params& params);
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
