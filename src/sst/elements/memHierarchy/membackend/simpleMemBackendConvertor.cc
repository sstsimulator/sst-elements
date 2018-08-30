// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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
#include <sst/core/params.h>
#include "sst/elements/memHierarchy/util.h"
#include "membackend/simpleMemBackendConvertor.h"
#include "membackend/memBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

SimpleMemBackendConvertor::SimpleMemBackendConvertor(Component *comp, Params &params) :
        MemBackendConvertor(comp,params) 
{
    using std::placeholders::_1;
    static_cast<SimpleMemBackend*>(m_backend)->setResponseHandler( std::bind( &SimpleMemBackendConvertor::handleMemResponse, this, _1 ) );
}

bool SimpleMemBackendConvertor::issue( BaseReq* req ) {
    if (req->isMemEv()) {
        MemReq * mreq = static_cast<MemReq*>(req);
        return static_cast<SimpleMemBackend*>(m_backend)->issueRequest( mreq->id(), mreq->addr(), mreq->isWrite(), m_backendRequestWidth );
    } else {
        CustomReq * creq = static_cast<CustomReq*>(req);
        return static_cast<SimpleMemBackend*>(m_backend)->issueCustomRequest( creq->id(), creq->getInfo() );
    }
}
