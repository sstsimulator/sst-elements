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

class EmberNetworkIOTestEvent : public EmberNetworkIOEvent {
public:
    EmberNetworkIOTestEvent(NetworkIO::Interface& api, Output* output,
                             NetworkIO::IORequest req,
                             bool* outDone,
                             NetworkIO::IOStatus* outStatus,
                             EmberEventTimeStatistic* stat = nullptr) :
        EmberNetworkIOEvent(api, output, stat),
        m_req(req), m_outDone(outDone), m_outStatus(outStatus)
    {}

    ~EmberNetworkIOTestEvent() {}

    std::string getName() { return "NetworkIOTest"; }

    virtual void issue(uint64_t time, Callback callback) {
        EmberEvent::issue(time);
        auto cb = [callback](NetworkIO::IOStatus st) {
            callback(st.error);
        };
        m_api.test(m_req, m_outDone, m_outStatus, cb);
    }

    virtual bool complete(uint64_t time, int retval = 0) {
        return EmberEvent::complete(time, retval);
    }

private:
    NetworkIO::IORequest m_req;
    bool* m_outDone;
    NetworkIO::IOStatus* m_outStatus;
};

}
}
