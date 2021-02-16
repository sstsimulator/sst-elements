// Copyright 2009-2021 NTESS. Under the terms
// // of Contract DE-NA0003525 with NTESS, the U.S.
// // Government retains certain rights in this software.
// //
// // Copyright (c) 2009-2021, NTESS
// // All rights reserved.
// //
// // This file is part of the SST software package. For license
// // information, see the LICENSE file in the top level directory of the
// // distribution.

#ifndef _H_SST_SAMBA_UTILS
#define _H_SST_SAMBA_UTILS

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/elements/memHierarchy/memEventBase.h>

namespace SST {
namespace SambaComponent {

    // Comparator for MemEventBase pointers for deterministic ordering when pointers are used as map keys
    struct MemEventPtrCompare {
        bool operator()(const MemHierarchy::MemEventBase* ptrA, const MemHierarchy::MemEventBase* ptrB) const {
            if (ptrA->getID().second < ptrB->getID().second) { // Compare on rank
                return true;
            } else {
                return ptrA->getID().first < ptrB->getID().first;
            }
        }
    };
}
}

#endif
