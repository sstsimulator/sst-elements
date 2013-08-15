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

#include <sst/core/output.h>

#include <vector>
#include "ioapi.h"

namespace SST {
namespace Firefly {

class ProtocolAPI 
{
  public:
    struct Request {
      public:
        Request() : delay(0) {} 
        virtual ~Request() {}

        std::vector<IO::IoVec>  ioVec;
        int                     delay;
        IO::NodeId              nodeId; 
    }; 

    ProtocolAPI(int verboseLevel, Output::output_location_t loc) {
        m_dbg.init("@t:ProtocolAPI::@p():@l ", verboseLevel, 0, loc );
    }
    virtual ~ProtocolAPI() {}
    virtual Request* getSendReq( ) = 0;
    virtual Request* getRecvReq( IO::NodeId src ) = 0;
    virtual Request* sendIODone( Request* ) = 0;
    virtual Request* recvIODone( Request* ) = 0;
    virtual Request* delayDone( Request* ) = 0;
    virtual void setup() = 0;

    Output m_dbg;
};
}
}
#endif
