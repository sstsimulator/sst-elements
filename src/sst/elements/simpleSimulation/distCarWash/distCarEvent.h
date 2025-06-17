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

#ifndef _UNCLEMOESCARWASH_EVENT_H
#define _UNCLEMOESCARWASH_EVENT_H

#include <sst/core/event.h>

namespace SST::DistributedCarWash {

// The car wash can handle "Small" and "Large" cars
// "None" is included as a convenience for the random number generation
enum class CarType { None, Small, Large };

// This class defines an event for the car wash
// Each event is a car arriving at the wash
// The event has two member variables
//  size_   : The size of the car (Small or Large)
//  time_   : The time the car was generated in simulated minutes
class CarEvent : public SST::Event
{
public:

    // Construct the event
    CarEvent(CarType size, Cycle_t time) : SST::Event(), size_(size), time_(time) { }

    // A default constructor, SST will use this for serialization
    CarEvent() : SST::Event() { }

    // Class variables
    CarType size_; // Size of the car (small or large)
    Cycle_t time_; // Time that the car was generated

    // Events must provide a serialization function that serializes
    // all data members of the event
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        SST_SER(size_);
        SST_SER(time_);
    }

    // Register this event as serializable
    ImplementSerializable(SST::DistributedCarWash::CarEvent);
};



} // namespace DistributedCarWash

#endif // _UNCLEMOESCARWASH_EVENT_H