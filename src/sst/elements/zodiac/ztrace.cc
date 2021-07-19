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

#include "sst/core/subcomponent.h"
#include "sst/core/params.h"

#include "ztrace.h"

using namespace SST;
using namespace SST::Zodiac;

ZodiacTraceReader::ZodiacTraceReader(ComponentId_t id, Params& params) :
  Component(id) {

      assert(0);
    std::string msgiface = params.find<std::string>("msgapi");

/*    if ( msgiface == "" ) {
        msgapi = new MP::Interface( this );
    } else {
        msgapi = dynamic_cast<MP::Interface*>(loadSubComponent(msgiface, this, params));

        if(NULL == msgapi) {
		std::cerr << "Message API: " << msgiface << " could not be loaded." << std::endl;
		exit(-1);
        }
    }*/
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

