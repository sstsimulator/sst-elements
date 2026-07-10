// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
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
#include <cstdint>
#include <utility>
#include <vector>

namespace SST {
namespace Carcosa {

/**
 * Ring event: tag + num (+ optional opaque Cmd payload; partners agree on layout).
 */
class HaliEvent : public SST::Event {
public:
    HaliEvent() : SST::Event(), str_(""), num_(0) {}
    HaliEvent(const std::string& val) : SST::Event(), str_(val), num_(0) {}
    HaliEvent(const std::string& sval, unsigned uval) : SST::Event(), str_(sval), num_(uval) {}
    HaliEvent(const std::string& sval, unsigned uval, std::vector<uint8_t> payload)
        : SST::Event(), str_(sval), num_(uval), payload_(std::move(payload)) {}
    HaliEvent(unsigned val) : SST::Event(), str_(""), num_(val) {}

    ~HaliEvent() {}

    std::string getStr() const { return str_; }
    unsigned getNum() const { return num_; }

    bool hasPayload() const { return !payload_.empty(); }
    const std::vector<uint8_t>& getPayload() const { return payload_; }
    void setPayload(std::vector<uint8_t> payload) { payload_ = std::move(payload); }

    std::string toString() const override {
        std::stringstream s;
        s << "HaliEvent. String='" << str_ << "' Number='" << num_
          << "' PayloadBytes='" << payload_.size() << "'";
        return s.str();
    }

    HaliEvent* clone() override {
        return new HaliEvent(*this);
    }

private:
    std::string str_;
    unsigned num_;
    std::vector<uint8_t> payload_;

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(str_);
        SST_SER(num_);
        SST_SER(payload_);
    }

    ImplementSerializable(SST::Carcosa::HaliEvent);
};

} // namespace Carcosa
} // namespace SST

#endif // CARCOSA_HALIEVENT_H
