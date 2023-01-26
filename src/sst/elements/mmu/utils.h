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

#ifndef _UTILS_H
#define _UTILS_H

namespace SST {

namespace MMU_Lib {

    static bool checkPerms( uint32_t wantPerms, uint32_t havePerms ) {
        // want executable
        if ( wantPerms & 1 && ! ( havePerms & 1 ) ) {
            return false;
        }
        // want write
        if ( wantPerms & (1<<1) && ! ( havePerms & (1<<1) ) ) {
            return false;
        }
        // want read 
        if ( wantPerms & (1<<2) && ! ( havePerms & (1<<2) ) ) {
            return false;
        }
        return true;
    }
}
}

#endif
