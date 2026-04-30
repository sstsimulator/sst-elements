// SPDX-FileCopyrightText: Copyright Hewlett Packard Enterprise Development LP
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <sst/core/sst_types.h>
#include <functional>
#include <cassert>
#include "hermes.h"

namespace SST {
namespace Hermes {
namespace NetworkIO {

// Network Storage I/O Interface
class Interface : public Hermes::Interface {
  public:
    typedef std::function<void(int)> Callback;

    Interface( ComponentId_t id ) : Hermes::Interface(id) {}

    virtual ~Interface() = default;

    // Network IO READ - reads from network storage to local buffer
    // dest: local destination address
    // offset: global byte offset in network storage (node ID calculated via interleaving)
    // length: number of bytes to read
    // blocking: whether to block until completion
    // callback: completion callback
    virtual void networkIORead(Vaddr dest, uint64_t offset, uint64_t length, Callback) { assert(0); }

    // Network IO WRITE - writes from local buffer to network storage
    // offset: global byte offset in network storage (node ID calculated via interleaving)
    // src: local source address
    // length: number of bytes to write
    // blocking: whether to block until completion
    // callback: completion callback
    virtual void networkIOWrite(uint64_t offset, Vaddr src, uint64_t length, Callback) { assert(0); }
};

} // namespace NetworkIO
} // namespace Hermes
} // namespace SST

