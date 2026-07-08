// Copyright 2013-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#pragma once 

#include "emberNetworkIOEvent.h"
#include <cstdlib>

namespace SST {
namespace Ember {

class EmberNetworkIOWriteEvent : public EmberNetworkIOEvent {
public:
    EmberNetworkIOWriteEvent(NetworkIO::Interface& api, Output* output, uint64_t offset,
                           Hermes::MemAddr src, uint32_t length,
                           EmberEventTimeStatistic* stat = nullptr) :
        EmberNetworkIOEvent(api, output, stat),
        m_offset(offset), m_src(src), m_length(length)
    {}

    ~EmberNetworkIOWriteEvent() {}

    std::string getName() { return "NetworkIOWrite"; }

    virtual void issue(uint64_t time, Callback callback) {
        EmberEvent::issue(time);
        m_api.networkIOWrite(m_offset, m_src.getSimVAddr(), m_length, callback);
    }

    // PR 7 step (e): assert success Status on completion unless the
    // environment variable SST_EMBER_ALLOW_NETWORKIO_DENY is set (PR 8
    // deny-path testsuite uses this to exercise AckDeny without crashing).
    virtual bool complete(uint64_t time, int retval = 0) {
        if (retval != 0) {
            static const bool s_allow =
                std::getenv("SST_EMBER_ALLOW_NETWORKIO_DENY") != nullptr;
            assert(s_allow &&
                   "EmberNetworkIOWriteEvent::complete: NetworkIO write was "
                   "denied (Status::PermissionDenied). Check pool config or "
                   "set SST_EMBER_ALLOW_NETWORKIO_DENY=1 for deny tests.");
        }
        return EmberEvent::complete(time, retval);
    }

private:
    uint64_t m_offset;
    Hermes::MemAddr m_src;
    uint32_t m_length;
};

}
}
