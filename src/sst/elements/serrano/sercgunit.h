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
\
#ifndef _H_SERRANO_COARSE_UNIT
#define _H_SERRANO_COARSE_UNIT

#include <sst/core/subcomponent.h>

#include <cinttypes>
#include <cstdint>
#include <vector>

#include "scircq.h"
#include "smsg.h"

namespace SST {
namespace Serrano {

enum SerranoStandardType {
        TYPE_INT32,
        TYPE_INT64,
        TYPE_FP32,
        TYPE_FP64,
        TYPE_CUSTOM
};

class SerranoCoarseUnit : public SST::SubComponent {

public:
	SST_ELI_REGISTER_SUBCOMPONENT_API( SST::Serrano::SerranoCoarseUnit )

	SerranoCoarseUnit( SST::ComponentId_t id, Params& params ) : 
		SubComponent(id) {

		int verbosity = params.find<int>("verbose", 0);
		char* comp_name = new char[64];
		snprintf(comp_name, 64, "[cgra]: ");

		output = new SST::Output(comp_name, verbosity, 0, Output::STDOUT );
	}

	~SerranoCoarseUnit() {
		delete output;
	}

	virtual bool stillProcessing() = 0;
	virtual void execute( const uint64_t current_cycle ) = 0;

	void addInputQueue( SerranoCircularQueue<SerranoMessage*>* new_q ) {
		output->verbose(CALL_INFO, 4, 0, "Added input queue.\n");
		input_qs.push_back( new_q );
	}

	void addOutputQueue( SerranoCircularQueue<SerranoMessage*>* new_q ) {
		output->verbose(CALL_INFO, 4, 0, "Added output queue.\n");
		output_qs.push_back( new_q );
	}

	virtual const char* getUnitTypeString() = 0;

	size_t countInputQueues()  const { return input_qs.size(); }
	size_t countOutputQueues() const { return output_qs.size(); }

	virtual void checkRequiredQueues( SST::Output* output ) = 0;

protected:
	SST::Output* output;
	std::vector< SerranoCircularQueue<SerranoMessage*>* > input_qs;
	std::vector< SerranoCircularQueue<SerranoMessage*>* > output_qs;

};

}
}

#endif
