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


#include "mersenne.h"

namespace McOpteron{		//Scoggin: Added a namespace to reduce possible conflicts as library
double genRandomProbability()
{
   double r = (double) genrand_real1(); // this function is defined in mersene.h
   return r;
}

void seedRandom(unsigned long seed)
{
   init_genrand(seed);
}
}
//end namespace McOpteron
