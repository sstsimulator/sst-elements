// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_CTRL_MSG_TIMING_BASE_H
#define COMPONENTS_FIREFLY_CTRL_MSG_TIMING_BASE_H

namespace SST {
namespace Firefly {
namespace CtrlMsg {

class MsgTimingBase {
  public:

    virtual ~MsgTimingBase() {}
    virtual uint64_t txDelay( size_t ) = 0;
    virtual uint64_t rxDelay( size_t ) = 0;

    virtual uint64_t sendReqFiniDelay( size_t ) = 0;
    virtual uint64_t recvReqFiniDelay( size_t ) = 0;
    virtual uint64_t rxPostDelay_ns( size_t ) = 0;
    virtual uint64_t sendAckDelay() = 0;
    virtual uint64_t shortMsgLength() = 0;
};

}
}
}

#endif
