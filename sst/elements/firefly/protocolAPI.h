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

#ifndef COMPONENTS_FIREFLY_PROTOCOLAPI_H
#define COMPONENTS_FIREFLY_PROTOCOLAPI_H

#include <sst/core/module.h>
#include <sst/core/output.h>

#include <vector>
#include "ioapi.h"
#include "ioVec.h"

namespace SST {
namespace Firefly {

class Info;

class ProtocolAPI : public SST::Module 
{
  public:

    class OutBase {
      public:
        virtual ~OutBase() {}
        virtual bool sendv(int dest, std::vector<IoVec>&, IO::Entry::Functor*) = 0;
        virtual bool recvv(int src, std::vector<IoVec>&, IO::Entry::Functor*) = 0;
    };

    virtual ~ProtocolAPI() {}
    virtual void printStatus( Output& ) {}
    virtual void setup() {};
    virtual void init( OutBase*, Info*, Link* ) = 0;  

    virtual IO::NodeId canSend() = 0;
    virtual void startSendStream( IO::NodeId ) = 0;
    virtual void startRecvStream( IO::NodeId ) = 0;

    virtual std::string name() = 0;
    virtual void setRetLink(SST::Link* link) { assert(0); } 

    Output m_dbg;
};

}
}
#endif
