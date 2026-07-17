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

// SPDX-FileCopyrightText: Copyright Hewlett Packard Enterprise Development LP
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "sst/elements/ember/embergen.h"
#include "sst/elements/ember/libs/emberLib.h"
#include "sst/elements/ember/libs/networkIOEvents/emberNetworkIOReadEvent.h"
#include "sst/elements/ember/libs/networkIOEvents/emberNetworkIOWriteEvent.h"
#include "sst/elements/ember/libs/networkIOEvents/emberNetworkIOOpenEvent.h"
#include "sst/elements/ember/libs/networkIOEvents/emberNetworkIOCloseEvent.h"
#include "sst/elements/ember/libs/networkIOEvents/emberNetworkIOReadAtEvent.h"
#include "sst/elements/ember/libs/networkIOEvents/emberNetworkIOWriteAtEvent.h"
#include "sst/elements/ember/libs/networkIOEvents/emberNetworkIOIReadAtEvent.h"
#include "sst/elements/ember/libs/networkIOEvents/emberNetworkIOIWriteAtEvent.h"
#include "sst/elements/ember/libs/networkIOEvents/emberNetworkIOWaitEvent.h"
#include "sst/elements/ember/libs/networkIOEvents/emberNetworkIOWaitallEvent.h"
#include "sst/elements/ember/libs/networkIOEvents/emberNetworkIOTestEvent.h"
#include "sst/elements/ember/libs/networkIOEvents/emberNetworkIOTestAnyEvent.h"
#include "sst/elements/ember/libs/networkIOEvents/emberNetworkIOCancelEvent.h"
#include "sst/elements/hermes/networkIOapi.h"

using namespace Hermes;

namespace SST {
namespace Ember {

class EmberNetworkIOLib : public EmberLib {
public:
    SST_ELI_REGISTER_MODULE(
        EmberNetworkIOLib,
        "ember",
        "networkIOLib",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Network I/O Library for network-attached storage operations",
        SST::Ember::EmberLib
    )

    SST_ELI_DOCUMENT_PARAMS()

    EmberNetworkIOLib(Params& params) {}

    void networkIORead(std::queue<EmberEvent*>& q, Hermes::MemAddr dest, uint64_t offset, uint64_t length)
    {
        q.push(new EmberNetworkIOReadEvent(api(), m_output, dest, offset, length));
    }

    void networkIOWrite(std::queue<EmberEvent*>& q, uint64_t offset, Hermes::MemAddr src, uint64_t length)
    {
        q.push(new EmberNetworkIOWriteEvent(api(), m_output, offset, src, length));
    }

    void open(std::queue<EmberEvent*>& q, const std::string& path,
              NetworkIO::OpenMode mode, NetworkIO::FileInfo info,
              NetworkIO::FileHandle* outHandle)
    {
        q.push(new EmberNetworkIOOpenEvent(api(), m_output, path, mode,
                                           std::move(info), outHandle));
    }

    void close(std::queue<EmberEvent*>& q, NetworkIO::FileHandle handle)
    {
        q.push(new EmberNetworkIOCloseEvent(api(), m_output, handle));
    }

    void read_at(std::queue<EmberEvent*>& q, NetworkIO::FileHandle handle,
                 Hermes::MemAddr dest, uint64_t offset, uint64_t length)
    {
        q.push(new EmberNetworkIOReadAtEvent(api(), m_output, handle,
                                             dest, offset, length));
    }

    void write_at(std::queue<EmberEvent*>& q, NetworkIO::FileHandle handle,
                  uint64_t offset, Hermes::MemAddr src, uint64_t length)
    {
        q.push(new EmberNetworkIOWriteAtEvent(api(), m_output, handle,
                                              offset, src, length));
    }

    void iread_at(std::queue<EmberEvent*>& q, NetworkIO::FileHandle handle,
                  Hermes::MemAddr dest, uint64_t offset, uint64_t length,
                  NetworkIO::IORequest* outReq)
    {
        q.push(new EmberNetworkIOIReadAtEvent(api(), m_output, handle,
                                              dest, offset, length, outReq));
    }

    void iwrite_at(std::queue<EmberEvent*>& q, NetworkIO::FileHandle handle,
                   uint64_t offset, Hermes::MemAddr src, uint64_t length,
                   NetworkIO::IORequest* outReq)
    {
        q.push(new EmberNetworkIOIWriteAtEvent(api(), m_output, handle,
                                               offset, src, length, outReq));
    }

    void wait(std::queue<EmberEvent*>& q, NetworkIO::IORequest req,
              NetworkIO::IOStatus* outStatus)
    {
        q.push(new EmberNetworkIOWaitEvent(api(), m_output, req, outStatus));
    }

    void waitall(std::queue<EmberEvent*>& q, int count,
                 NetworkIO::IORequest* reqs, NetworkIO::IOStatus* statuses)
    {
        q.push(new EmberNetworkIOWaitallEvent(api(), m_output, count,
                                              reqs, statuses));
    }

    void test(std::queue<EmberEvent*>& q, NetworkIO::IORequest req,
              bool* outDone, NetworkIO::IOStatus* outStatus)
    {
        q.push(new EmberNetworkIOTestEvent(api(), m_output, req,
                                           outDone, outStatus));
    }

    void testany(std::queue<EmberEvent*>& q, int count,
                 NetworkIO::IORequest* reqs, int* outIdx,
                 bool* outDone, NetworkIO::IOStatus* outStatus)
    {
        q.push(new EmberNetworkIOTestAnyEvent(api(), m_output, count, reqs,
                                              outIdx, outDone, outStatus));
    }

    void cancel(std::queue<EmberEvent*>& q, NetworkIO::IORequest req)
    {
        q.push(new EmberNetworkIOCancelEvent(api(), m_output, req));
    }

private:
    NetworkIO::Interface& api() { return *static_cast<NetworkIO::Interface*>(m_api); }
};

}
}
