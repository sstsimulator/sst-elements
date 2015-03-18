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

#ifndef _MY_ALLREDUCE_H_
#define _MY_ALLREDUCE_H_

#include "collective_patterns/collective_topology.h"


void my_allreduce(double *in, double *result, int msg_len, Collective_topology *ctopo);


#endif  // _MY_ALLREDUCE_H_
