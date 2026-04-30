// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef CARCOSA_HALIEVENT_H
#define CARCOSA_HALIEVENT_H

#include <sst/core/event.h>

namespace SST {
namespace Carcosa {

/**
 * Hali Event for communication between Hali components in a ring.
 */
class HaliEvent : public SST::Event {
public:
    HaliEvent() : SST::Event(), str_(""), num_(0) {}
    HaliEvent(const std::string& val) : SST::Event(), str_(val), num_(0) {}
    HaliEvent(const std::string& sval, unsigned uval) : SST::Event(), str_(sval), num_(uval) {}
    HaliEvent(unsigned val) : SST::Event(), str_(""), num_(val) {}

    ~HaliEvent() {}

    std::string getStr() const { return str_; }
    unsigned getNum() const { return num_; }

    std::string toString() const override {
        std::stringstream s;
        s << "HaliEvent. String='" << str_ << "' Number='" << num_ << "'";
        return s.str();
    }

    HaliEvent* clone() override {
        return new HaliEvent(*this);
    }

private:
    std::string str_;
    unsigned num_;

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(str_);
        SST_SER(num_);
    }

    ImplementSerializable(SST::Carcosa::HaliEvent);
};

} // namespace Carcosa
} // namespace SST

#endif // CARCOSA_HALIEVENT_H
