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
#include "simpleCarWash.h"

#include <sst/core/output.h>

using namespace SST;
using namespace SST::SimpleCarWash;

/*
 * Create a "simple" car wash component
 * This component does everything itself:
 * - Generate cars to go through the carwash
 * - Decide which cars will go in which bay
 * - Manage the bays and move cars through the stages of washing
 */
simpleCarWash::simpleCarWash(ComponentId_t id, Params& params) :
  Component(id)
{
    int64_t random_seed;
	
    // See if any optional simulation parameters were set in the input configuration (Python script, JSON file, etc.)
    simulation_time_ = params.find<int64_t>("time", 1440); 	// 24h * 60m/h = 1440m in a day
	arrival_frequency_ = params.find<int64_t>("arrival_frequency", 3);
    random_seed = params.find<int64_t>("random_seed", 151515);

    // Configure SST Output object to print output up to verbosity 1 to STDOUT
    out_.init("", 0, 1, Output::STDOUT);

    out_.output("\n\n");
    out_.output("Welcome to Uncle Ed's Fantastic, Awesome, and Incredible Carwash!\n\n");
    out_.output("The carwash clock is configured for a clock period of 60s\n");
    out_.output("Random seed used is: %" PRId64 "\n", random_seed);
    out_.output("The car wash will operate for %" PRId64 " minutes\n\n", simulation_time_);

	registerClock("60s", new SST::Clock::Handler2<simpleCarWash, &simpleCarWash::tick>(this));

    // Tell the simulator that this component will tell it when to end the simulation
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // Register a clock
    // The clock will automatically be called by the SST framework
    // This will allow our component to wake up on a regular basis and check progress or add new cars to the line
    // Clocks are not *strictly* required but some way of moving time forward in time is! 

    rng_ = new SST::RNG::MarsagliaRNG(11,  random_seed);

    // Initialize all of the simulation variables and parameters
    log_.small_cars_washed = 0;	// How many small cars were washed
    log_.large_cars_washed = 0;	// How many large cars were washed
    log_.cars_arrived = 0;		// How many cars arrived
	log_.time_in_line = 0;		// How much total time cars spent waiting in line 
    hourly_log_.small_cars_washed = 0;
    hourly_log_.large_cars_washed = 0;
    hourly_log_.cars_arrived = 0;
	hourly_log_.time_in_line = 0;

	// Initialize bays
	large_bay_.occupied = CarType::None;
	large_bay_.time_left = 0;

	small_bay_.occupied = CarType::None;
	small_bay_.time_left = 0;
    
    out_.output("The carwash simulation is now initialized.\n");
} // End constructor


// SST calls finish() when the simulation completes
void simpleCarWash::finish()
{
	log_.large_cars_washed += hourly_log_.large_cars_washed;
	log_.small_cars_washed += hourly_log_.small_cars_washed;
	log_.time_in_line += hourly_log_.time_in_line;
	log_.cars_arrived += hourly_log_.cars_arrived;
	SimTime_t end_time = getCurrentSimTime("60s");
	out_.output("\n\n------------------------------------------------------------------\n");
	out_.output("Uncle Ed's Carwash Simulation has completed!\n");
	out_.output("Here is a summary of the results:\n");
	out_.output("Simulation duration was: %" PRId64 " minutes (~ %" PRId64 " hours)\n", end_time, end_time / 60);
	out_.output("Closing was delayed by %" PRId64 " minutes due to cars in the wash bays\n", end_time - simulation_time_);
	out_.output(" Small Cars Washed: %d\n", log_.small_cars_washed);
	out_.output(" Large Cars Washed: %d\n", log_.large_cars_washed);
	out_.output(" Total Cars Washed: %d\n", log_.small_cars_washed + log_.large_cars_washed);
	out_.output(" Number of cars that arrived: %d\n", log_.cars_arrived);
	out_.output(" Total time spent by cars waiting for a wash: %" PRIu64 " minutes\n", log_.time_in_line);
	out_.output(" Average time spent waiting per car: %" PRIu64 " minutes\n", 
		log_.time_in_line / (log_.small_cars_washed + log_.large_cars_washed));
	out_.output("------------------------------------------------------------------\n");
	
    showDisappointedCustomers();
} // End finish()


