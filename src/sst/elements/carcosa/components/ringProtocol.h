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

#ifndef CARCOSA_RING_PROTOCOL_H
#define CARCOSA_RING_PROTOCOL_H

#include <string>

namespace SST {
namespace Carcosa {

// Hali ring accelerator handshake: hub SeqLen+Cmd (-> optional payload),
// partner Done, hub Exit. carcosa never parses Cmd payload; partners agree privately.
namespace RingTag {
    static constexpr const char* Cmd    = "cmd";     // dispatch a GPU-resident kernel
    static constexpr const char* SeqLen = "seqlen";  // sequence-length hint before a Cmd
    static constexpr const char* Done   = "done";    // partner completed the dispatched work
    static constexpr const char* Exit   = "exit";    // end of run
}

// Convenience predicates (avoid scattering string literals across agents).
inline bool ringTagIs(const std::string& s, const char* tag) { return s == tag; }

} // namespace Carcosa
} // namespace SST

#endif // CARCOSA_RING_PROTOCOL_H
