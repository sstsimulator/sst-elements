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

#ifndef _UNCLEMOESCARWASH_H
#define _UNCLEMOESCARWASH_H

#include <sst/core/component.h>
#include "distCarEvent.h"

#include <array>
#include <queue>

#define WASH_BAY_EMPTY 0
#define WASH_BAY_FULL 1
#define MINUTES_IN_A_DAY 24*60

namespace SST::DistributedCarWash {

class distCarWash : public SST::Component
{
public:

/******** SST Registration and documentation macros *********/

SST_ELI_REGISTER_COMPONENT(
    distCarWash,
    "simpleSimulation",
    "distCarWash",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "Demo Component: A car wash that needs to be connected to a car generator",
    COMPONENT_CATEGORY_UNCATEGORIZED
)

SST_ELI_DOCUMENT_PARAMS(
    { "time", "Number of minutes the car wash should stay open, default is 24h (1440 min)", "1440" },
)

SST_ELI_DOCUMENT_PORTS(
    { "car", "Car events will be sent out of this port", { "simpleSimulation.CarEvent" } }
)

/******** Class member functions - public **********/
    // Constructor
    distCarWash(SST::ComponentId_t id, SST::Params& params);

    // Inherited functions from "SST::Component".
    // Not used in this example so function definitions are empty
    void init(unsigned int phase) override { }
    void complete(unsigned int phase) override { }

    // Inherited function from SST::Component
    // Will use this function to set up a wakeup for the carwash's closing time
    void setup() override;
    // Will use this function to print a simulation summary when the simulation finishes
    void finish() override;

private:
/******** Types used in this class ******************/

    // Entry in the queue of cars waiting to be washed
    struct CarQueueEntry {
        Cycle_t queue_time; // Time that the car entered the queue
        Cycle_t wash_time;  // How much time has the car been in the wash booth?
        CarType car;        // Type of car being washed
    };

    struct WashBay {
        CarType occupied; // What kind of car currently occupies the wash bay
        Cycle_t time_left; // How much longer the car needs to stay in the bay

        void run(SimTime_t elapsed) {
            if ( occupied != CarType::None )  {
                if ( time_left >= elapsed ) {
                    time_left -= elapsed;
                } else {
                    time_left = 0;
                }
            }
        }

        bool isWashFinished() {
            if ( occupied == CarType::None )
                return false; // No car was washed
            return time_left == 0; // Done if time_left is 0
        }
    };

/******** Class member functions - private **********/
    // The clock handler for hourly reports
    bool report(SST::Cycle_t);

    // Enqueue a car to wait for a wash bay
    // The signature has changed from simpleCarWash so that
    // queueACar is an event handler instead of a function called
    // by the clock
    void queueACar(SST::Event* ev);

    // Run the wash bays
    // The signature has changed from simpleCarWash so that
    // cycleWashBays is an event handler instead of a function called
    // by the clock
    void cycleWashBays(SST::Event* ev);

    // If it's closing time, check if we can close
    void closingTime();

    // Show how many customers were turned away when the carwash closed
    void showDisappointedCustomers();

    // Pretty print the queue
    void showCarArt();

/******** Class member variables - private **********/

    // Wash time for each CarType in minutes
    const std::array<Cycle_t, 3> wash_times_{ 0, 3, 5 };

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

    // A wash bay for large and small cars
    WashBay large_bay_;

    // A wash bay for small cars only
    WashBay small_bay_;

    // Queue of waiting cars
    std::queue<CarQueueEntry> queue_;

    // Link to car generator
    SST::Link* car_link_;

    // Internal link to wakeup when it's time to cycle cars through the bays
    SST::Link* bay_link_;

    // We are measuring time in terms of minutes but don't have a minute clock running
    // This timeconverter will be used to figure out how many minutes have elapsed
    TimeConverter minute_tc_;

    // We'll be skipping through time, waking up when we need to instead of every minute
    // We will need to know how many minutes have elapsed since we last woke up
    SimTime_t last_wakeup_time_;
};

} // namespace SST::DistributedCarWash

#endif /* _UNCLEMOESCARWASH_H */