// The clock function - called by SST on each clock cycle
bool simpleCarWash::tick( Cycle_t tick )
{
    CarType car = CarType::None;

    // Announce the top of the hour and give a little summary of where we are at with our carwashing business
    if ( tick != 0 && (tick % 60) == 0 ) {
        out_.output("------------------------------------------------------------------\n");
        out_.output("Time = %" PRIu64 " (minutes). Another simulated hour has passed! Summary:\n", tick);
        out_.output(" Small Cars Washed = %d\n", hourly_log_.small_cars_washed);
        out_.output(" Large Cars Washed = %d\n", hourly_log_.large_cars_washed);
		out_.output(" Average wait this hour = %" PRIu64 " minutes\n", 
			(hourly_log_.time_in_line / (hourly_log_.small_cars_washed + hourly_log_.large_cars_washed)));
        out_.output(" Total Cars Arrived = %d\n", hourly_log_.cars_arrived);
		out_.output(" Number of Cars Waiting = %zu\n\n", queue_.size());

        // showCarArt(); // uncomment to see graphical of cars in queue
        
		// Update summary log and reset hourly log
		log_.large_cars_washed += hourly_log_.large_cars_washed;
		log_.small_cars_washed += hourly_log_.small_cars_washed;
		log_.cars_arrived += hourly_log_.cars_arrived;
		log_.time_in_line += hourly_log_.time_in_line;
		hourly_log_.large_cars_washed = 0;
		hourly_log_.small_cars_washed = 0;
		hourly_log_.cars_arrived = 0;
		hourly_log_.time_in_line = 0;
    }

	// Check if it is time for the car wash to close
	// Probably shouldn't leave any cars in the wash bays so let them finish first...
    if ( tick >= simulation_time_) {
		return closingTime();
    }

	// Otherwise, continue the simulation...

	// Check for new car.
	if ( tick % arrival_frequency_ == 0 ) {
		car = checkForNewCustomer();
	}

	// If a car arrived, put it in the queue
	if (CarType::None != car) {
		hourly_log_.cars_arrived++;
		queueACar(car, tick);
	}

	// Run the car wash for another cycle
	large_bay_.run();
	small_bay_.run();

	// Move any cars that finished washing out of the bays
	// Fill empty bays
	cycleWashBays(tick);
    return false; // Not ready to end yet, keep the clock running
} // End tick()


// Randomly select a car type from {none, small, large} to join the queue
simpleCarWash::CarType simpleCarWash::checkForNewCustomer()
{
    int32_t rndNumber;
    rndNumber = (int)(rng_->generateNextInt32());
    rndNumber = (rndNumber & 0x0000FFFF) ^ ((rndNumber & 0xFFFF0000) >> 16);
    rndNumber = abs((int32_t)(rndNumber % 3));

    return static_cast<CarType>(rndNumber);
} // End checkForNewCustomer()


// Add a car to the queue
void simpleCarWash::queueACar(CarType car, Cycle_t time)
{
	// Entry contains:
	// 	time - time that the car was enqueued for tracking how long it waits in line
	//  wash_times_ - how long it takes to wash this type of car
	//  car - type of car (small, large)
    CarQueueEntry entry {time, wash_times_[(int)car], car };
    queue_.push(entry);
} // End queueACar()


