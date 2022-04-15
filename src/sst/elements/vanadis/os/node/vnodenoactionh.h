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

#ifndef _H_VANADIS_OS_NO_ACTION_HANDLER
#define _H_VANADIS_OS_NO_ACTION_HANDLER

#include "os/node/vnodeoshstate.h"
#include <cstdint>
#include <functional>

namespace SST {
namespace Vanadis {

class VanadisNoActionHandlerState : public VanadisHandlerState {
public:
    VanadisNoActionHandlerState(uint32_t verbosity) : VanadisHandlerState(verbosity), return_code(0) {

        completed = true;
    }

    VanadisNoActionHandlerState(uint32_t verbosity, int64_t rc) : VanadisHandlerState(verbosity), return_code(rc) {

        completed = true;
    }

    virtual void handleIncomingRequest(StandardMem::Request* req) {}

    virtual VanadisSyscallResponse* generateResponse() { return new VanadisSyscallResponse(return_code); }

protected:
    int64_t return_code;
};

} // namespace Vanadis
} // namespace SST

#endif
