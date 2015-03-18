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
#include "sst/core/params.h"

#include "ztrace.h"

using namespace SST;
using namespace SST::Zodiac;

ZodiacTraceReader::ZodiacTraceReader(ComponentId_t id, Params& params) :
  Component(id) {

    std::string msgiface = params.find_string("msgapi");

    if ( msgiface == "" ) {
        msgapi = new MP::Interface();
    } else {
	msgapi = dynamic_cast<MP::Interface*>(loadModule(msgiface, params));

        if(NULL == msgapi) {
		std::cerr << "Message API: " << msgiface << " could not be loaded." << std::endl;
		exit(-1);
        }
    }
}

ZodiacTraceReader::ZodiacTraceReader() :
    Component(-1)
{
    // for serialization only
}

void ZodiacTraceReader::handleEvent(Event *ev) {

}

bool ZodiacTraceReader::clockTic( Cycle_t ) {
  // return false so we keep going
  return false;
}

BOOST_CLASS_EXPORT(ZodiacTraceReader)
