// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//


//
/* Author: Amro Awad
 * E-mail: aawad@sandia.gov
 */



#include <sst_config.h>
#include <string>
#include "Opal.h"


using namespace SST::Interfaces;
using namespace SST;
using namespace SST::OpalComponent;


#define OPAL_VERBOSE(LEVEL, OUTPUT) if(verbosity >= (LEVEL)) OUTPUT



Opal::Opal(SST::ComponentId_t id, SST::Params& params): Component(id) {


	int core_count = (uint32_t) params.find<uint32_t>("num_cores", 1);

	Handlers = new core_handler[core_count];

	samba_to_opal = new SST::Link * [core_count];

	char* link_buffer = (char*) malloc(sizeof(char) * 256);

	for(uint32_t i = 0; i < core_count; ++i) {
		sprintf(link_buffer, "requestLink%" PRIu32, i);
		SST::Link * link = configureLink(link_buffer, "0ps", new Event::Handler<core_handler>((&Handlers[i]), &core_handler::handleRequest));
		samba_to_opal[i] = link;
		Handlers[i].singLink = link;
		Handlers[i].id = i;
	}

	std::string cpu_clock = params.find<std::string>("clock", "1GHz");

}



Opal::Opal() : Component(-1)
{
	// for serialization only
	// 
}



bool Opal::tick(SST::Cycle_t x)
{

	// We tick the MMU hierarchy of each core
	//	for(uint32_t i = 0; i < core_count; ++i)

	return false;
}


void Opal::handleRequest( SST::Event* e )
{







}

