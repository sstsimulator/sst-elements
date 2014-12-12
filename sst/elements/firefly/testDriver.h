// Copyright 2013-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_TESTDRIVER_H
#define COMPONENTS_FIREFLY_TESTDRIVER_H

#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/output.h>

#include <iostream>
#include <fstream>

#include "sst/elements/hermes/msgapi.h"

using namespace Hermes;

namespace SST {
namespace Firefly {

class TestDriver : public SST::Component {

  public:
    TestDriver(SST::ComponentId_t id, SST::Params &params);
    ~TestDriver();
    void setup(void); 
    void init(unsigned int phase);
    void printStatus( Output& );

  private:
    typedef Arg_Functor<TestDriver, int, bool > DerivedFunctor;

    void handle_event(SST::Event*);
    bool funcDone(int retval);
    void allgatherEnter();
    void allgatherReturn();
    void allgathervEnter();
    void allgathervReturn();
    void gathervEnter();
    void gathervReturn();
    void gatherEnter();
    void gatherReturn();
    void alltoallvEnter();
    void alltoallvReturn();
    void alltoallEnter();
    void alltoallReturn();
    void recvReturn();
    void waitRecvReturn();
    void waitanyRecvReturn();
    void waitallReturn();
    void waitanyReturn();

    bool                        m_sharedTraceFile;
    std::string                 m_traceFileName;
    std::ifstream               m_traceFile;
    DerivedFunctor              m_functor;
    MP::Interface*   			m_hermes;
    SST::Link*                  m_selfLink;
    std::string                 m_funcName;

    std::deque<std::string>     m_fileBuffer;

    enum { SendReq, RecvReq };

    int                     my_index;
    MP::MessageRequest  my_req[2];
    MP::MessageResponse my_resp[2];
    MP::MessageResponse* my_respPtr[2];
    MP::RankID          my_rank;
    int                     my_size;
    MP::RankID          m_root;
    int                     m_collectiveIn;
    int                     m_collectiveOut;
    Output                  m_dbg;

    size_t                  m_bufLen;
    std::vector<unsigned char> m_recvBuf;
    std::vector<unsigned char> m_sendBuf;

    std::vector<unsigned int> m_intRecvBuf;
    std::vector<unsigned int> m_intSendBuf;

    std::vector<unsigned int> m_gatherSendBuf;
    std::vector<unsigned int> m_gatherRecvBuf;
    std::vector<unsigned int> m_gathervSendBuf;
    std::vector<unsigned int> m_gathervRecvBuf;

    std::vector<unsigned int> m_allgatherSendBuf;
    std::vector<unsigned int> m_allgatherRecvBuf;

    std::vector<unsigned int> m_recvdispls;
    std::vector<unsigned int> m_recvcnts;
    std::vector<unsigned int> m_senddispls;
    std::vector<unsigned int> m_sendcnts;
};

} // namesapce Firefly 
} // namespace SST

#endif
