// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdio.h>

#define INNER_LOOP 1024*1024
#define OUTER_LOOP 100
int main() { 

  register int a,b;
  register double c, d=0.0;
  for(a=0; a<OUTER_LOOP; a++) 
    for(b=0; b<INNER_LOOP; b++) { 
      // do some simple computations
      c = (double)(b+2.25); 
      d = (double)(2.0+c)*(d+1.0);
    }

  return d>2.5; 

} 
