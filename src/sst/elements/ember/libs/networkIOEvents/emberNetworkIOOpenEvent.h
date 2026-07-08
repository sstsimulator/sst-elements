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
#include <string>

namespace SST {
namespace Ember {

class EmberNetworkIOOpenEvent : public EmberNetworkIOEvent {
public:
    EmberNetworkIOOpenEvent(NetworkIO::Interface& api, Output* output,
                            const std::string& path,
                            NetworkIO::OpenMode mode,
                            NetworkIO::FileInfo info,
                            NetworkIO::FileHandle* outHandle,
                            EmberEventTimeStatistic* stat = nullptr) :
        EmberNetworkIOEvent(api, output, stat),
        m_path(path), m_mode(mode), m_info(std::move(info)),
        m_outHandle(outHandle)
    {}

    ~EmberNetworkIOOpenEvent() {}

    std::string getName() { return "NetworkIOOpen"; }

    virtual void issue(uint64_t time, Callback callback) {
        EmberEvent::issue(time);
        auto cb = [callback](NetworkIO::IOStatus st) {
            callback(st.error);
        };
        m_api.open(m_path, m_mode, m_info, m_outHandle, cb);
    }

    virtual bool complete(uint64_t time, int retval = 0) {
        if (retval != 0) {
            static const bool s_allow =
                std::getenv("SST_EMBER_ALLOW_NETWORKIO_DENY") != nullptr;
            assert(s_allow &&
                   "EmberNetworkIOOpenEvent::complete: open returned "
                   "non-OK IOStatus.error. Set SST_EMBER_ALLOW_NETWORKIO_DENY "
                   "to override for deny/short tests.");
        }
        return EmberEvent::complete(time, retval);
    }

private:
    std::string m_path;
    NetworkIO::OpenMode m_mode;
    NetworkIO::FileInfo m_info;
    NetworkIO::FileHandle* m_outHandle;
};

}
}