// Move cars in and out of bays
void simpleCarWash::cycleWashBays(Cycle_t time)
{
	// Check if either bay is done
	// If it is, log the finished wash and empty the bay
	if ( true == large_bay_.isWashFinished() ) { // Check large bay
		if ( large_bay_.occupied == CarType::Small) {
			hourly_log_.small_cars_washed++;
		} else {
			hourly_log_.large_cars_washed++;
		}
		large_bay_.occupied = CarType::None;
	} // End check large bay

	if ( true == small_bay_.isWashFinished() ) { // Check small bay
		hourly_log_.small_cars_washed++;
		small_bay_.occupied = CarType::None;
	} // End check small bay

	// If any cars are waiting, try to assign them to a bay
	// A large car might block a small car if it needs to wait for the large bay!
	while (! queue_.empty() ) { // Loop until the car at the front of the queue can't be put into an available bay
		
		// Handle a large car at the front of the queue
		if ( queue_.front().car == CarType::Large ) {
			if ( large_bay_.occupied != CarType::None ) break; // The Large wash bay is occupied, cars must wait

			// Wash the large car
			large_bay_.occupied = CarType::Large;
			large_bay_.time_left = queue_.front().wash_time;
			hourly_log_.time_in_line += (time - queue_.front().queue_time); // Record how long the car waited
			queue_.pop();
		} else if ( queue_.front().car == CarType::Small ) {
		// Handle a small car at the front of the queue
			if ( small_bay_.occupied == CarType::None ) { // Put the small car in the small bay
				small_bay_.occupied = CarType::Small;
				small_bay_.time_left = queue_.front().wash_time;
				hourly_log_.time_in_line += (time - queue_.front().queue_time); // Record how long the car waited
				queue_.pop();	
			} else if ( large_bay_.occupied == CarType::None ) { // Put the small car in the large bay
				large_bay_.occupied = CarType::Small;
				large_bay_.time_left = queue_.front().wash_time;
				hourly_log_.time_in_line += (time - queue_.front().queue_time); // Record how long the car waited
				queue_.pop();	
			} else { // No free bays, wait
				break;
			}
		} // End handle small car
	} // End while loop
} // End cycleWashBays()


// Closing time - return whether the carwash is ready to close
// Cannot close if there are cars currently in the bays!
bool simpleCarWash::closingTime()
{
	// It's closing time, check if we need to stay open just a bit longer
	// to wait for cars currently in the bays to finish
	large_bay_.run();
	small_bay_.run();

	// Check large bay
	if ( true == large_bay_.isWashFinished() ) {
		if ( large_bay_.occupied == CarType::Small) {
			hourly_log_.small_cars_washed++;
		} else {
			hourly_log_.large_cars_washed++;
		}
		large_bay_.occupied = CarType::None;
	}

	// Check small bay
	if ( true == small_bay_.isWashFinished() ) {
		hourly_log_.small_cars_washed++;
		small_bay_.occupied = CarType::None;
	}

	// Return true if both bays are empty, otherwise false (need to stay open longer)
	if ( small_bay_.occupied == CarType::None && large_bay_.occupied == CarType::None ) {
		out_.output("It is closing time and bays are empty so the car wash is ready to close\n");
		primaryComponentOKToEndSim(); // Let SST know the simulation can end
		return true;
	} else {
		out_.output("It is closing time but there are cars still being washed, let them finish\n");
		return false;
	}
} // End closingTime()


// When the carwash closes, all waiting customers are turned away
void simpleCarWash::showDisappointedCustomers()
{
	int small_car_customers = 0;
	int large_car_customers = 0;

	if (queue_.empty()) {
		out_.output("All waiting cars were washed! No disappointed customers!\n");
		return;
	}

	// Empty the queue and count the customers
	while (!queue_.empty()) {
		if (queue_.front().car == CarType::Small) {
			small_car_customers++;
		} else {
			large_car_customers++;
		}
		queue_.pop();
	} 

	out_.output("Disappointed Customers\n");
	out_.output("-----------------------------\n");
	out_.output(" Small Cars: %d\n", small_car_customers);
	out_.output(" Large Cars: %d\n", large_car_customers);
	out_.output(" Total: %d\n", small_car_customers + large_car_customers);
} // End showDisappointedCustomers()


// Print the queue
void simpleCarWash::showCarArt()
{
	// Queues aren't iterable so copy and use the copy to inspect the queue
	std::queue<CarQueueEntry> tmp_queue = queue_;

	while ( !tmp_queue.empty() ) {
		if ( tmp_queue.front().car == CarType::Small ) {
			out_.output("SC: ");
		} else { // CarType::Large
			out_.output("LC: ");
		}
		tmp_queue.pop();
	}
	out_.output("\n");
} // End showCarArt()
