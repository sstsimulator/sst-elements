// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MMU_TYPES_H
#define MMU_TYPES_H

namespace SST {

namespace MMU_Lib {

typedef uintptr_t RequestID;

struct PTE {
    PTE() : ppn(0), perms(0) {}
    PTE( uint32_t ppn, uint32_t perms ) : ppn(ppn), perms(perms) {}
    uint32_t perms : 3;
    uint32_t ppn: 29;

    void serialize_order(SST::Core::Serialization::serializer& ser) {
        // Init local (non-bit field) version to vars (serializing) or 0 (deserializing)
        uint32_t perms_loc = perms;
        uint32_t ppn_loc = ppn;
        // Serialize/deserialize
        SST_SER_NAME(perms_loc, "perms");
        SST_SER_NAME(ppn_loc, "ppn");
        // If de-serializing, update vars. Otherwise, a no-op.
        perms = perms_loc;
        ppn = ppn_loc;
    }
};

} //namespace MMU_Lib
} //namespace SST

#endif /* EVENTS_H */
