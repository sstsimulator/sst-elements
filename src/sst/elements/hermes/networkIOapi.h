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

    /**
     * @brief Read from network storage to local buffer
     * @param dest   Local destination address
     * @param offset Global byte offset in network storage (node ID calculated via interleaving)
     * @param length Number of bytes to read
     * @param callback Completion callback
     */
    virtual void networkIORead(Vaddr dest, uint64_t offset, uint64_t length, Callback) { assert(0); }

    /**
     * @brief Write from local buffer to network storage
     * @param offset Global byte offset in network storage (node ID calculated via interleaving)
     * @param src    Local source address
     * @param length Number of bytes to write
     * @param callback Completion callback
     */
    virtual void networkIOWrite(uint64_t offset, Vaddr src, uint64_t length, Callback) { assert(0); }
};

} // namespace NetworkIO
} // namespace Hermes
} // namespace SST

