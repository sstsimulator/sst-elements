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

#ifndef _BASIC_SIM_LIFECYCLE_EVENT_H
#define _BASIC_SIM_LIFECYCLE_EVENT_H

#include <sst/core/event.h>

namespace SST {
namespace simpleElementExample {

/*
 * Event for the basicSimLifeCycle example
 * Depending on the phase the simulation sometimes sends a string and sometimes an unsigned int
 * The same event is used in all phases to simplify the code
 */

class basicLifeCycleEvent : public SST::Event
{
public:
    // Constructors
    basicLifeCycleEvent() : SST::Event(), str(""), num(0) { } /* This constructor is required for serialization */
    basicLifeCycleEvent(std::string val) : SST::Event(), str(val), num(0) { }
    basicLifeCycleEvent(std::string sval, unsigned uval) : SST::Event(), str(sval), num(uval) { }
    basicLifeCycleEvent(unsigned val) : SST::Event(), str(""), num(val) { }
    
    // Destructor
    ~basicLifeCycleEvent() { }

    std::string getStr() { return str; }
    unsigned getNum() { return num; }

    std::string toString() const override {
        std::stringstream s;
        s << "basicLifeCycleEvent. String='" << str << "' Number='" << num << "'";
        return s.str();
    }

private:
    std::string str;
    unsigned num;

    // Serialization
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & str;
        ser & num;
    }

    // Register this event as serializable
    ImplementSerializable(SST::simpleElementExample::basicLifeCycleEvent);
};

} // namespace simpleElementExample
} // namespace SST

#endif /* _BASIC_SIM_LIFECYCLE_EVENT_H */
