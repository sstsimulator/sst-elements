// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_SST_MEM_H_REQUEST_GEN_EVENT
#define _H_SST_MEM_H_REQUEST_GEN_EVENT

#include <cstdint>
#include <deque>
#include <string>
#include <utility>

#include <sst/core/event.h>
#include <sst/core/params.h>

namespace SST {
namespace Miranda {

class MirandaReqEvent : public SST::Event {
public:
    MirandaReqEvent() = default;

    MirandaReqEvent(uint64_t key,
                    const std::deque<std::pair<std::string,SST::Params>>& gens) :
        key(key), generators(gens)
    {}

    uint64_t key;
    std::deque<std::pair<std::string,SST::Params>> generators;

private:
    // serialize in declaration order, calling base class first
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        Event::serialize_order(ser);
        SST_SER(key);
        SST_SER(generators);
    }

    ImplementSerializable(SST::Miranda::MirandaReqEvent);
};

class MirandaRspEvent : public SST::Event {
public:
    MirandaRspEvent() = default;

    explicit MirandaRspEvent(uint64_t key) : key(key) {}

    uint64_t key;

private:
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        Event::serialize_order(ser);
        SST_SER(key);
    }

    ImplementSerializable(SST::Miranda::MirandaRspEvent);
};

} // namespace Miranda
} // namespace SST

#endif // _H_SST_MIRANDA_EVENT
