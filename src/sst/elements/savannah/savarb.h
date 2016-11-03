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


#ifndef _H_SST_SAVANNAH_ARBITRATOR
#define _H_SST_SAVANNAH_ARBITRATOR

#include <sst/elements/memHierarchy/memoryController.h>
#include <sst/elements/memHierarchy/membackend/memBackend.h>

#include <queue>

#include "savevent.h"

using namespace SST::MemHierarchy;

namespace SST {
namespace Savannah {

class SavannahIssueArbitrator : public SST::SubComponent {
public:
        SavannahIssueArbitrator(Component* comp, Params& params) : SubComponent(comp) {}
	~SavannahIssueArbitrator() {}
        virtual void issue(std::queue<SavannahRequestEvent*>& q, MemBackend* backend) = 0;
};

}
}

#endif
