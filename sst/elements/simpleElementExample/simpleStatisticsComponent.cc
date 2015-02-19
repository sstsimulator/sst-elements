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
#include "sst/core/serialization.h"
#include "sst/core/element.h"
#include "sst/core/params.h"

#include "simpleStatisticsComponent.h"

using namespace SST;
using namespace SST::RNG;
using namespace SST::SimpleStatisticsComponent;

simpleStatisticsComponent::simpleStatisticsComponent(ComponentId_t id, Params& params) : Component(id)
{
    // Get runtime parameters from the input file (.py),  
    // these will set the random number generation.
    rng_count = 0;
    rng_max_count = params.find_integer("count", 1000);

    if( params.find("rng") != params.end() ) {
        rng_type = params["rng"];
 
        if( params["rng"] == "mersenne" ) {
            std::cout << "Using Mersenne Random Number Generator..." << std::endl;
            unsigned int seed = 1447;
     
            if( params.find("seed") != params.end() ) {
                seed = params.find_integer("seed");
            }
     
            rng = new MersenneRNG(seed);
 	  	} else if ( params["rng"] == "marsaglia" ) {
 	  	    std::cout << "Using Marsaglia Random Number Generator..." << std::endl;
 
            unsigned int m_z = 0;
            unsigned int  m_w = 0;
            
            m_w = params.find_integer("seed_w", 0);
            m_z = params.find_integer("seed_z", 0);
 
            if(m_w == 0 || m_z == 0) {
                rng = new MarsagliaRNG();
 			} else {
 			    rng = new MarsagliaRNG(m_z, m_w);
 			}
   		} else {
   		    std::cout << "RNG provided but unknown " << params["rng"] << ", so using Mersenne..." << std::endl;
   		    rng = new MersenneRNG(1447);
 	    }
 	} else {
 	    std::cout << "No rng parameter provided, so using Mersenne..." << std::endl;
 	    rng = new MersenneRNG(1447);
 	}

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    /////////////////////////////////////////
    // Register the Clocks

    // First 1ns Clock 
    std::cout << "REGISTER CLOCK #1 at 1 ns" << std::endl;
    registerClock("1 ns", new Clock::Handler<simpleStatisticsComponent>(this, &simpleStatisticsComponent::Clock1Tick));
    
    // Second 5ns Clock 
    std::cout << "REGISTER CLOCK #2 at 5 ns" << std::endl;
    registerClock("5 ns", new Clock::Handler<simpleStatisticsComponent, uint32_t>(this, &simpleStatisticsComponent::Clock2Tick, 222));
    
    // Third 15ns Clock
    std::cout << "REGISTER CLOCK #3 at 15 ns" << std::endl;
    Clock3Handler = new Clock::Handler<simpleStatisticsComponent, uint32_t>(this, &simpleStatisticsComponent::Clock3Tick, 333);
    tc = registerClock("15 ns", Clock3Handler);

    /////////////////////////////////////////
    // Create the Statistics objects
    histoU32 = registerStatistic<uint32_t>("histo_U32", "1");
    histoU64 = registerStatistic<uint64_t>("histo_U64", "2");
    histoI32 = registerStatistic<int32_t> ("histo_I32", "3");
    histoI64 = registerStatistic<int64_t> ("histo_I64", "4");
    accumU32 = registerStatistic<uint32_t>("accum_U32", "5");
    accumU64 = registerStatistic<uint64_t>("accum_U64", "6");
    
    // Try to Register Illegal Statistic Objects
    printf("STATISTIC TESTING: TRYING TO REGISTER A DUPLICATE STAT NAME - SHOULD RETURN A NULLSTATISTIC\n");
    accumU32_NOTUSED = registerStatistic<uint32_t>("accum_U32", "1");   // This stat should not be registered because it has a duplicated name

    // Create the OneShot Callback Handlers
    callback1Handler = new OneShot::Handler<simpleStatisticsComponent, uint32_t>(this, &simpleStatisticsComponent::Oneshot1Callback, 1);
    callback2Handler = new OneShot::Handler<simpleStatisticsComponent>(this, &simpleStatisticsComponent::Oneshot2Callback);
    
    // Test Statistic functions for delayed output and collection and to disable Stat
//    histoU32->disable();
//    histoU32->delayOutput("10 ns");
//    histoU32->delayCollection("10 ns"); 
}

simpleStatisticsComponent::simpleStatisticsComponent() :
    Component(-1)
{
    // for serialization only
}

bool simpleStatisticsComponent::Clock1Tick(Cycle_t CycleNum) 
{
    // NOTE: THIS IS THE 1NS CLOCK 
    std::cout << "@ " << CycleNum << std::endl;
    
    rng->nextUniform();
    uint32_t U32 = rng->generateNextUInt32();
    uint64_t U64 = rng->generateNextUInt64();
    int32_t I32 = rng->generateNextInt32();
    int64_t I64 = rng->generateNextInt64();
    rng_count++;
    
//    std::cout << "Random: " << rng_count << " of " << rng_max_count << ": " <<
//    nU << ", " << U32 << ", " << U64 << ", " << I32 <<
//    ", " << I64 << std::endl;
    
    // Save the Histogram Data
    histoU32->addData(U32);
    histoU64->addData(U64);
    histoI32->addData(I32);
    histoI64->addData(I64);
    accumU32->addData(U32);
    accumU64->addData(U64);
    
    
////////////////////////////////////////////////////////    
   
    if (CycleNum == 60) {
        std::cout << "*** STATISTIC TESTING: DISABLE histoU32 OUTPUT FOR 15ns PERIOD" << std::endl;
        histoU32->delayOutput("15 ns");
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


bool simpleStatisticsComponent::Clock2Tick(SST::Cycle_t CycleNum, uint32_t Param)
{
    // NOTE: THIS IS THE 5NS CLOCK 
    std::cout << "  CLOCK #2 - TICK Num " << CycleNum << "; Param = " << Param << std::endl;

    // return false so we keep going or true to stop
    if(rng_count >= rng_max_count) {
    	return true;
    } else {
    	return false;
    }
}
    
bool simpleStatisticsComponent::Clock3Tick(SST::Cycle_t CycleNum, uint32_t Param)
{
    // NOTE: THIS IS THE 15NS CLOCK 
    std::cout << "  CLOCK #3 - TICK Num " << CycleNum << "; Param = " << Param << std::endl;    
    
    if ((CycleNum == 1) || (CycleNum == 4))  {

      std::cout << "*** STATISTIC TESTING: REGISTERING ONESHOTS " << std::endl ;
      registerOneShot("10ns", callback1Handler);
      registerOneShot("18ns", callback2Handler);
    }
    
    // return false so we keep going or true to stop
    if(rng_count >= rng_max_count) {
    	return true;
    } else {
    	return false;
    }
}

void simpleStatisticsComponent::Oneshot1Callback(uint32_t Param)
{
    std::cout << "STATISTIC TESTING: --- ONESHOT #1 CALLBACK; Param = " << Param << std::endl;
}
    
void simpleStatisticsComponent::Oneshot2Callback()
{
    std::cout << "STATISTIC TESTING: --- ONESHOT #2 CALLBACK" << std::endl;
}
    
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

// Serialization

BOOST_CLASS_EXPORT(simpleStatisticsComponent)


