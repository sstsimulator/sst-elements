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


#include "mpi.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "siriusconst.h"

int sirius_rank;
int sirius_npes;
struct timespec load_lib_time;
double load_library;

FILE* trace_dump;
