// SPDX-FileCopyrightText: Copyright Hewlett Packard Enterprise Development LP
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "emberNetworkIOEvent.h"

namespace SST {
namespace Ember {

class EmberNetworkIOReadEvent : public EmberNetworkIOEvent {
public:
    EmberNetworkIOReadEvent(NetworkIO::Interface& api, Output* output, Hermes::MemAddr dest,
                          uint64_t offset, uint32_t length,
                          EmberEventTimeStatistic* stat = NULL) :
        EmberNetworkIOEvent(api, output, stat),
        m_dest(dest), m_offset(offset), m_length(length)
    {}

    ~EmberNetworkIOReadEvent() {}

    std::string getName() { return "NetworkIORead"; }

    virtual void issue(uint64_t time, Callback callback) {
        EmberEvent::issue(time);
        m_api.networkIORead(m_dest.getSimVAddr(), m_offset, m_length, callback);
    }

private:
    Hermes::MemAddr m_dest;
    uint64_t m_offset;
    uint32_t m_length;
};

}
}
