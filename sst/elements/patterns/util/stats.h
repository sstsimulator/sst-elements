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

/*
** Some helper functions to calculate and print statistics
*/
#ifndef _STATS_H_
#define _STATS_H_

#if defined(NOT_PART_OF_SIM)
#include <list>
using namespace std;
#else
#include <boost/serialization/list.hpp>
#endif

void print_stats(std::list<double> t);
void print_stats(std::list<double> t, double precision);
void print_stats(std::list<double> t, double precision, bool print_p);

#endif // _STATS_H_
