// Copyright 2011 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2011, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SSTDISKSIM_TAU_PARSER_H
#define _SSTDISKSIM_TAU_PARSER_H

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
#include "sstdisksim_posix_call.h"
#include "sstdisksim_diskmodel.h"

using namespace std;

class sstdisksim_tau_parser {

 public:
  sstdisksim_tau_parser(const char* trc_file, const char* edf_file,
			sstdisksim_diskmodel* model);
  ~sstdisksim_tau_parser();
};

#endif // _SSTDISKSIM_TAU_PARSER_H 
