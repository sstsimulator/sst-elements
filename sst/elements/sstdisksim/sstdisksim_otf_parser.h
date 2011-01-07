// Copyright 2009-2011 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2011, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SSTDISKSIM_OTF_PARSER_H
#define _SSTDISKSIM_OTF_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <iostream>
#include <stdarg.h>
#include <string.h>
#include <fstream>
#include <string>
#include "sstdisksim_event.h"

using namespace std;

class sstdisksim_otf_parser {

 public:
  sstdisksim_otf_parser(string filename);
  ~sstdisksim_otf_parser();
  sstdisksim_event* getNextEvent();

 private:
  ifstream filestream;
};

#endif /* _SSTDISKSIM_OTF_PARSER_H */
