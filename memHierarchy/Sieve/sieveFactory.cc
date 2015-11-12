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

/*
 * File:   sieveFactory.cc
 */


#include <sst_config.h>
#include <sst/core/params.h>
#include "../util.h"
#include "../hash.h"
#include "sieveController.h"


namespace SST{ namespace MemHierarchy{
    using namespace SST::MemHierarchy;
    using namespace std;

Sieve* Sieve::sieveFactory(ComponentId_t id, Params &params){
 
    /* --------------- Output Class --------------- */
    Output* output = new Output();
    int debugLevel = params.find_integer("debug_level", 0);
    
    output->init("--->  ", debugLevel, 0,(Output::output_location_t)params.find_integer("debug", 0));
    if(debugLevel < 0 || debugLevel > 10)     output->fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");
    output->debug(_INFO_,"\n--------------------------- Initializing [Memory Hierarchy] --------------------------- \n\n");

    /* --------------- Get Parameters --------------- */
    // LRU - default replacement policy
    int associativity           = params.find_integer("associativity", -1);
    string sizeStr              = params.find_string("cache_size", "");                  //Bytes
    int lineSize                = params.find_integer("cache_line_size", -1);            //Bytes

    /* Check user specified all required fields */
    if(-1 >= associativity)         output->fatal(CALL_INFO, -1, "Param not specified: associativity\n");
    if(sizeStr.empty())             output->fatal(CALL_INFO, -1, "Param not specified: cache_size\n");
    if(-1 == lineSize)              output->fatal(CALL_INFO, -1, "Param not specified: cache_line_size - number of bytes in a cacheline (block size)\n");
    
    long cacheSize = SST::MemHierarchy::convertToBytes(sizeStr);
    uint numLines = cacheSize/lineSize;

    /* ---------------- Initialization ----------------- */
    HashFunction* ht = new PureIdHashFunction;
    ReplacementMgr* replManager = new LRUReplacementMgr(output, numLines, associativity, true);
    CacheArray* cacheArray = new SetAssociativeArray(output, numLines, lineSize, associativity, replManager, ht, false);
    
    return new Sieve(id, params, cacheArray, output);
}



Sieve::Sieve(ComponentId_t id, Params &params, CacheArray * cacheArray, Output * output) : Component(id){
    cacheArray_ = cacheArray;
    output_ = output;
    output_->debug(_INFO_,"--------------------------- Initializing [Sieve]: %s... \n", this->Component::getName().c_str());
    
    /* --------------- Sieve profiler - implemented as a cassini prefetcher subcomponent ---------------*/
    string listener   = params.find_string("profiler");
    if (listener.empty()) {
	  Params emptyParams;
	  listener_ = new CacheListener(this, emptyParams);
    } else {
      if (listener != std::string("cassini.AddrHistogrammer")) {
          output_->fatal(CALL_INFO, -1, "%s, Sieve does not support prefetching. It can only support "
              "profiling through Cassini's AddrHistogrammer.",
              getName().c_str());
      }
	  Params listenerParams = params.find_prefix_params("profiler." );
	  listener_ = dynamic_cast<CacheListener*>(loadSubComponent(listener, this, listenerParams));
    }
    
    configureLinks();
}

void Sieve::configureLinks() {
    SST::Link* link;
    cpuLinkCount_ = 0;
    for ( int i = 0 ; i < 200 ; i++ ) { // 200 is chosen to be reasonably large but no reason it can't be larger
        std::ostringstream linkName;
        linkName << "cpu_link_" << i;
        std::string ln = linkName.str();
        link = configureLink(ln, "100 ps", new Event::Handler<Sieve>(this, &Sieve::processEvent));
        if (link) {
            cpuLinks_.push_back(link);
            cpuLinkCount_++;
            output_->output(CALL_INFO, "Port %lu = Link %d\n", cpuLinks_[i]->getId(), i);
        }
    }
    if (cpuLinkCount_ < 1) output_->fatal(CALL_INFO, -1,"Did not find any connected links on ports cpu_link_\%d\n");
}

}}
