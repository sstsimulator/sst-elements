// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_OS_UNAME_HANDLER
#define _H_VANADIS_OS_UNAME_HANDLER

#include "os/node/vnodeoshstate.h"
#include <cstdint>
#include <functional>

namespace SST {
namespace Vanadis {

class VanadisUnameHandlerState : public VanadisHandlerState {
public:
    VanadisUnameHandlerState(uint32_t verbosity) : VanadisHandlerState(verbosity) { completed = true; }

    virtual void handleIncomingRequest(StandardMem::Request* req) {
        output->verbose(CALL_INFO, 16, 0,
                        "-> [syscall-uname] handle incoming event for uname(), %s\n",
                        req->getString().c_str());
    }

    virtual VanadisSyscallResponse* generateResponse() { return new VanadisSyscallResponse(0); }
};

} // namespace Vanadis
} // namespace SST

#endif
