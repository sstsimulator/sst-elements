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
#include "membackend/simpleMemScratchBackendConvertor.h"
#include "membackend/memBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

SimpleMemScratchBackendConvertor::SimpleMemScratchBackendConvertor(Component *comp, Params &params) :
        ScratchBackendConvertor(comp, params) 
{
    using std::placeholders::_1;
    static_cast<SimpleMemBackend*>(m_backend)->setResponseHandler( std::bind( &SimpleMemScratchBackendConvertor::handleMemResponse, this, _1 ) );
}

bool SimpleMemScratchBackendConvertor::issue( MemReq* req ) {
    return static_cast<SimpleMemBackend*>(m_backend)->issueRequest( req->id(), req->addr(), req->isWrite(), m_backendRequestWidth );
}
