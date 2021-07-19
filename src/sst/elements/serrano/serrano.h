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

#ifndef _H_SST_SERRANO
#define _H_SST_SERRANO

#include <sst/core/sst_config.h>
#include <sst/core/component.h>
#include <sst/core/output.h>

#include <cstdio>

#include "smsg.h"
#include "scircq.h"
#include "sercgunit.h"


namespace SST {
namespace Serrano {

class SerranoComponent : public SST::Component {

public:
	SerranoComponent( SST::ComponentId_t id, SST::Params& params );
	~SerranoComponent();

	bool tick( SST::Cycle_t currentCycle );

	SST_ELI_REGISTER_COMPONENT(
		SerranoComponent,
		"serrano",
		"Serrano",
		SST_ELI_ELEMENT_VERSION( 1, 0, 0 ),
		"High-Level CGRA Simulation Model",
		COMPONENT_CATEGORY_PROCESSOR
		)

	SST_ELI_DOCUMENT_PARAMS(

		)

	SST_ELI_DOCUMENT_STATISTICS(

		)

	void clearGraph();
	void constructGraph( SST::Output* output, const char* kernel_file );

private:
	int read_line( FILE* file_h, char* buffer, const size_t buffer_max );

	SST::Output* output;
	std::list< std::string > kernel_queue;
	std::map< uint64_t, SerranoCoarseUnit* > units;
	std::map< uint64_t, SerranoCircularQueue<SerranoMessage*>* > msg_queues;
	

};

}
}

#endif
