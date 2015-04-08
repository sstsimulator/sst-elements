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

// File:   addrHistogrammer.cc
// Author: Jagan Jayaraj (derived from Cassini prefetcher modules written by Si Hammond)

#include "sst_config.h"
#include "addrHistogrammer.h"

#include <stdint.h>

#include "sst/core/element.h"
#include "sst/core/params.h"
#include "sst/core/serialization.h"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Cassini;

AddrHistogrammer::AddrHistogrammer(Component* owner, Params& params) : CacheListener(owner, params) {
	Simulation::getSimulation()->requireEvent("memHierarchy.MemEvent");
	blockSize = (uint32_t) params.find_integer("cache_line_size", 64);
	binWidth = (uint32_t) params.find_integer("histo_bin_width", 64);
    // Get the level of verbosity the user is asking to print out, default is 1
    // which means don't print much.
    verbosity = (uint32_t) params.find_integer("verbose", 0);
    
    rdHisto = new Statistics::Histogram<uint32_t,uint32_t>("Histogram of reads",binWidth);
    wrHisto = new Statistics::Histogram<uint32_t,uint32_t>("Histogram of writes",binWidth);

    output = new Output();
    output->init("", verbosity, (uint32_t) 0, Output::FILE);
}


AddrHistogrammer::~AddrHistogrammer() {
    delete rdHisto;
    delete wrHisto;
    delete output;
}


void AddrHistogrammer::notifyAccess(const NotifyAccessType notifyType, const NotifyResultType notifyResType,
	const Addr addr, const uint32_t size)
{
    if(notifyResType != MISS) return;
    Addr baseAddr = (addr - (addr % blockSize));
    switch (notifyType) {
      case READ:
        // Add to the read histogram
        rdHisto->add(baseAddr);
        return;
      case WRITE:
        // Add to the write hitogram
        wrHisto->add(baseAddr);
        return;
    }
}

void AddrHistogrammer::registerResponseCallback(Event::HandlerBase *handler) {
	registeredCallbacks.push_back(handler);
}

// print the Histogram efficiently using string buffers
void AddrHistogrammer::printHistogram(Statistics::Histogram<uint32_t, uint32_t> *histo, std::string RdWr) {
    uint32_t binstart = histo->getBinStart();
    uint32_t binend   = histo->getBinEnd();
    uint32_t binwidth = histo->getBinWidth();
    uint32_t nbins    = histo->getBinCount();
    
    // The output method in the Output class performs an fflush for every
    //  single invocation when the target is a FILE.  Since we don't care
    //  so much about the ordering of the prints between different processes
    //  here, we want to minimize the number of calls to output.  We pack
    //  all the print statements in a string buffer, and call output just once.
    std::string prefix = "addrHistogrammer (" + RdWr +"):";
    
    // compute the size of the string buffer
    int addrlen  = 20; // We are assuming the worst case address length.
                       // 2^64 is twenty chars wide in ASCII
    int countlen = 10; // Counts are assumed to be 32-bit integers.
                       // It takes 10 ASCII characters to represent 2^32
    int nspaces  =  5; // Spaces between the addresses and count
    int nbrackets=  2; // Number of brackets
    int linelen  = prefix.length() + 2*addrlen + countlen + nspaces + nbrackets + 1;
                  // Length of the prefix + 2 addresses + 1 count + spaces + brackets + newline
    int buflen   = linelen * nbins;
    
    std::string strbuf;
    strbuf.reserve(buflen);
    std::stringstream ss(strbuf); // Initialize the stringstream to be the size of the buffer
                                  // Yes it's a wasteful copy, but far better than file flushes
    ss.seekp(0);                  // Reset the output stream to the first char in the buffer
    
    //ss << "Histogram Min: %" << binstart << "\n";
    //ss << "Histogram Max: %" << binend   << "\n";
    //ss << "Histogram Bin: %" << binwidth << "\n";
    
    for(uint32_t i = binstart; i <= binend; i += binwidth) {
        if( histo->getBinCountByBinStart(i) > 0 ) {
            ss << prefix << " [" << i << ", " << (i + binwidth) << "]   "
                << histo->getBinCountByBinStart(i) << "\n";
        }
    }
    
    output->output(1,"addrHistogrammer.cc","printHistogram","%s",ss.str().c_str());
        // Both str() and c_str() create new objects or arrays and involve data
        //  copies, but definitely better than fflushes.
}

// Called during Cache::finish
void AddrHistogrammer::printStats(Output& dbg) {
    //output->output(1,"addrHistogrammer.cc","printStats","Reads: Address - Address+binWidth    Count\n");
    //output->output(1,"addrHistogrammer.cc","printStats","Writes: Address - Address+binWidth    Count\n");
    AddrHistogrammer::printHistogram(rdHisto, "read");
    AddrHistogrammer::printHistogram(wrHisto, "write");
}
