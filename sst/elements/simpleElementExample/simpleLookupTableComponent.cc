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

#include <sst_config.h>
#include <sst/core/serialization.h>

#include <errno.h>

#include "simpleLookupTableComponent.h"

#include <sst/core/params.h>
#include <sst/core/lookupTable.h>

namespace SST {
namespace SimpleElementExample {

SimpleLookupTableComponent::SimpleLookupTableComponent(SST::ComponentId_t id, SST::Params &params) : Component(id)
{
    char buffer[128] = {0};
    snprintf(buffer, 128, "LookupTableComponent %3lu  [@t]  ", id);
    out.init(buffer, 0, 0, Output::STDOUT);

    const std::string & fname = params.find_string("filename", "");
    if ( fname.empty() )
        out.fatal(CALL_INFO, 1, "Must specify a filename for the lookup table.\n");

    tableSize = 0;
    table = (const uint8_t *)getLookupTable("SimpleLookupTable", new SimpleLookupTableBuilder(fname, &tableSize));
    if ( !table || tableSize == 0 )
        out.fatal(CALL_INFO, 1, "Unable to load lookup table.  Error code: %d\n", errno);


    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    registerClock("1GHz", new Clock::Handler<SimpleLookupTableComponent>(this, &SimpleLookupTableComponent::tick));
}


SimpleLookupTableComponent::~SimpleLookupTableComponent()
{
}


void SimpleLookupTableComponent::init(unsigned int phase)
{
}


void SimpleLookupTableComponent::setup()
{
}


void SimpleLookupTableComponent::finish()
{
}


bool SimpleLookupTableComponent::tick(SST::Cycle_t)
{
    bool done = false;
    static const size_t nPerRow = 8;
    if ( tableSize > 0 ) {
        char buffer[nPerRow*5 + 1] = {0};
        size_t nitems = std::min(nPerRow, tableSize);
        for ( size_t i = 0 ; i < nitems ; i++ ) {
            char  tbuf[6] = {0};
            sprintf(tbuf, "0x%02x ", *table++); tableSize--;
            strcat(buffer, tbuf);
        }
        out.output(CALL_INFO, "%s\n", buffer);
    }

    done = (tableSize == 0);

    if ( done ) {
        primaryComponentOKToEndSim();
        return true;
    }
    return false;
}


}
}


