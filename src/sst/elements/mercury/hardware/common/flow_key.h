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

#pragma once

#include <cstdint>

namespace SST {
namespace Hg {

// NIC reassembly key: source in top 16 bits, flowId in low 48 (flowId alone collides at fan-in).
constexpr uint64_t kFlowKeyMaxSrc = 0xFFFFull;       // 16-bit source field
constexpr uint64_t kFlowKeyIdMask = 0xFFFFFFFFFFFFull; // low 48 bits

inline constexpr uint64_t recvCqFlowKey(uint64_t src, uint64_t flowId) {
  return (src << 48) | (flowId & kFlowKeyIdMask);
}

} // namespace Hg
} // namespace SST
