// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "addrHistogrammer.h"

#include <stdint.h>

#include "sst/core/element.h"
#include "sst/core/params.h"
#include "sst/core/serialization.h"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Cassini;

AddrHistogrammer::AddrHistogrammer(Params& params) {
	Simulation::getSimulation()->requireEvent("memHierarchy.MemEvent");
	blockSize = (uint32_t) params.find_integer("cache_line_size", 64);
	binWidth = (uint32_t) params.find_integer("bin_width", 64);
    rdHisto = new Statistics::Histogram<uint32_t,uint32_t>("Histogram of reads",binWidth);
    wrHisto = new Statistics::Histogram<uint32_t,uint32_t>("Histogram of writes",binWidth);
    // Get the level of verbosity the user is asking to print out, default is 1
    // which means don't print much.
    verbosity = (uint32_t) params.find_integer("verbose", 1);
}


AddrHistogrammer::~AddrHistogrammer() { delete rdHisto;  delete wrHisto; }


void AddrHistogrammer::notifyAccess(NotifyAccessType notifyType, NotifyResultType notifyResType, Addr addr)
{
	if(notifyResType == MISS) {
       Addr baseAddr = (addr - (addr % blockSize));
       if(notifyType == READ) {
          // Add to the read histogram
          rdHisto->add(baseAddr);
       } else {
          // Add to the write hitogram
          wrHisto->add(baseAddr);
       }
	}
}

void AddrHistogrammer::registerResponseCallback(Event::HandlerBase *handler)  
{
	registeredCallbacks.push_back(handler);
}

void AddrHistogrammer::setOwningComponent(const SST::Component* own) 
{
	owner = own;
}

// copied from emberengine.cc
void AddrHistogrammer::printHistogram(Statistics::Histogram<uint32_t, uint32_t> *histo) {
        output->output(1,"addrHistogrammer.cc","printHistogram","Histogram Min: %" PRIu32 "\n", histo->getBinStart());
        output->output(1,"addrHistogrammer.cc","printHistogram","Histogram Max: %" PRIu32 "\n", histo->getBinEnd());
        output->output(1,"addrHistogrammer.cc","printHistogram","Histogram Bin: %" PRIu32 "\n", histo->getBinWidth());
        for(uint32_t i = histo->getBinStart(); i <= histo->getBinEnd(); i += histo->getBinWidth()) {
                if( histo->getBinCountByBinStart(i) > 0 ) {
                        output->output(1,"addrHistogrammer.cc","printHistogram"," [%" PRIu32 ", %" PRIu32 "]   %" PRIu32 "\n",
                                i, (i + histo->getBinWidth()), histo->getBinCountByBinStart(i));
                }
        }
}

// Called during Cache::finish
void AddrHistogrammer::printStats(Output& dbg) {
    output = new Output();
    output->init("addrHistogrammer: ", verbosity, (uint32_t) 0, Output::FILE);
    
    output->output(1,"addrHistogrammer.cc","printStats","Reads: Address - Address+binWidth    Count\n");
    AddrHistogrammer::printHistogram(rdHisto);
    output->output(1,"addrHistogrammer.cc","printStats","Writes: Address - Address+binWidth    Count\n");
    AddrHistogrammer::printHistogram(wrHisto);
    delete output;
}
