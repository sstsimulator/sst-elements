// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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

    virtual void handleIncomingRequest(SimpleMem::Request* req) {
        output->verbose(CALL_INFO, 16, 0,
                        "-> [syscall-uname] handle incoming event for uname(), "
                        "req->size: %" PRIu32 "\n",
                        (uint32_t)req->size);
    }

    virtual VanadisSyscallResponse* generateResponse() { return new VanadisSyscallResponse(0); }
};

} // namespace Vanadis
} // namespace SST

#endif
