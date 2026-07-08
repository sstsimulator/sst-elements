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

class EmberNetworkIOIReadAtEvent : public EmberNetworkIOEvent {
public:
    EmberNetworkIOIReadAtEvent(NetworkIO::Interface& api, Output* output,
                                NetworkIO::FileHandle handle,
                                Hermes::MemAddr dest, uint64_t offset,
                                uint64_t length,
                                NetworkIO::IORequest* outReq,
                                EmberEventTimeStatistic* stat = nullptr) :
        EmberNetworkIOEvent(api, output, stat),
        m_handle(handle), m_dest(dest), m_offset(offset),
        m_length(length), m_outReq(outReq)
    {}

    ~EmberNetworkIOIReadAtEvent() {}

    std::string getName() { return "NetworkIOIReadAt"; }

    virtual void issue(uint64_t time, Callback callback) {
        // PR 10: non-blocking. Fire the engine completion callback
        // synchronously after handing the request off to the API; the
        // ACK arrives asynchronously and updates IORequest state for
        // the eventual wait/waitall. Mirrors MPI_File_iread_at returning
        // immediately with an MPI_Request handle.
        EmberEvent::issue(time);
        m_api.iread_at(m_handle, m_offset, m_dest.getSimVAddr(),
                       m_length, m_outReq, NetworkIO::StatusCallback());
        callback(0);
    }

    virtual bool complete(uint64_t time, int retval = 0) {
        return EmberEvent::complete(time, retval);
    }

private:
    NetworkIO::FileHandle m_handle;
    Hermes::MemAddr m_dest;
    uint64_t m_offset;
    uint64_t m_length;
    NetworkIO::IORequest* m_outReq;
};

}
}
