// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "distCarGenerator.h"
#include "distCarEvent.h"

#include <sst/core/output.h>

using namespace SST;
using namespace SST::DistributedCarWash;

/*
 * Create a car generator to send cars to a car wash
 */
distCarGenerator::distCarGenerator(ComponentId_t id, Params& params) :
  Component(id)
{
    int64_t random_seed;
	
    // See if any optional simulation parameters were set in the input configuration (Python script, JSON file, etc.)
    random_seed = params.find<int64_t>("random_seed", 151515);

    // Configure SST Output object to print output up to verbosity 1 to STDOUT
    out_.init("", 0, 1, Output::STDOUT);

    out_.output("\n\n");
    out_.output("Welcome to the car generator for Uncle Moe's Fantastic, Awesome, and Incredible Carwash!\n\n");
    out_.output("Every 60s, the generator will try to generate a car\n");
	out_.output("If a car is generated it will be sent to the connected carwash\n");
	out_.output("Random seed used is: %" PRId64 "\n", random_seed);

	// Register a clock so that this component can try to generate a car on a regular basis
    int arrival_frequency = params.find<int>("arrival_frequency", 3) * 60;
    std::string arrival_frequency_string = std::to_string(arrival_frequency) + "s";
	registerClock(arrival_frequency_string, new SST::Clock::Handler2<distCarGenerator, &distCarGenerator::tick>(this));

    minute_tc_ = getTimeConverter("60s");

	// Configure the link that this component will use to send cars to the carwash
	link_ = configureLink("car", new SST::Event::Handler2<distCarGenerator, &distCarGenerator::linkHandler>(this));
    sst_assert(link_, CALL_INFO, -1, "Error: Is the car generator's 'car' port connected? SST was unable to configure it.\n");

	// This component does not help decide when the car wash closes, so it does not register as primary

	// Create a random number generator for generating cars
    rng_ = new SST::RNG::MarsagliaRNG(11,  random_seed);

    out_.output("The car generator is now initialized and will try to generate a car every %s.\n", 
        arrival_frequency_string.c_str());
} // End constructor


// The clock function - called by SST on each clock cycle
bool distCarGenerator::tick( Cycle_t UNUSED(tick) )
{
    CarType car;

	// Check for new car.
	car = checkForNewCustomer();

	if ( car != CarType::None ) {
        // The generator's clock may not be the same as the car wash's clock 
        // so exchange times in minutes
		CarEvent * ev = new CarEvent( car, getCurrentSimTime(minute_tc_) );
		link_->send(ev);
	}

    return false; // Keep the clock running
} // End tick()


// Randomly select a car type from {none, small, large} to join the queue
CarType distCarGenerator::checkForNewCustomer()
{
    int32_t rndNumber;
    rndNumber = (int)(rng_->generateNextInt32());
    rndNumber = (rndNumber & 0x0000FFFF) ^ ((rndNumber & 0xFFFF0000) >> 16);
    rndNumber = abs((int32_t)(rndNumber % 3));

    return static_cast<CarType>(rndNumber);
} // End checkForNewCustomer()


// A handler for events arriving on our link
// The car generator doesn't use this
void distCarGenerator::linkHandler(SST::Event* ev)
{
	out_.output("Something strange happened, the carwash sent the generator an event! Ignoring it...\n");
	delete ev;
}