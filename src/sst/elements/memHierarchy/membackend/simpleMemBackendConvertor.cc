// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <sst/core/params.h>
#include "sst/elements/memHierarchy/util.h"
#include "membackend/simpleMemBackendConvertor.h"
#include "membackend/memBackend.h"

namespace {
inline bool logConvReadsEnabled() {
    static int cached = -1;
    if (cached == -1) {
        const char* env = std::getenv("SNNDL_MEM_CONV_DEBUG");
        cached = (env && std::atoi(env) != 0) ? 1 : 0;
    }
    return cached == 1;
}
}

using namespace SST;
using namespace SST::MemHierarchy;


SimpleMemBackendConvertor::SimpleMemBackendConvertor(ComponentId_t id, Params &params, MemBackend* backend, uint32_t reqWidth) :
        MemBackendConvertor(id, params, backend, reqWidth)
{
    using std::placeholders::_1;
    static_cast<SimpleMemBackend*>(m_backend)->setResponseHandler( std::bind( &SimpleMemBackendConvertor::handleMemResponse, this, _1 ) );
}

bool SimpleMemBackendConvertor::issue( BaseReq* req ) {
    if (req->isMemEv()) {
        MemReq * mreq = static_cast<MemReq*>(req);
        uint32_t remaining = mreq->size() - mreq->processed();
        uint32_t chunk = remaining < m_backendRequestWidth && remaining > 0 ? remaining : m_backendRequestWidth;
        if (!mreq->isWrite() && chunk == 0) chunk = remaining;
        if (!mreq->isWrite() && logConvReadsEnabled()) {
            std::printf("[conv-read] issue id=%" PRIu64 " addr=0x%" PRIx64 " chunk=%u remaining=%u\n",
                (uint64_t)mreq->id(), (uint64_t)mreq->addr(), chunk, remaining);
        }
        return static_cast<SimpleMemBackend*>(m_backend)->issueRequest( mreq->id(), mreq->addr(), mreq->isWrite(), chunk );
    } else {
        CustomReq * creq = static_cast<CustomReq*>(req);
        return static_cast<SimpleMemBackend*>(m_backend)->issueCustomRequest( creq->id(), creq->getInfo() );
    }
}

void SimpleMemBackendConvertor::serialize_order(SST::Core::Serialization::serializer& ser) {
    MemBackendConvertor::serialize_order(ser);

    if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
        using std::placeholders::_1;
        static_cast<SimpleMemBackend*>(m_backend)->setResponseHandler( std::bind( &SimpleMemBackendConvertor::handleMemResponse, this, _1 ) );
    }
}
