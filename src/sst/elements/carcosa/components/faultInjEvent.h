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

#ifndef CARCOSA_FAULTINJEVENT_H
#define CARCOSA_FAULTINJEVENT_H

#include <sst/core/event.h>

namespace SST {
namespace Carcosa {

/**
 * Fault Injection Event for passing fault injection parameters
 * between CPU and Hali components.
 */
class FaultInjEvent : public SST::Event {
public:
    FaultInjEvent() : SST::Event(), fname_(""), probability_(0.0f), rate_(0.0f), str_(""), num_(0) {}
    FaultInjEvent(const std::string& val) : SST::Event(), fname_(""), probability_(0.0f), rate_(0.0f), str_(val), num_(0) {}
    FaultInjEvent(const std::string& sval, unsigned uval) : SST::Event(), fname_(""), probability_(0.0f), rate_(0.0f), str_(sval), num_(uval) {}
    FaultInjEvent(unsigned val) : SST::Event(), fname_(""), probability_(0.0f), rate_(0.0f), str_(""), num_(val) {}
    FaultInjEvent(const std::string& file, float prob, float r)
        : SST::Event(), fname_(file), probability_(prob), rate_(r), str_(""), num_(0) {}

    ~FaultInjEvent() {}

    std::string getStr() const { return str_; }
    unsigned getNum() const { return num_; }
    float getProbability() const { return probability_; }
    float getRate() const { return rate_; }
    std::string getFname() const { return fname_; }

    std::string toString() const override {
        std::stringstream s;
        s << "FaultInjEvent. String='" << str_ << "' Number='" << num_
          << "' Probability='" << probability_ << "' Rate='" << rate_ << "'";
        return s.str();
    }

    FaultInjEvent* clone() override {
        return new FaultInjEvent(*this);
    }

private:
    std::string fname_;
    float probability_;
    float rate_;
    std::string str_;
    unsigned num_;

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(fname_);
        SST_SER(probability_);
        SST_SER(rate_);
        SST_SER(str_);
        SST_SER(num_);
    }

    ImplementSerializable(SST::Carcosa::FaultInjEvent);
};

} // namespace Carcosa
} // namespace SST

#endif // CARCOSA_FAULTINJEVENT_H
