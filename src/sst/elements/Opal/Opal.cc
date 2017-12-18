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


#define MESSIER_VERBOSE(LEVEL, OUTPUT) if(verbosity >= (LEVEL)) OUTPUT


// Here we do the initialization of the Samba units of the system, connecting them to the cores and instantiating TLB hierachy objects for each one

Opal::Opal(SST::ComponentId_t id, SST::Params& params): Component(id) {


	//registerClock( cpu_clock, new Clock::Handler<Opal>(this, &Opal::tick ) );

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
