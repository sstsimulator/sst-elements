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

#ifndef _STAT_P_H_
#define _STAT_P_H_

double stat_p(int N, double tot, double tot_squared);
void disp_stats(double *results, int cnt, double precision);

#endif /* _STAT_P_H_ */
