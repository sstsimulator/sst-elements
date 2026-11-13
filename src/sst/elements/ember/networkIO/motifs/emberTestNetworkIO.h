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

#include <strings.h>
#include <string>
#include <vector>
#include <sst/core/rng/marsaglia.h>
#include "networkIO/emberNetworkIOGen.h"

namespace SST {
namespace Ember {

class EmberTestNetworkIOGenerator : public EmberNetworkIOGenerator
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberTestNetworkIOGenerator,
        "ember",
        "TestNetworkIOMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Network IO Test",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"arg.messageSize","Message size in bytes","1024"},
        {"arg.iterations","Number of iterations to perform","1"},
        {"arg.op","Operation type: read, write, read_at, write_at, "
                  "open_close, two_files, short_read, async, "
                  "async_waitall_16, async_cancel","write"},
        {"arg.fileSize","Storage file size in bytes","10485760"},
        {"arg.capacity","Per-file capacity for handle ops (0=unbounded)","0"},
        {"arg.asyncBatch","PR 10 async-mode in-flight iread/iwrite batch "
                         "size (max 16). iterations is reused as the count "
                         "of iread_at issues per phase.","4"},
        {"arg.cancelInflight","PR 10 async-mode: cancel reqs[1] before "
                              "the waitall fires (0/1).","0"},
        {"arg.rngSeedZ","Non-zero MarsagliaRNG z seed; 0 = wall-clock seed","0"},
        {"arg.rngSeedW","Non-zero MarsagliaRNG w seed; 0 = wall-clock seed","0"}
    )

    EmberTestNetworkIOGenerator(SST::ComponentId_t id, Params& params);

    bool generate( std::queue<EmberEvent*>& evQ);

private:
    int m_phase;

    SST::RNG::MarsagliaRNG* m_rng;

    uint32_t m_messageSize;
    uint32_t m_iterations;
    uint64_t m_fileSize;
    uint64_t m_capacity;
    std::string m_opType;

    Hermes::MemAddr m_localBuffer;

    Hermes::NetworkIO::FileHandle m_fhA = nullptr;
    Hermes::NetworkIO::FileHandle m_fhB = nullptr;

    // PR 10: async-mode state. m_asyncBatch is the number of in-flight
    // iread_at / iwrite_at requests issued per phase; reqs[] is the
    // request handle array consumed by waitall.
    static constexpr size_t kMaxAsyncBatch = 16;
    uint32_t m_asyncBatch = 4;
    Hermes::NetworkIO::IORequest m_reqs[kMaxAsyncBatch] = {};
    Hermes::NetworkIO::IOStatus  m_statuses[kMaxAsyncBatch] = {};
    bool m_cancelInflight = false;

    uint64_t m_startTime;
    uint64_t m_stopTime;
};
}
}
