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

#ifndef _H_SHOGUN_ARB_H
#define _H_SHOGUN_ARB_H

#include "shogun_event.h"
#include "shogun_q.h"

using namespace SST::Shogun;

namespace SST {
namespace Shogun {

    class ShogunStatisticsBundle;

    class ShogunArbitrator {

    public:
        ShogunArbitrator() {}
        virtual ~ShogunArbitrator() {}

    virtual void moveEvents(const int num_events,
                            const int port_count,
                            ShogunQueue<ShogunEvent*>** inputQueues,
                            int32_t output_slots,
                            ShogunEvent*** outputEvents,
                            uint64_t cycle )
                            = 0;

        void setOutput(SST::Output* out)
        {
            output = out;
        }

        void setStatisticsBundle(ShogunStatisticsBundle* b)
        {
            bundle = b;
        }

    protected:
        SST::Output* output;
        ShogunStatisticsBundle* bundle;
    };

}
}

#endif
