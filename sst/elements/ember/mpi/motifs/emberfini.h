// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_FINI
#define _H_EMBER_FINI

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberFiniGenerator : public EmberMessagePassingGenerator {

public:
    EmberFiniGenerator(SST::Component* owner, Params& params) :
        EmberMessagePassingGenerator(owner, params)
    {
        m_name = "Fini";
    }

    bool generate( std::queue<EmberEvent*>& evQ)
    {
        GEN_DBG( 2, "\n" );
        enQ_fini( evQ );
        return true;
    }

    void finish(const SST::Output* output, uint64_t time ) {
        delete getData();
        EmberMessagePassingGenerator::finish(output,time);
    }
};

}
}

#endif
