// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/serialization.h"
#include <assert.h>

#include "sst/core/element.h"

#include "zdumpi.h"

using namespace std;
using namespace SST;
using namespace SST::Zodiac;

ZodiacDUMPITraceReader::ZodiacDUMPITraceReader(ComponentId_t id, Params& params) :
  Component(id) {

    string msgiface = params.find_string("msgapi");

    if ( msgiface == "" ) {
        msgapi = new SST::Hermes::MP::Interface();
    } else {
	msgapi = dynamic_cast<SST::Hermes::MP::Interface*>(loadModule(msgiface, params));

        if(NULL == msgapi) {
		std::cerr << "Message API: " << msgiface << " could not be loaded." << std::endl;
		exit(-1);
        }
    }

    string trace_file = params.find_string("tracefile");
    if("" == trace_file) {
	std::cerr << "Error: could not find a file contain a trace to simulate!" << std::endl;
	exit(-1);
    }

    uint32_t rank = (uint32_t) params.find_integer("rank", 0);
    eventQ = new std::queue<ZodiacEvent*>();
    trace = new DUMPIReader(trace_file, rank, 64, eventQ);
}

ZodiacDUMPITraceReader::~ZodiacDUMPITraceReader() {
	trace->close();
}

ZodiacDUMPITraceReader::ZodiacDUMPITraceReader() :
    Component(-1)
{
    // for serialization only
}

void ZodiacDUMPITraceReader::handleEvent(Event *ev) {

}

bool ZodiacDUMPITraceReader::clockTic( Cycle_t ) {
  // return false so we keep going
  return false;
}

BOOST_CLASS_EXPORT(SST::Zodiac::ZodiacDUMPITraceReader)
