// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_TESTDRIVER_H
#define COMPONENTS_FIREFLY_TESTDRIVER_H

#include <sst/core/sst_types.h>
#include <sst/core/component.h>

#include <iostream>
#include <fstream>

#include "sst/elements/hermes/msgapi.h"

namespace SST {
namespace Firefly {

class TestDriver : public SST::Component {

  public:
    TestDriver(SST::ComponentId_t id, SST::Component::Params_t &params);
    ~TestDriver();
    void setup(void); 
    void init(unsigned int phase);

  private:
    typedef Arg_Functor<TestDriver, int> DerivedFunctor;

    void handle_event(SST::Event*);
    void funcDone(int retval);

    std::string                 m_traceFileName;
    std::ifstream               m_traceFile;
    DerivedFunctor              m_functor;
    Hermes::MessageInterface*   m_hermes;
    SST::Link*                  m_selfLink;

    Hermes::MessageRequest  my_req;
    Hermes::MessageResponse my_resp;
    int                     my_rank;
    int                     my_size;

    std::vector<unsigned char> m_recvBuf;
    std::vector<unsigned char> m_sendBuf;

};

} // namesapce Firefly 
} // namespace SST

#endif
