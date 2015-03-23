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
#include "simpleStatisticsComponent.h"

#include "sst/core/rng/mersenne.h"
#include "sst/core/rng/marsaglia.h"

using namespace SST;
using namespace SST::RNG;
using namespace SST::SimpleStatisticsComponent;

simpleStatisticsComponent::simpleStatisticsComponent(ComponentId_t id, Params& params) : Component(id)
{
    // Get runtime parameters from the input file (.py),  
    // these will set the random number generation.
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

    /////////////////////////////////////////
    // Register the Clock

    // First 1ns Clock 
    std::cout << "REGISTER CLOCK #1 at 1 ns" << std::endl;
    registerClock("1 ns", new Clock::Handler<simpleStatisticsComponent>(this, &simpleStatisticsComponent::Clock1Tick));

    /////////////////////////////////////////
    // Create the Statistics objects
    stat1_U32 = registerStatistic<uint32_t>("stat1_U32", "1");
    stat2_U64 = registerStatistic<uint64_t>("stat2_U64", "2");
    stat3_I32 = registerStatistic<int32_t> ("stat3_I32", "3");
    stat4_I64 = registerStatistic<int64_t> ("stat4_I64", "4");
    stat5_U32 = registerStatistic<uint32_t>("stat5_U32", "5");
    stat6_U64 = registerStatistic<uint64_t>("stat6_U64", "6");
    
    // Try to Register Illegal Statistic Objects
    printf("STATISTIC TESTING: TRYING TO REGISTER A DUPLICATE STAT NAME - SHOULD RETURN A NULLSTATISTIC\n");
    stat7_U32_NOTUSED = registerStatistic<uint32_t>("stat5_U32", "5");   // This stat should not be registered because it has a duplicated name

    // Test Statistic functions for delayed output and collection and to disable Stat
//    stat1_U32->disable();
//    stat1_U32->delayOutput("10 ns");
//    stat1_U32->delayCollection("10 ns"); 
}

simpleStatisticsComponent::simpleStatisticsComponent() :
    Component(-1)
{
    // for serialization only
}

bool simpleStatisticsComponent::Clock1Tick(Cycle_t CycleNum) 
{
    // NOTE: THIS IS THE 1NS CLOCK 
//    std::cout << "@ " << CycleNum << std::endl;
    
    rng->nextUniform();
    uint32_t U32 = rng->generateNextUInt32();
    uint64_t U64 = rng->generateNextUInt64();
    int32_t  I32 = rng->generateNextInt32();
    int64_t  I64 = rng->generateNextInt64();
    rng_count++;
    
    // Scale the data 
    uint32_t scaled_U32 = U32 / 10000000;
    uint64_t scaled_U64 = U64 / 1000000000000000;
    int32_t  scaled_I32 = I32 / 10000000;
    int64_t  scaled_I64 = I64 / 1000000000000000;

//    std::cout << "Raw Random: " << rng_count << " of " << rng_max_count << ": " <<
//    rng << ", U32 = " << U32 << ", U64 = " << U64 << ", I32 = " << I32 << ", I64 = " << I64 << std::endl;

//    std::cout << "Scaled Random: " << rng_count << " of " << rng_max_count << ": " <<
//    rng << ", U32 = " << scaled_U32 << ", U64 = " << scaled_U64 << ", I32 = " << scaled_I32 << ", I64 = " << scaled_I64 << std::endl;
    
    // Add the Statistic Data
    stat1_U32->addData(scaled_U32);
    stat2_U64->addData(scaled_U64);
    stat3_I32->addData(scaled_I32);
    stat4_I64->addData(scaled_I64);
    stat5_U32->addData(scaled_U32);
    stat6_U64->addData(scaled_U64);
    
    
////////////////////////////////////////////////////////    
   
    if (CycleNum == 60) {
        std::cout << "@ " << CycleNum << std::endl;
        std::cout << "*** STATISTIC TESTING: DISABLE stat1_U32 OUTPUT FOR 15ns PERIOD" << std::endl;
        stat1_U32->delayOutput("15 ns");
    }
    
////////////////////////////////////////////////////////    
    
    // return false so we keep going or true to stop
    if(rng_count >= rng_max_count) {
        primaryComponentOKToEndSim();
        return true;
    } else {
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

// Serialization

BOOST_CLASS_EXPORT(simpleStatisticsComponent)


