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
#include "distCarWash.h"

#include <sst/core/output.h>

using namespace SST;
using namespace SST::DistributedCarWash;

/*
 * Create a distributed car wash component
 * This component manages the carwash only
 * It is intended to be connected to distCarGenerator which will send it cars to wash
 * Tasks for this component
 * - When a car arrives, put it in line
 * - When a bay is free, put the car in the head of the line in the bay (if possible)
 * 		- large cars in large bays
 * 		- small cars in large or small bays
 * - Move cars out of the wash bays when they are clean
 * - Close up shop at closing time
 */
distCarWash::distCarWash(ComponentId_t id, Params& params) :
  Component(id)
{
    // See if any optional simulation parameters were set in the input configuration (Python script, JSON file, etc.)
    simulation_time_ = params.find<int64_t>("time", 1440); 	// 24h * 60m/h = 1440m in a day

    // Configure SST Output object to print output up to verbosity 1 to STDOUT
    out_.init("", 0, 1, Output::STDOUT);

    out_.output("\n");
    out_.output("Welcome to Uncle Moe's Fantastic, Awesome, and Incredible Carwash!\n\n");
    out_.output("The car wash will operate for %" PRIu64 " minutes\n\n", simulation_time_);

	// In SST, clocks are regularly occurring events and they can be disabled and enabled throughout simulation.
	// Instead of waking up every minute to decide if we need to do something
	// as the simpleCarWash did, this one will:
	// * Keep track of time in minutes by default
	minute_tc_ = getTimeConverter("60s");
	setDefaultTimeBase(minute_tc_);

	// Cars arrive on the car link
	car_link_ = configureLink("car", new SST::Event::Handler2<distCarWash, &distCarWash::queueACar>(this));

	// * Wake up every hour to do the hourly report
	registerClock("3600s", new SST::Clock::Handler2<distCarWash, &distCarWash::report>(this));

	// * Use a self-link (internal link) to wakeup whenever a car is done washing
	bay_link_ = configureSelfLink("bay", minute_tc_, new SST::Event::Handler2<distCarWash, &distCarWash::cycleWashBays>(this));
	last_wakeup_time_ = 0; // Keep track of when we wake up

    // Tell the simulator that this component will tell it when to end the simulation
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

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


// SST calls setup() when the simulation is about to begin
// It is the first opportunity to send simulation events on links
void distCarWash::setup()
{
	// Wakeup the simulation after 'simulation_time_' minutes
	// This ensures that this component wakes up at closing time so that it can
	// close down the car wash and end simulation
	scheduleWakeup(simulation_time_);
}


// SST calls finish() when the simulation completes
void distCarWash::finish()
{
	log_.large_cars_washed += hourly_log_.large_cars_washed;
	log_.small_cars_washed += hourly_log_.small_cars_washed;
	log_.time_in_line += hourly_log_.time_in_line;
	log_.cars_arrived += hourly_log_.cars_arrived;
	SimTime_t end_time = getCurrentSimTime(minute_tc_);
	out_.output("\n\n------------------------------------------------------------------\n");
	out_.output("Uncle Moe's Carwash Simulation has completed!\n");
	out_.output("Here is a summary of the results:\n");
	out_.output("Simulation duration was: %" PRId64 " minutes (~ %" PRId64 " hours) of operation\n", end_time, end_time / 60);
	out_.output("Closing was delayed by %" PRId64 " minutes due to cars in the wash bays\n", end_time - simulation_time_);
	out_.output(" Small Cars Washed: %d\n", log_.small_cars_washed);
	out_.output(" Large Cars Washed: %d\n", log_.large_cars_washed);
	out_.output(" Total Cars Washed: %d\n", log_.small_cars_washed + log_.large_cars_washed);
	out_.output(" Number of cars that arrived: %d\n", log_.cars_arrived);
	out_.output(" Total time spent by cars waiting for a wash: %" PRIu64 " minutes\n", log_.time_in_line);
	out_.output(" Average time spent waiting per car: %" PRIu64 " minutes\n", log_.time_in_line / (log_.small_cars_washed + log_.large_cars_washed));
	out_.output("------------------------------------------------------------------\n");

    showDisappointedCustomers();
} // End finish()


// A clock function that is called hourly to produce a report
bool distCarWash::report( Cycle_t tick )
{
	out_.output("------------------------------------------------------------------\n");
	out_.output("Time = %" PRIu64 " (hour). Another simulated hour has passed! Summary:\n", tick);
	out_.output(" Small Cars Washed = %d\n", hourly_log_.small_cars_washed);
	out_.output(" Large Cars Washed = %d\n", hourly_log_.large_cars_washed);
	out_.output(" Average wait this hour = %" PRIu64 " minutes\n",
		hourly_log_.time_in_line / (hourly_log_.large_cars_washed + hourly_log_.small_cars_washed));
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
	return false;
}


// Add a car to the queue
// SST calls this when a CarEvent arrives on car_link_
void distCarWash::queueACar(SST::Event* ev)
{
	// Static_cast is a lot faster than dynamic
	CarEvent * car = static_cast<CarEvent*>(ev);

	hourly_log_.cars_arrived++;

	// Queue entry contains:
	// 	time - time that the car was enqueued for tracking how long it waits in line
	//  wash_times_ - how long it takes to wash this type of car
	//  car - type of car (small, large)
    CarQueueEntry entry {car->time_, wash_times_[(int)car->size_], car->size_ };
    queue_.push(entry);

	// If this is the only car in the queue, check if it can be washed
	// The bays may be empty and able to accept a car
	if (queue_.size() == 1) {
		cycleWashBays(nullptr);
	}

	delete car; // Components are responsible for memory management of events they receive
} // End queueACar()


// Move cars in and out of bays
// This is called when an event arrives on bay_link_
// IMPORTANT: We send 'nullptr' events on bay_link_ since
//   we don't need any event data, just a wakeup. So, this
//   function will not delete ev (it will always be nullptr).
void distCarWash::cycleWashBays(SST::Event* ev)
{
	SimTime_t time_min = getCurrentSimTime(minute_tc_);
	SimTime_t elapsed_time = time_min - last_wakeup_time_;

	// In simpleCarWash, run() always ran 1 min
	// Now, we run for however many minutes passed since
	// the last wakeup
	large_bay_.run(elapsed_time);
	small_bay_.run(elapsed_time);

	// If it's closing time, start closing up and stop accepting
	// new customers into the bays
	if ( time_min >= simulation_time_ ) {
		closingTime();
		return;
	}

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

	// Cars are waiting, try to assign them to a bay
	// A large car might block a small car if it needs to wait for the large bay!
	//
	// If we put a car in a bay, remove it from the queue and send ourselves a wakeup when the car is scheduled
	// to finish washing
	while ( !queue_.empty() ) { // Loop until the car at the front of the queue can't be put into an available bay

		// Handle a large car at the front of the queue
		if ( queue_.front().car == CarType::Large ) {
			if ( large_bay_.occupied != CarType::None ) break; // The Large wash bay is occupied, cars must wait

			// Wash the large car
			large_bay_.occupied = CarType::Large;
			large_bay_.time_left = queue_.front().wash_time;
			hourly_log_.time_in_line += (time_min - queue_.front().queue_time); // Record how long the car waited
			queue_.pop();
			scheduleWakeup(large_bay_.time_left); // Wakeup and try again when the wash is done

		// Handle a small car at the front of the queue
		} else if ( queue_.front().car == CarType::Small ) {
			if ( small_bay_.occupied == CarType::None ) { // Put the small car in the small bay
				small_bay_.occupied = CarType::Small;
				small_bay_.time_left = queue_.front().wash_time;
				hourly_log_.time_in_line += (time_min - queue_.front().queue_time); // Record how long the car waited
				queue_.pop();
				scheduleWakeup(small_bay_.time_left);	// Wakeup and try again when the wash is done
			} else if ( large_bay_.occupied == CarType::None ) { // Put the small car in the large bay
				large_bay_.occupied = CarType::Small;
				large_bay_.time_left = queue_.front().wash_time;
				hourly_log_.time_in_line += (time_min - queue_.front().queue_time); // Record how long the car waited
				queue_.pop();
				scheduleWakeup(large_bay_.time_left);	// Wakeup and try again when the wash is done
			} else { // No free bays, wait
				break;
			}
		} // End handle small car

	} // End while loop

	// Update last wakeup time
	last_wakeup_time_ = time_min;
} // End cycleWashBays()


// Schedule a wakeup to this component by sending
// an empty (nullptr) event on our self link 'bay_link_'
// The wakeup will occur after 'time' cycles and cause
// cycleWashBays() to execute
void distCarWash::scheduleWakeup(Cycle_t time)
{
	bay_link_->send(time, nullptr);
} // End scheduleWakeup()


// Closing time - check whether the carwash is ready to close
// Cannot close if there are cars currently in the bays!
// Change from simpleCarWash: we don't need to return whether the carwash is ready to close
void distCarWash::closingTime()
{
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
	} else {
		out_.output("It is closing time but there are cars still being washed, let them finish\n");
	}
} // End closingTime()


// When the carwash closes, all waiting customers are turned away
void distCarWash::showDisappointedCustomers()
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
void distCarWash::showCarArt()
{
	// Queues aren't iterable so copy and use the copy to destructively inspect the queue
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
