// -*- mode: c++ -*-

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


#ifndef COMPONENTS_MERLIN_TARGET_GENERATOR_SHIFT_H
#define COMPONENTS_MERLIN_TARGET_GENERATOR_SHIFT_H

#include <sst/elements/merlin/target_generator/target_generator.h>

namespace SST {
namespace Merlin {


class ShiftDist : public TargetGenerator {

public:

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        ShiftDist,
        "merlin",
        "targetgen.shift",
        SST_ELI_ELEMENT_VERSION(0,0,1),
        "Generates a generalized bit complement pattern.  Returns the same value of num_peers - 1 - id.",
        SST::Merlin::TargetGenerator)

    SST_ELI_DOCUMENT_PARAMS(
        {"shift",        "Number of endpoints to shift by"}        
    )

    int dest;

public:

    ShiftDist(ComponentId_t cid, Params &params, int id, int num_peers) :
        TargetGenerator(cid)
    {
        TraceFunction trace(CALL_INFO_LONG);
        bool found;
        int shift = params.find<int>("shift",found);
        trace.output("%d, %d, %d\n", id, num_peers, shift);
        if ( !found ) {
            fatal(CALL_INFO,1,"ERROR: shift must be specified in targetgen.shift\n");
        }
        dest = (id + shift) % num_peers;
    }

    ~ShiftDist() {
    }

    void initialize(int id, int num_peers) {
        dest = num_peers - 1 - id;
    }

    int getNextValue(void) {
        return dest;
    }

    void seed(uint32_t val) {
    }
};

} //namespace Merlin
} //namespace SST

#endif
