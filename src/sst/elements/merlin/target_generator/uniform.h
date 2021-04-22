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


#ifndef COMPONENTS_MERLIN_TARGET_GENERATOR_UNIFORM_H
#define COMPONENTS_MERLIN_TARGET_GENERATOR_UNIFORM_H

#include <sst/elements/merlin/target_generator/target_generator.h>

#include <sst/core/rng/mersenne.h>
#include <sst/core/rng/uniform.h>


namespace SST {
namespace Merlin {


class UniformDist : public TargetGenerator {

public:

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        UniformDist,
        "merlin",
        "targetgen.uniform",
        SST_ELI_ELEMENT_VERSION(0,0,1),
        "Generates a uniform random set of target IDs.",
        SST::Merlin::TargetGenerator)

    SST_ELI_DOCUMENT_PARAMS(
        {"min",   "Minimum address to generate","0"},
        {"max",   "Maximum address to generate","numpeers - 1"}
   )

    MersenneRNG* gen;
    SSTUniformDistribution* dist;

    int min;
    int max;


public:

    UniformDist(ComponentId_t cid, Params &params, int id, int num_peers) :
        TargetGenerator(cid)
    {
        min = params.find<int>("min",0);
        max = params.find<int>("max",num_peers - 1);

        gen = new MersenneRNG(id);

        int dist_size = std::max(1, max-min);
        dist = new SSTUniformDistribution(dist_size, gen);

    }

    ~UniformDist() {
        delete dist;
        delete gen;
    }

    void initialize(int id, int num_peers) {
        gen = new MersenneRNG(id);

        if ( min == -1 ) min = 0;
        if ( max == -1 ) max = num_peers;

        int dist_size = std::max(1, max-min);
        dist = new SSTUniformDistribution(dist_size, gen);
    }

    int getNextValue(void) {
        // TraceFunction trace(CALL_INFO);
        return (int)dist->getNextDouble() + min;
    }

    void seed(uint32_t val) {
        delete dist;
        delete gen;
        gen = new MersenneRNG((unsigned int) val);
        dist = new SSTUniformDistribution(std::max(1, max-min),gen);
    }
};

} //namespace Merlin
} //namespace SST

#endif
