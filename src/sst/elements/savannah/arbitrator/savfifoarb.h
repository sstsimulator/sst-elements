// Copyright 2009-2018 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_SAVANNAH_IN_ORDER_ARB
#define _H_SST_SAVANNAH_IN_ORDER_ARB

#include <sst/core/output.h>
#include <sst/core/params.h>

namespace SST {
namespace Savannah {

class SavannahInOrderArbitrator : public SavannahIssueArbitrator {
public:
	SavannahInOrderArbitrator(Component* comp, Params& params) :
		SavannahIssueArbitrator(comp, params) {

		const int verbose = params.find<int64_t>("verbose");
		output = new SST::Output("SavannahFIFOArb[@p:@l]: ",
			verbose, 0, SST::Output::STDOUT);

		maxIssuePerCycle = params.find<int64_t>("max_issue_per_cycle");
	}

	~SavannahInOrderArbitrator() {
		delete output;
	}

	void issue(std::queue<SavannahRequestEvent*>& q, MemBackend* backend) {

	}

private:
	uint32_t maxIssuePerCycle;
	Output* output;

};

}
}

#endif
