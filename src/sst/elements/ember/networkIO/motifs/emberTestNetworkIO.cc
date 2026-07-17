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

#include <sst_config.h>
#include "emberTestNetworkIO.h"
#include <iostream>

using namespace SST::Ember;

EmberTestNetworkIOGenerator::EmberTestNetworkIOGenerator(SST::ComponentId_t id, Params& params)
    : EmberNetworkIOGenerator(id, params, "TestNetworkIO"), m_phase(0)
{
        m_messageSize = params.find<uint32_t>("arg.messageSize", 1024);
        m_iterations  = params.find<uint32_t>("arg.iterations", 5);
        m_opType      = params.find<std::string>("arg.op", "write");
        m_fileSize    = params.find<uint64_t>("arg.fileSize", 10485760);
        m_capacity    = params.find<uint64_t>("arg.capacity", 0);
        m_asyncBatch  = params.find<uint32_t>("arg.asyncBatch", 4);
        if (m_asyncBatch > kMaxAsyncBatch) m_asyncBatch = kMaxAsyncBatch;
        m_cancelInflight = params.find<bool>("arg.cancelInflight", false);

        unsigned int seedZ = params.find<unsigned int>("arg.rngSeedZ", 0);
        unsigned int seedW = params.find<unsigned int>("arg.rngSeedW", 0);
        if (seedZ != 0 && seedW != 0) {
            m_rng = new SST::RNG::MarsagliaRNG(seedZ, seedW);
        } else {
            m_rng = new SST::RNG::MarsagliaRNG();
        }
        m_startTime = 0;
        m_stopTime  = 0;
}

bool EmberTestNetworkIOGenerator::generate( std::queue<EmberEvent*>& evQ)
{
    bool ret = false;

    // PR 10 async modes: open A -> N iread_at -> (optional cancel) ->
    // waitall(N) -> close A. Each phase enqueues exactly one logical
    // batch so the EmberEngine state machine reaches Complete between
    // them. See docs/pr-10-design.md §4.6.
    if (m_opType == "async" || m_opType == "async_cancel") {

        Hermes::NetworkIO::FileInfo info;
        if (m_capacity > 0) {
            info.hints["capacity"] = std::to_string(m_capacity);
        }
        const uint32_t N = m_asyncBatch;

        switch (m_phase) {
          case 0:
            memSetNotBacked();
            m_localBuffer = memAlloc(m_messageSize);
            enQ_getTime(evQ, &m_startTime);
            networkIO().open(evQ, "file_A",
                             Hermes::NetworkIO::OpenMode::ReadWrite,
                             info, &m_fhA);
            break;

          case 1:
            for (uint32_t i = 0; i < N; i++) {
                uint64_t off = (uint64_t)i * m_messageSize;
                networkIO().iread_at(evQ, m_fhA, m_localBuffer, off,
                                     m_messageSize, &m_reqs[i]);
            }
            break;

          case 2:
            if (m_opType == "async_cancel" && N >= 2) {
                networkIO().cancel(evQ, m_reqs[1]);
                break;
            }
            ++m_phase;
            return generate(evQ);

          case 3:
            networkIO().waitall(evQ, (int)N, m_reqs, m_statuses);
            break;

          case 4:
            if (m_fhA) networkIO().close(evQ, m_fhA);
            enQ_getTime(evQ, &m_stopTime);
            break;

          case 5: {
            double totalTime =
                (double)(m_stopTime - m_startTime) / 1000000000.0;
            output("async-mode %s, message-size %u, batch %u, "
                   "total-time %.3lf us\n",
                   m_opType.c_str(), m_messageSize, N,
                   totalTime * 1000000.0);
            ret = true;
            break;
          }
        }
        ++m_phase;
        return ret;
    }

    // Handle-based modes: open -> N read_at/write_at -> close.
    // Each motif phase enqueues exactly one logical phase of work so the
    // EmberEngine state machine reaches Complete between them.
    if (m_opType == "open_close" || m_opType == "two_files"
        || m_opType == "short_read") {

        Hermes::NetworkIO::FileInfo info;
        if (m_capacity > 0) {
            info.hints["capacity"] = std::to_string(m_capacity);
        }

        switch (m_phase) {
          case 0:
            memSetNotBacked();
            m_localBuffer = memAlloc(m_messageSize);
            enQ_getTime(evQ, &m_startTime);
            networkIO().open(evQ, "file_A",
                             Hermes::NetworkIO::OpenMode::ReadWrite,
                             info, &m_fhA);
            break;

          case 1:
            if (m_opType == "two_files") {
                Hermes::NetworkIO::FileInfo infoB;
                networkIO().open(evQ, "file_B",
                                 Hermes::NetworkIO::OpenMode::ReadWrite,
                                 infoB, &m_fhB);
            } else {
                ++m_phase;
                return generate(evQ);
            }
            break;

          case 2:
            for (uint32_t i = 0; i < m_iterations; i++) {
                uint64_t off = (uint64_t)i * m_messageSize;
                Hermes::NetworkIO::FileHandle target =
                    (m_opType == "two_files" && (i & 1)) ? m_fhB : m_fhA;
                if (m_opType == "short_read" && i == m_iterations - 1
                    && m_capacity > 0) {
                    networkIO().read_at(evQ, m_fhA, m_localBuffer,
                                        m_capacity - 1,
                                        m_messageSize * 4);
                } else {
                    networkIO().write_at(evQ, target, off, m_localBuffer,
                                         m_messageSize);
                }
            }
            break;

          case 3:
            if (m_fhA) networkIO().close(evQ, m_fhA);
            if (m_fhB) networkIO().close(evQ, m_fhB);
            enQ_getTime(evQ, &m_stopTime);
            break;

          case 4: {
            double totalTime = (double)(m_stopTime - m_startTime)/1000000000.0;
            output("handle-mode %s, message-size %u, iterations %u, total-time %.3lf us\n",
                   m_opType.c_str(), m_messageSize, m_iterations,
                   totalTime * 1000000.0);
            ret = true;
            break;
          }
        }
        ++m_phase;
        return ret;
    }

    switch(m_phase)
    {
        case 0:
            memSetNotBacked();
            m_localBuffer = memAlloc(m_messageSize);
            enQ_getTime(evQ, &m_startTime);
            for (uint32_t i = 0; i < m_iterations; i++)
            {
                uint64_t offset = m_rng->generateNextUInt64() % m_fileSize;

                if (m_opType == "read")
                    networkIO().networkIORead(evQ, m_localBuffer, offset, m_messageSize);
                else
                    networkIO().networkIOWrite(evQ, offset, m_localBuffer, m_messageSize);
            }
            enQ_getTime(evQ, &m_stopTime);
            break;

        case 1:
            double totalTime = (double)(m_stopTime - m_startTime)/1000000000.0;
            double latency = (totalTime/m_iterations);
            output("message-size %u, iterations %u, total-time %.3lf us\n",
                        m_messageSize, m_iterations, totalTime * 1000000.0) ;
            ret = true;
            break;
    }

    ++m_phase;
    return ret;
}
