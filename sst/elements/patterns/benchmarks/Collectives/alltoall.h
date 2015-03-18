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

#ifndef _MY_ALLTOALL_H_
#define _MY_ALLTOALL_H_


void my_alltoall(double *in, double *result, int num_doubles, int nranks, int my_rank);


#endif  // _MY_ALLTOALL_H_
