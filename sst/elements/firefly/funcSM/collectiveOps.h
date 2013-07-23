// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_FUNCSM_COLLECTIVEOPS_H
#define COMPONENTS_FIREFLY_FUNCSM_COLLECTIVEOPS_H

#include "sst/elements/hermes/msgapi.h"

namespace SST {
namespace Firefly {

void EEE( std::vector<void*>& input, int numIn, void* result, int count, 
            Hermes::PayloadDataType, Hermes::ReductionOperation );   

template <class Type>
void EEE( std::vector<Type*>& input, int numIn, Type* result, int count, 
                                    Hermes::ReductionOperation );   

}
}

#endif
