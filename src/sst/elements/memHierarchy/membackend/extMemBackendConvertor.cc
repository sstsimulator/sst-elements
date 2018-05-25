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
#include <vector>
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memoryController.h"
#include "membackend/extMemBackendConvertor.h"
#include "membackend/memBackend.h"
#include "customcmd/customOpCodeCmd.h"

using namespace SST;
using namespace SST::MemHierarchy;

#ifdef __SST_DEBUG_OUTPUT__
#define Debug(level, fmt, ... ) m_dbg.debug( level, fmt, ##__VA_ARGS__  )
#else
#define Debug(level, fmt, ... )
#endif

ExtMemBackendConvertor::ExtMemBackendConvertor(Component *comp, Params &params) :
    MemBackendConvertor(comp,params)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    static_cast<ExtMemBackend*>(m_backend)->setResponseHandler( std::bind( &ExtMemBackendConvertor::handleMemResponse, this, _1,_2 ) );

}

bool ExtMemBackendConvertor::issue( BaseReq *req ) {

    std::vector<uint64_t> NULLVEC;

    if( req->isCustCmd() ){
      // issue custom request
      CustomReq * mreq = static_cast<CustomReq*>(req);
      CustomOpCodeCmdInfo *info = static_cast<CustomOpCodeCmdInfo*>(mreq->getInfo());
      return static_cast<ExtMemBackend*>(m_backend)->issueCustomRequest( mreq->id(),
                                                                         info->getAddr(),
                                                                         info->getOpCode(),
                                                                         NULLVEC, // this is null for normal requests
                                                                         info->getFlags(),
                                                                         m_backendRequestWidth );
    }else{
      // issue standard request
      MemReq * mreq = static_cast<MemReq*>(req);
      return static_cast<ExtMemBackend*>(m_backend)->issueRequest( mreq->id(),
                                                                   mreq->addr(),
                                                                   mreq->isWrite(),
                                                                   NULLVEC, // this is null for normal requests
                                                                   mreq->getMemEvent()->getFlags(),
                                                                   m_backendRequestWidth );
    }
}

// EOF
