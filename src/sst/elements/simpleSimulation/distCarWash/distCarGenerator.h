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

#ifndef _UNCLEMOESCARWASH_GENERATOR_H
#define _UNCLEMOESCARWASH_GENERATOR_H

#include <sst/core/component.h>
#include <sst/core/rng/marsaglia.h>

#include "distCarEvent.h"

#include <array>
#include <queue>

namespace SST::DistributedCarWash {

class distCarGenerator : public SST::Component
{
public:

/******** SST Registration and documentation macros *********/

SST_ELI_REGISTER_COMPONENT(
    distCarGenerator,
    "simpleSimulation",
    "distCarGenerator",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "Demo Component: A car generator for a car wash. Connect this to distCarWash.",
    COMPONENT_CATEGORY_UNCATEGORIZED
)

SST_ELI_DOCUMENT_PARAMS(
    { "arrival_frequency", "How often (in minutes) to attempt to generate a car for the queue", "3"},
    { "random_seed", "A seed to use with the random number generator that generates a queue of cars", "151515"}
)

SST_ELI_DOCUMENT_PORTS(
    { "car", "Car events will be sent out of this port", { "simpleSimulation.CarEvent" } }
)

/******** Class member functions - public **********/
    // Constructor
    distCarGenerator(SST::ComponentId_t id, SST::Params& params);

    // Inherited functions from "SST::Component".
    // Not used in this example so function definitions are empty
    void setup() override { }
    void init(unsigned int phase) override { }
    void complete(unsigned int phase) override { }
    void finish() override { }

private:

/******** Class member functions - private **********/
    // The clock handler
    bool tick(SST::Cycle_t);

    // Determine if a car arrived and if so, which kind
    CarType checkForNewCustomer();

    // A handler for events arriving on the car port
    // Won't be used since the carwash doesn't send any events
    void linkHandler(SST::Event* ev);

/******** Class member variables - private **********/

    // Random number generator to randomize car arrival
    SST::RNG::MarsagliaRNG* rng_;

    // An SST Output object to print information to stdout
    SST::Output out_;

    // The link to the carwash
    SST::Link* link_;

    // We are measuring time in terms of minutes but may not have a minute clock running
    // This timeconverter will be used to figure out how many minutes have elapsed
    TimeConverter minute_tc_;
};

} // namespace SST::DistributedCarWash

#endif /* _UNCLEMOESCARWASH_GENERATOR_H */
