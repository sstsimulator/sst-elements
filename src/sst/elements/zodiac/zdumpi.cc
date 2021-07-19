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


#include "sst_config.h"
#include <assert.h>

#include "zdumpi.h"

using namespace std;
using namespace SST;
using namespace SST::Zodiac;

ZodiacDUMPITraceReader::ZodiacDUMPITraceReader(ComponentId_t id, Params& params) :
  Component(id) {

    string msgiface = params.find<std::string>("msgapi");

    if ( msgiface == "" ) {
        msgapi = new SST::Hermes::MP::Interface(this);
    } else {
	msgapi = dynamic_cast<SST::Hermes::MP::Interface*>(loadSubComponent(msgiface, this, params));

        if(NULL == msgapi) {
		std::cerr << "Message API: " << msgiface << " could not be loaded." << std::endl;
		exit(-1);
        }
    }

    string trace_file = params.find<std::string>("tracefile");
    if("" == trace_file) {
	std::cerr << "Error: could not find a file contain a trace to simulate!" << std::endl;
	exit(-1);
    }

    uint32_t rank = params.find<uint32_t>("rank", 0);
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

