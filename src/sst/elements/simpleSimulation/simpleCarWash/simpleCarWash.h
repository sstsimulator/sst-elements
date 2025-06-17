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

#ifndef _UNCLEEDSCARWASH_H
#define _UNCLEEDSCARWASH_H

#include <sst/core/component.h>
#include <sst/core/rng/marsaglia.h>

#include <array>
#include <queue>

#define WASH_BAY_EMPTY 0
#define WASH_BAY_FULL 1
#define MINUTES_IN_A_DAY 24*60

namespace SST::SimpleCarWash {

class simpleCarWash : public SST::Component
{
public:

/******** SST Registration and documentation macros *********/

SST_ELI_REGISTER_COMPONENT(
    simpleCarWash,
    "simpleSimulation",
    "simpleCarWash",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "Demo Component: A car wash designed modeled as a single self-contained clocked component",
    COMPONENT_CATEGORY_UNCATEGORIZED
)

SST_ELI_DOCUMENT_PARAMS(
    { "time", "Number of minutes to execute, default is 24h (1440 min)", "1440" },
    { "arrival_frequency", "How often (in minutes) to attempt to generate a car for the queue", "3"},
    { "random_seed", "A seed to use with the random number generator that generates a queue of cars", "151515"}
)

/******** Class member functions - public **********/
    // Constructor
    simpleCarWash(SST::ComponentId_t id, SST::Params& params);

    // Inherited functions from "SST::Component".
    // Not used in this example so function definitions are empty
    void setup() override { }
    void init(unsigned int phase) override { }
    void complete(unsigned int phase) override { }

    // Inherited function from SST::Component
    // Will use this function to print a simulation summary when the simulation finishes
    void finish() override;

private:
/******** Types used in this class ******************/

    // The car wash can handle "Small" and "Large" cars
    enum class CarType { None, Small, Large };

    // Entry in the queue of cars waiting to be washed
    struct CarQueueEntry {
        Cycle_t queue_time; // Time that the car entered the queue
        Cycle_t wash_time;  // How much time has the car been in the wash booth?
        CarType car;        // Type of car being washed
    };

    struct WashBay {
        CarType occupied; // What kind of car currently occupies the wash bay
        Cycle_t time_left; // How much longer the car needs to stay in the bay

        void run() {
            if (occupied != CarType::None)
                time_left--;
        }

        bool isWashFinished() {
            if ( occupied == CarType::None )
                return false; // No car was washed
            return time_left == 0; // Done if time_left is 0
        }
    };

/******** Class member functions - private **********/
    // Default constructor - used for serialization only
    simpleCarWash() {}

    // The clock handler
    bool tick(SST::Cycle_t);

    // Determine if a car arrived and if so, which kind
    CarType checkForNewCustomer();

    // Enqueue a car to wait for a wash bay
    void queueACar(CarType carSize, Cycle_t time);

    // Run the wash bays
    void cycleWashBays(Cycle_t time);

    // If it's closing time, check if we can close
    bool closingTime();

    // Show how many customers were turned away when the carwash closed
    void showDisappointedCustomers();

    // Pretty print the queue
    void showCarArt();

/******** Class member variables - private **********/

    // Wash time for each CarType in minutes
    const std::array<Cycle_t, 3> wash_times_{ 0, 3, 5 };

    // Random number generator to randomize car arrival
    SST::RNG::MarsagliaRNG* rng_;

    // An SST Output object to print information to stdout
    SST::Output out_;

    // Summary and hourly logs of the car wash operations
    struct OperationLog {
    // Records to manage the "books" for the car wash
        int small_cars_washed;	// Number of small cars washed
        int large_cars_washed;	// Number of large cars washed
        int cars_arrived;       // Number of cars that arrived
        Cycle_t time_in_line;   // Total time that cars spend waiting before being washed
    } log_, hourly_log_;

    // Time the simulation should run for in clock ticks
    int64_t simulation_time_;

    // How often to check for new cars, in clock ticks
    int64_t arrival_frequency_;

    // A wash bay for large and small cars
    WashBay large_bay_;

    // A wash bay for small cars only
    WashBay small_bay_;

    // Queue of waiting cars
    std::queue<CarQueueEntry> queue_;

};

} // namespace SST::SimpleCarWash

#endif /* _UNCLEEDSCARWASH_H */
