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
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memoryController.h"
#include "membackend/flagMemBackendConvertor.h"
#include "membackend/memBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

#ifdef __SST_DEBUG_OUTPUT__
#define Debug(level, fmt, ... ) m_dbg.debug( level, fmt, ##__VA_ARGS__  )
#else
#define Debug(level, fmt, ... )
#endif

FlagMemBackendConvertor::FlagMemBackendConvertor(Component *comp, Params &params) :
    MemBackendConvertor(comp,params) 
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    static_cast<FlagMemBackend*>(m_backend)->setResponseHandler( std::bind( &FlagMemBackendConvertor::handleMemResponse, this, _1,_2 ) );

}

bool FlagMemBackendConvertor::issue( BaseReq *breq ) {
    if (breq->isMemEv()) {
        MemReq * req = static_cast<MemReq*>(breq);
        MemEvent* event = req->getMemEvent();
        return static_cast<FlagMemBackend*>(m_backend)->issueRequest( req->id(), req->addr(), req->isWrite(), event->getFlags(), m_backendRequestWidth );
    } else {
        CustomReq * req = static_cast<CustomReq*>(breq);
        return static_cast<FlagMemBackend*>(m_backend)->issueCustomRequest(req->id(), req->getInfo());
    }
}
