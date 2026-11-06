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

class EmberNetworkIOWaitEvent : public EmberNetworkIOEvent {
public:
    EmberNetworkIOWaitEvent(NetworkIO::Interface& api, Output* output,
                             NetworkIO::IORequest req,
                             NetworkIO::IOStatus* outStatus,
                             EmberEventTimeStatistic* stat = nullptr) :
        EmberNetworkIOEvent(api, output, stat),
        m_req(req), m_outStatus(outStatus)
    {}

    ~EmberNetworkIOWaitEvent() {}

    std::string getName() { return "NetworkIOWait"; }

    virtual void issue(uint64_t time, Callback callback) {
        EmberEvent::issue(time);
        auto cb = [callback](NetworkIO::IOStatus st) {
            callback(st.error);
        };
        m_api.wait(m_req, m_outStatus, cb);
    }

    virtual bool complete(uint64_t time, int retval = 0) {
        if (retval != 0) {
            static const bool s_allow =
                std::getenv("SST_EMBER_ALLOW_NETWORKIO_DENY") != nullptr;
            assert(s_allow &&
                   "EmberNetworkIOWaitEvent::complete: wait aggregate "
                   "delivered non-OK error. Set SST_EMBER_ALLOW_NETWORKIO_DENY "
                   "to override for deny/short tests.");
        }
        return EmberEvent::complete(time, retval);
    }

private:
    NetworkIO::IORequest m_req;
    NetworkIO::IOStatus* m_outStatus;
};

}
}
