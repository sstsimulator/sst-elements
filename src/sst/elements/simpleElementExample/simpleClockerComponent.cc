// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

//#include <assert.h>

#include "sst_config.h"
#include "simpleClockerComponent.h"

namespace SST {
namespace SimpleClockerComponent {

simpleClockerComponent::simpleClockerComponent(ComponentId_t id, Params& params) :
  Component(id) 
{
    clock_frequency_str = params.find<std::string>("clock", "1GHz");
    clock_count = params.find<int64_t>("clockcount", 1000);
    
    std::cout << "Clock is configured for: " << clock_frequency_str << std::endl;
    
    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    //set our Main Clock
    registerClock(clock_frequency_str, new Clock::Handler<simpleClockerComponent>(this,
			      &simpleClockerComponent::tick));
  
    // Set some other clocks
    // Second Clock (5ns) 
    std::cout << "REGISTER CLOCK #2 at 5 ns" << std::endl;
    registerClock("5 ns", new Clock::Handler<simpleClockerComponent, uint32_t>(this, &simpleClockerComponent::Clock2Tick, 222));
    
    // Third Clock (15ns)
    std::cout << "REGISTER CLOCK #3 at 15 ns" << std::endl;
    Clock3Handler = new Clock::Handler<simpleClockerComponent, uint32_t>(this, &simpleClockerComponent::Clock3Tick, 333);
    tc = registerClock("15 ns", Clock3Handler);

    // Create the OneShot Callback Handlers
    callback1Handler = new OneShot::Handler<simpleClockerComponent, uint32_t>(this, &simpleClockerComponent::Oneshot1Callback, 456);
    callback2Handler = new OneShot::Handler<simpleClockerComponent>(this, &simpleClockerComponent::Oneshot2Callback);
}

simpleClockerComponent::simpleClockerComponent() :
    Component(-1)
{
    // for serialization only
}

bool simpleClockerComponent::tick( Cycle_t ) 
{
    clock_count--;
    
    // return false so we keep going
    if(clock_count == 0) {
    primaryComponentOKToEndSim();
        return true;
    } else {
        return false;
    }
}

bool simpleClockerComponent::Clock2Tick(SST::Cycle_t CycleNum, uint32_t Param)
{
    // NOTE: THIS IS THE 5NS CLOCK 
    std::cout << "  CLOCK #2 - TICK Num " << CycleNum << "; Param = " << Param << std::endl;
    
    // return false so we keep going or true to stop
    if (CycleNum == 15) {
        return true;
    } else {
        return false;
    }
}
    
bool simpleClockerComponent::Clock3Tick(SST::Cycle_t CycleNum, uint32_t Param)
{
    // NOTE: THIS IS THE 15NS CLOCK 
    std::cout << "  CLOCK #3 - TICK Num " << CycleNum << "; Param = " << Param << std::endl;    
    
    if ((CycleNum == 1) || (CycleNum == 4))  {
        std::cout << "*** REGISTERING ONESHOTS " << std::endl ;
        registerOneShot("10ns", callback1Handler);
        registerOneShot("18ns", callback2Handler);
    }
    
    // return false so we keep going or true to stop
    if (CycleNum == 15) {
        return true;
    } else {
        return false;
    }
}

void simpleClockerComponent::Oneshot1Callback(uint32_t Param)
{
    std::cout << "-------- ONESHOT #1 CALLBACK; Param = " << Param << std::endl;
}
    
void simpleClockerComponent::Oneshot2Callback()
{
    std::cout << "-------- ONESHOT #2 CALLBACK" << std::endl;
}

// Serialization 
} // namespace simpleClockerComponent
} // namespace SST


