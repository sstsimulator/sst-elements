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

#include <string>
#include <iostream>

#include <sst_config.h>

#include "Opal.h"


using namespace SST::Interfaces;
using namespace SST;
using namespace SST::OpalComponent;


#define OPAL_VERBOSE(LEVEL, OUTPUT) if(verbosity >= (LEVEL)) OUTPUT



Opal::Opal(SST::ComponentId_t id, SST::Params& params): Component(id)
{

 registerAsPrimaryComponent();
//    primaryComponentDoNotEndSim();

	core_count = (uint32_t) params.find<uint32_t>("num_cores", 1);
	latency = (uint32_t) params.find<uint32_t>("latency", 1);

	std::cout<<"The latency is "<<latency<<std::endl;

	Handlers = new core_handler[core_count*2];

	// Note the links can also come directly form Ariel to send hints, rathar than only from Samba's Page Table Walker
	samba_to_opal = new SST::Link * [core_count*2];

	char* link_buffer = (char*) malloc(sizeof(char) * 256);

	for(uint32_t i = 0; i < core_count * 2; ++i)
      {
		sprintf(link_buffer, "requestLink%" PRIu32, i);
		SST::Link * link = configureLink(link_buffer, "1ns", new Event::Handler<core_handler>((&Handlers[i]), &core_handler::handleRequest));
		samba_to_opal[i] = link;
		Handlers[i].singLink = link;
		Handlers[i].id = i;
		Handlers[i].latency = latency;
	}

	std::string cpu_clock = params.find<std::string>("clock", "1GHz");

	registerClock( cpu_clock, new Clock::Handler<Opal>(this, &Opal::tick ) );
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

