// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
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

typedef uint64_t RequestID;

struct PTE {
    PTE() : ppn(0), perms(0) {}
    PTE( uint32_t ppn, uint32_t perms ) : ppn(ppn), perms(perms) {} 
    uint32_t perms : 3;
    uint32_t ppn: 29;
};

} //namespace MMU_Lib
} //namespace SST

#endif /* EVENTS_H */
