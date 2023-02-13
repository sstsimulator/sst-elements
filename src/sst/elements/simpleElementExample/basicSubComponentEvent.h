// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _BASIC_SUBCOMPONENT_EVENT_H
#define _BASIC_SUBCOMPONENT_EVENT_H

#include <sst/core/event.h>

namespace SST {
namespace simpleElementExample {

/*
 * Event for the basicSubComponent example
 * The event has three fields:
 *  Sender: who sent the event (so the sender knows when it returned to them again)
 *  Number: current value of the computation
 *  Computation: string describing the computation that has been done
 */

class basicSubComponentEvent : public SST::Event
{
public:
    // Constructors
    basicSubComponentEvent() : SST::Event(), sender(""), number(0), computation("0") { } /* This constructor is required for serialization */
    basicSubComponentEvent(std::string s, int n, std::string c) : SST::Event(), sender(s), number(n), computation(c) { }

    // Destructor
    ~basicSubComponentEvent() { }

    // Only need a getter for the sender
    std::string getSender() { return sender; }

    // Getter/setter for number field
    void setNumber(int n) { number = n; }
    int getNumber() { return number; }

    // Getter/setter for the computation field
    void setComputation(std::string s) { computation = s; }
    std::string getComputation() { return computation; }

    // A toString function for debugging if needed
    std::string toString() const override {
        std::stringstream s;
        s << "basicSubComponentEvent. Sender='" << sender;
        s << "' Number='" << number;
        s << "' Computation='" << computation << "'";
        return s.str();
    }

private:
    std::string sender;
    std::string computation;
    int number;

    // Serialization
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & sender;
        ser & number;
        ser & computation;
    }

    // Register this event as serializable
    ImplementSerializable(SST::simpleElementExample::basicSubComponentEvent);
};

} // namespace simpleElementExample
} // namespace SST

#endif /* _BASIC_SUBCOMPONENT_EVENT_H */
