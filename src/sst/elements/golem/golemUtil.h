// Copyright 2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _GOLEMUTIL_H
#define _GOLEMUTIL_H

#include <vector>

namespace SST {
namespace Golem {

/* Set of functions which are used by multiple components (tiles and drivers)
*/

//For simplicity this works 8B at a time, should be sufficient for most implementations.
//If you need more, call this multiple times
inline void intToData(uint64_t num, std::vector<uint8_t>* data, uint8_t payloadSize) {
    data->clear();
    assert(payloadSize <= 8);

    for (int i = 0; i < payloadSize; ++i) {
        data->push_back(num & 0xFF);
        num >>=8;
    }
}


//Same as with intToData, 8B should be enough for most applications
//If it isn't the caller can split the vector into 8 element chunks
inline uint64_t dataToInt(std::vector<uint8_t>* data) {
    uint64_t retval = 0;
    assert (data->size() <= 8);

    for (int i = data->size(); i > 0; i--) {
        retval <<= 8;
        retval |= (*data)[i-1];
    }
    return retval;
}

}
}

#endif /* _GOLEMUTIL_H */