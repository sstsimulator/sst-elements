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


#ifndef COMPONENTS_MERLIN_TARGET_GENERATOR_BIT_COMPLEMENT_H
#define COMPONENTS_MERLIN_TARGET_GENERATOR_BIT_COMPLEMENT_H

#include <sst/elements/merlin/target_generator/target_generator.h>

namespace SST {
namespace Merlin {


class BitComplementDist : public TargetGenerator {

public:

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        BitComplementDist,
        "merlin",
        "targetgen.bit_complement",
        SST_ELI_ELEMENT_VERSION(0,0,1),
        "Generates a generalized bit complement pattern.  Returns the same value of num_peers - 1 - id.",
        SST::Merlin::TargetGenerator)

    SST_ELI_DOCUMENT_PARAMS(
    )

    int dest;

public:

    BitComplementDist(ComponentId_t cid, Params &params, int id, int num_peers) :
        TargetGenerator(cid)
    {
        dest = num_peers - 1 - id;
    }

    ~BitComplementDist() {
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
