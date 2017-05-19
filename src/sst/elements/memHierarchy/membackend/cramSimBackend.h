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


#ifndef _H_SST_MEMH_CRAM_SIM_BACKEND
#define _H_SST_MEMH_CRAM_SIM_BACKEND

#include "membackend/memBackend.h"

namespace SST {
namespace MemHierarchy {

class CramSimMemory : public SimpleMemBackend {
public:
    CramSimMemory(Component *comp, Params &params);
	virtual bool issueRequest( ReqId, Addr, bool isWrite, unsigned numBytes );
private:
    void handleCramsimEvent(SST::Event *event);

	std::set<ReqId> dramReqs;
    SST::Link *cramsim_link;
};

}
}

#endif
