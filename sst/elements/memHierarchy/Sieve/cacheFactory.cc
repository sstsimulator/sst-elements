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
 * File:   cacheFactory.cc
 */


#include <sst_config.h>
#include <sst/core/params.h>
#include "../util.h"
#include "../hash.h"
#include "cacheController.h"


namespace SST{ namespace MemHierarchy{
    using namespace SST::MemHierarchy;
    using namespace std;

Sieve* Sieve::cacheFactory(ComponentId_t _id, Params &_params){
 
    /* --------------- Output Class --------------- */
    Output* dbg = new Output();
    int debugLevel = _params.find_integer("debug_level", 0);
    
    dbg->init("--->  ", debugLevel, 0,(Output::output_location_t)_params.find_integer("debug", 0));
    if(debugLevel < 0 || debugLevel > 10)     dbg->fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");
    dbg->debug(_INFO_,"\n--------------------------- Initializing [Memory Hierarchy] --------------------------- \n\n");

    /* --------------- Get Parameters --------------- */
    // LRU - default replacement policy
    int associativity           = _params.find_integer("associativity", -1);
    string sizeStr              = _params.find_string("cache_size", "");                  //Bytes
    int lineSize                = _params.find_integer("cache_line_size", -1);            //Bytes

    /* Check user specified all required fields */
    if(-1 >= associativity)         dbg->fatal(CALL_INFO, -1, "Param not specified: associativity\n");
    if(sizeStr.empty())             dbg->fatal(CALL_INFO, -1, "Param not specified: cache_size\n");
    if(-1 == lineSize)              dbg->fatal(CALL_INFO, -1, "Param not specified: cache_line_size - number of bytes in a cacheline (block size)\n");
    
    long cacheSize = SST::MemHierarchy::convertToBytes(sizeStr);
    uint numLines = cacheSize/lineSize;

    /* ---------------- Initialization ----------------- */
    HashFunction* ht = new PureIdHashFunction;
    ReplacementMgr* replManager = new LRUReplacementMgr(dbg, numLines, associativity, true);
    CacheArray* cacheArray = new SetAssociativeArray(dbg, cacheSize, lineSize, associativity, replManager, ht, false);
    
    CacheConfig config = {cacheArray, dbg, replManager};
    return new Sieve(_id, _params, config);
}



Sieve::Sieve(ComponentId_t _id, Params &_params, CacheConfig _config) : Component(_id){
    cf_ = _config;
    d_  = cf_.dbg_;
    d_->debug(_INFO_,"--------------------------- Initializing [Sieve]: %s... \n", this->Component::getName().c_str());
    
    /* --------------- Prefetcher ---------------*/
    string prefetcher   = _params.find_string("prefetcher");
    if (prefetcher.empty()) {
	  Params emptyParams;
	  listener_ = new CacheListener(this, emptyParams);
    } else {
      if (prefetcher != std::string("cassini.AddrHistogrammer")) {
          d_->fatal(CALL_INFO, -1, "%s, Sieve does not support prefetching. It can only support "
              "profiling through Cassini's AddrHistogrammer.",
              getName().c_str());
      }
	  Params prefetcherParams = _params.find_prefix_params("prefetcher." );
	  listener_ = dynamic_cast<CacheListener*>(loadSubComponent(prefetcher, this, prefetcherParams));
    }
    
    cpu_link = configureLink("cpu_link", "50ps",
               new Event::Handler<Sieve>(this, &Sieve::processEvent));
    if (!cpu_link) {
        d_->fatal(CALL_INFO, -1, "%s, Error creating link to CPU from Sieve", getName().c_str());
    }
}


}}
