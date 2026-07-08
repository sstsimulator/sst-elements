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

class EmberNetworkIOReadAtEvent : public EmberNetworkIOEvent {
public:
    EmberNetworkIOReadAtEvent(NetworkIO::Interface& api, Output* output,
                              NetworkIO::FileHandle handle,
                              Hermes::MemAddr dest, uint64_t offset,
                              uint64_t length,
                              EmberEventTimeStatistic* stat = nullptr) :
        EmberNetworkIOEvent(api, output, stat),
        m_handle(handle), m_dest(dest), m_offset(offset), m_length(length)
    {}

    ~EmberNetworkIOReadAtEvent() {}

    std::string getName() { return "NetworkIOReadAt"; }

    virtual void issue(uint64_t time, Callback callback) {
        EmberEvent::issue(time);
        auto cb = [callback](NetworkIO::IOStatus st) {
            callback(st.error);
        };
        m_api.read_at(m_handle, m_offset, m_dest.getSimVAddr(), m_length, cb);
    }

    virtual bool complete(uint64_t time, int retval = 0) {
        if (retval != 0) {
            static const bool s_allow =
                std::getenv("SST_EMBER_ALLOW_NETWORKIO_DENY") != nullptr;
            assert(s_allow &&
                   "EmberNetworkIOReadAtEvent::complete: read_at returned "
                   "non-OK IOStatus.error. Set SST_EMBER_ALLOW_NETWORKIO_DENY "
                   "to override for short-read/deny tests.");
        }
        return EmberEvent::complete(time, retval);
    }

private:
    NetworkIO::FileHandle m_handle;
    Hermes::MemAddr m_dest;
    uint64_t m_offset;
    uint64_t m_length;
};

}
}
