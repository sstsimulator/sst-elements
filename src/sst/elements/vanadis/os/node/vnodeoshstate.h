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

#ifndef _H_VANADIS_OS_HANDLER_STATE
#define _H_VANADIS_OS_HANDLER_STATE

#include <sst/core/interfaces/stdMem.h>

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisHandlerState {
public:
    VanadisHandlerState(uint32_t verb) : hw_thr(-1) {
        output = new SST::Output("[os-func-handler]: ", verb, 0, SST::Output::STDOUT);
        completed = false;
    }

    virtual ~VanadisHandlerState() { delete output; }

    virtual void handleIncomingRequest(StandardMem::Request* req) {}
    virtual VanadisSyscallResponse* generateResponse() = 0;
    virtual bool isComplete() const { return completed; }
    virtual void markComplete() { completed = true; }
    void setHWThread( int thr ) { hw_thr = thr; }
    int getHWThread() { assert( hw_thr > -1 ); return hw_thr; } 

protected:
    SST::Output* output;
    bool completed;
    int hw_thr;
};

} // namespace Vanadis
} // namespace SST

#endif
