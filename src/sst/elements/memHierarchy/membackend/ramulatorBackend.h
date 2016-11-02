// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEMH_RAMULATOR_BACKEND
#define _H_SST_MEMH_RAMULATOR_BACKEND

#include "membackend/memBackend.h"

#include "Gem5Wrapper.h"

#ifdef OLD_DEBUG
#define DEBUG OLD_DEBUG
#undef OLD_DEBUG
#endif

namespace SST {
namespace MemHierarchy {

class ramulatorMemory : public MemBackend {
public:
    ramulatorMemory(Component *comp, Params &params);
    virtual bool issueRequest(DRAMReq *req);
    virtual void clock();
    virtual void finish();

protected:
    ramulator::Gem5Wrapper *memSystem;
    std::function<void(ramulator::Request&)> callBackFunc;
    std::map<uint64_t, std::deque<DRAMReq*> > dramReqs;
    void ramulatorDone(ramulator::Request& req);
};

}
}

#endif
