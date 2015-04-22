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

#include <assert.h>

#include "sst_config.h"
#include "simpleRNGComponent.h"

#include "sst/core/rng/mersenne.h"
#include "sst/core/rng/marsaglia.h"

using namespace SST;
using namespace SST::RNG;
using namespace SST::SimpleRNGComponent;

simpleRNGComponent::simpleRNGComponent(ComponentId_t id, Params& params) :
  Component(id) 
{
    rng_count = 0;
    rng_max_count = params.find_integer("count", 1000);

    std::string rngType = params.find_string("rng", "mersenne");
    
    if (rngType == "mersenne") {
        unsigned int seed =  params.find_integer("seed", 1447);
        
        std::cout << "Using Mersenne Random Number Generator with seed = " << seed << std::endl;
        rng = new MersenneRNG(seed);
    } else if (rngType == "marsaglia") {
        unsigned int m_w = params.find_integer("seed_w", 0);
        unsigned int m_z = params.find_integer("seed_z", 0);
        
        if(m_w == 0 || m_z == 0) {
            std::cout << "Using Marsaglia Random Number Generator with no seeds ..." << std::endl;
            rng = new MarsagliaRNG();
        } else {
            std::cout << "Using Marsaglia Random Number Generator with seeds m_z = " << m_z << ", m_w = " << m_w << std::endl;
            rng = new MarsagliaRNG(m_z, m_w);
        }
        
    } else {
        std::cout << "RNG provided but unknown " << rngType << ", so using Mersenne with seed = 1447..." << std::endl;
        rng = new MersenneRNG(1447);
    }

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    //set our clock
    registerClock("1GHz", new Clock::Handler<simpleRNGComponent>(this, 
			       &simpleRNGComponent::tick));
}

simpleRNGComponent::simpleRNGComponent() :
    Component(-1)
{
    // for serialization only
}

bool simpleRNGComponent::tick( Cycle_t ) 
{
    double nU = rng->nextUniform();
    uint32_t U32 = rng->generateNextUInt32();
    uint64_t U64 = rng->generateNextUInt64();
    int32_t I32 = rng->generateNextInt32();
    int64_t I64 = rng->generateNextInt64();
    rng_count++;
    
    std::cout << "Random: " << rng_count << " of " << rng_max_count << ": " <<
    nU << ", " << U32 << ", " << U64 << ", " << I32 <<
    ", " << I64 << std::endl;

  	// return false so we keep going
  	if(rng_count == rng_max_count) {
  	    primaryComponentOKToEndSim();
  		return true;
  	} else {
  		return false;
  	}
}

// Element Library / Serialization stuff

BOOST_CLASS_EXPORT(SST::SimpleRNGComponent::simpleRNGComponent)

