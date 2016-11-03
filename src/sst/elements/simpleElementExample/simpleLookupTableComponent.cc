// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

#include "simpleLookupTableComponent.h"

#include <sst/core/params.h>
#include <sst/core/sharedRegion.h>

namespace SST {
namespace SimpleElementExample {

SimpleLookupTableComponent::SimpleLookupTableComponent(SST::ComponentId_t id, SST::Params &params) : Component(id)
{
    char buffer[128] = {0};
    snprintf(buffer, 128, "LookupTableComponent %3lu  [@t]  ", id);
    out.init(buffer, 0, 0, Output::STDOUT);

    const std::string & fname = params.find<std::string>("filename", "");
    if ( !fname.empty() ) {
        struct stat buf;
        int ret = stat(fname.c_str(), &buf);
        if ( 0 != ret )
            out.fatal(CALL_INFO, 1, "Unable to load lookup table. stat(%s) failed with code %d\n", fname.c_str(), errno);
        tableSize = buf.st_size;

        sregion = getLocalSharedRegion("SimpleLookupTable", tableSize);
        if ( 0 == sregion->getLocalShareID() ) {
            FILE *fp = fopen(fname.c_str(), "r");
            if ( !fp )
                out.fatal(CALL_INFO, 1, "Unable to read file %s\n", fname.c_str());
            fread(sregion->getRawPtr(), 1, tableSize, fp);
            fclose(fp);
        }
    } else {
        tableSize = (size_t)params.find<int64_t>("num_entities", 1) * sizeof(size_t);
        size_t myID = (size_t)params.find<int64_t>("myid", 1);
        sregion = getGlobalSharedRegion("SimpleLookupTable", tableSize, new SharedRegionMerger());
        sregion->modifyArray(myID, myID);
    }
    sregion->publish();

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    registerClock("1GHz", new Clock::Handler<SimpleLookupTableComponent>(this, &SimpleLookupTableComponent::tick));
}


SimpleLookupTableComponent::~SimpleLookupTableComponent()
{
    sregion->shutdown();
}


void SimpleLookupTableComponent::init(unsigned int phase)
{
}


void SimpleLookupTableComponent::setup()
{
    table = sregion->getPtr<const uint8_t*>();
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


