// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <inttypes.h>

#ifndef _H_SST_ARIEL_READ_EVENT
#define _H_SST_ARIEL_READ_EVENT

namespace SST {
namespace RtlComponent {

class RtlReadEvent {

    public:
        RtlReadEvent(uint64_t rAddr, uint32_t length) :
                readAddress(rAddr), readLength(length) {
        }

        ~RtlReadEvent() {
        }

        uint64_t getAddress() const {
                return readAddress;
        }

        uint32_t getLength() const {
                return readLength;
        }

    private:
        const uint64_t readAddress;
        const uint32_t readLength;

};

}
}

#endif
