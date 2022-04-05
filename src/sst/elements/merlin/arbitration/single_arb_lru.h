// -*- mode: c++ -*-

// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_ARBITRATION_SINGLE_ARB_LRU_H
#define COMPONENTS_MERLIN_ARBITRATION_SINGLE_ARB_LRU_H

#include "sst/elements/merlin/arbitration/single_arb.h"

namespace SST {
namespace Merlin {

class single_arb_lru : public SingleArbitration {

public:

    SST_ELI_REGISTER_MODULE_DERIVED(
        single_arb_lru,
        "merlin",
        "arb.base.single.lru",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Base LRU module used when only one match per round is needed.",
        SST::Merlin::SingleArbitration
    )

private:

    int16_t tail;

    int16_t current;
    int16_t last;

    int16_t* order;


public:

    single_arb_lru(Params& params, int16_t size) :
        SingleArbitration()
    {
        // Initialize head.  Index 0 of vector will be the head
        // pointer.
        order = new int16_t[size + 1];
        for ( int i = 0; i < size; ++i ) {
            order[i] = i + 1;
        }
        order[size] = 1;

        current = 0;
        tail = size;
        last = tail;
    }

    ~single_arb_lru() {
        delete[] order;
    }

    int next() {
        last = current;
        current = order[current];
        // Data array is one smaller than order array, so we need to
        // subtract 1.
        return current-1;
    }

    void satisfied() {
        if ( current == 0 ) {
            // next() not called, so nothing to do
            return;
        }
        // If this is the head, we have to do it slightly differently
        if ( current == order[0] ) {
            // Nothing in linked chain changes, just need to change
            // head, tail.  This only works because the "linked list"
            // is circular.
            order[0] = order[current];
            tail = current;
        }
        else if ( current == tail ) {
            // Nothing to be done, list stays unchanged
        }
        else {
            // Make the last one point to the next one
            order[last] = order[current];

            // Make current point to head in preparation for moving it
            // to tail
            order[current] = order[0];
            // Move current to tail
            order[tail] = current;
            tail = current;
        }

        // Reset curr_ind and last.  Once we're satisfied, we start
        // back at the head of the list
        current = 0;
        last = tail;
    }
};


}
}

#endif // COMPONENTS_MERLIN_ROUTER_H
