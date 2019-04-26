// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_ARIEL_TRACE_GEN
#define _H_SST_ARIEL_TRACE_GEN

#include <sst/core/module.h>

namespace SST {
namespace ArielComponent {

typedef enum {
    READ,
    WRITE
} ArielTraceEntryOperation;

class ArielTraceGenerator : public Module {

    public:
        ArielTraceGenerator() {}
        ~ArielTraceGenerator() {}

        virtual void publishEntry(const uint64_t picoS,
                const uint64_t physAddr,
                const uint32_t reqLength,
                const ArielTraceEntryOperation op) = 0;
        virtual void setCoreID(uint32_t coreID) = 0;

};

}
}

#endif
