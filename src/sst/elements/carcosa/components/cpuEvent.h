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

#ifndef CARCOSA_CPUEVENT_H
#define CARCOSA_CPUEVENT_H

#include <sst/core/event.h>
#include "sst/elements/memHierarchy/memTypes.h"

namespace SST {
namespace Carcosa {

class CpuEvent : public SST::Event {
public:
    CpuEvent() : SST::Event(), fname_(""), startAddr_(0), endAddr_(0), str_(""), num_(0) {}
    CpuEvent(const std::string& val) : SST::Event(), fname_(""), startAddr_(0), endAddr_(0), str_(val), num_(0) {}
    CpuEvent(const std::string& sval, unsigned uval) : SST::Event(), fname_(""), startAddr_(0), endAddr_(0), str_(sval), num_(uval) {}
    CpuEvent(unsigned val) : SST::Event(), fname_(""), startAddr_(0), endAddr_(0), str_(""), num_(val) {}
    CpuEvent(const std::string& file, MemHierarchy::Addr start, MemHierarchy::Addr end)
        : SST::Event(), fname_(file), startAddr_(start), endAddr_(end), str_(""), num_(0) {}

    ~CpuEvent() {}

    std::string getStr() const { return str_; }
    unsigned getNum() const { return num_; }
    MemHierarchy::Addr getStart() const { return startAddr_; }
    MemHierarchy::Addr getEnd() const { return endAddr_; }
    std::string getFname() const { return fname_; }

    std::string toString() const override {
        std::stringstream s;
        s << "CpuEvent. String='" << str_ << "' Number='" << num_ << "'";
        return s.str();
    }

    CpuEvent* clone() override {
        return new CpuEvent(*this);
    }

private:
    std::string fname_;
    MemHierarchy::Addr startAddr_;
    MemHierarchy::Addr endAddr_;
    std::string str_;
    unsigned num_;

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(fname_);
        SST_SER(startAddr_);
        SST_SER(endAddr_);
        SST_SER(str_);
        SST_SER(num_);
    }

    ImplementSerializable(SST::Carcosa::CpuEvent);
};

} // namespace Carcosa
} // namespace SST

#endif // CARCOSA_CPUEVENT_H
