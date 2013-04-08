// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * Utility functions
 */

#ifndef __MISC_H__
#define __MISC_H__

#define SCHEDULER_TIME_BASE "1 us"

#include <string>
using namespace std;

namespace SST {
namespace Scheduler {

void warning(string mesg);         //report warning (program continues)
void error(string mesg);           //report user-caused error
void internal_error(string mesg);  //report invalid program state

}
}
#endif
