// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef CARCOSA_HYADES_PROTOCOL_H
#define CARCOSA_HYADES_PROTOCOL_H

#include <cstdint>

namespace SST {
namespace Carcosa {

/**
 * C++ mirror of hyades.h MMIO ABI — keep offsets in sync across transports.
 */
namespace HyadesAbi {
    static constexpr uint64_t kCommand        = 0x00; // R: next kernel/action index; <0 = exit
    static constexpr uint64_t kStatus         = 0x04; // W: completed index (kernel-end / FSM advance)
    static constexpr uint64_t kSeqLen         = 0x08; // R: current sequence length
    static constexpr uint64_t kRole           = 0x10; // R: core role (0 = default)
    static constexpr uint64_t kRegionBaseLo   = 0x40;  // W: staged region base, low 32 bits
    static constexpr uint64_t kRegionBaseHi   = 0x80;  // W: staged region base, high 32 bits
    static constexpr uint64_t kRegionSize     = 0xC0;  // W: staged region size
    static constexpr uint64_t kRegionCommit   = 0x100; // W: commit staged region into slot N
    static constexpr uint64_t kActionChecksum = 0x140; // W: per-frame action checksum, low 32 bits (commits the value)
    static constexpr uint64_t kActionToken    = 0x180; // W: per-frame decoded-action token
    // Action-checksum HI. Workload writes HI, fences, then LO (commit); a
    // second fence keeps status-write from overtaking. Consumers clear HI after LO.
    static constexpr uint64_t kActionChecksumHi = 0x1C0;

    constexpr uint64_t combineActionChecksum(uint32_t hi, uint32_t lo) {
        return (static_cast<uint64_t>(hi) << 32) | static_cast<uint64_t>(lo);
    }
    static_assert(combineActionChecksum(0x01234567u, 0x89ABCDEFu) ==
                  0x0123456789ABCDEFull,
                  "Hyades checksum HI/LO combine must preserve all 64 bits");

    static constexpr int      kExitSentinel   = -1;   // command value meaning "end run"
}

} // namespace Carcosa
} // namespace SST

#endif // CARCOSA_HYADES_PROTOCOL_H
