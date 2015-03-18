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
#include <stdio.h>
#include <math.h>
#include "stats.h"



void
print_stats(std::list<double> t)
{
    print_stats(t, 0.0, false);
}  /* end of stats() */



void
print_stats(std::list<double> t, double precision)
{
    print_stats(t, precision, true);
}  /* end of stats() */



void
print_stats(std::list<double> t, double precision, bool print_p)
{

int cnt;
std::list<double>::iterator it;
int num_sets;
double min, avg, med, max, sd;


	t.sort();
	cnt= 0;
	num_sets= t.size();
	med= 0.0;
	avg= 0.0;
	for (it= t.begin(); it != t.end(); it++)   {
	    if (cnt == (num_sets / 2))   {
		// FIXME: For an even number of entries, this should be the average of the
		// two middle ones.
		med= *it;
	    }
	    avg= avg + *it;
	    cnt++;
	}
	avg= avg / num_sets;

	// Calculate the standard deviation
	sd= 0.0;
	for (it= t.begin(); it != t.end(); it++)   {
	    sd= sd + ((*it - avg) * (*it - avg));
	}
	sd= sqrt(sd / num_sets);
	min= t.front();
	max= t.back();

	if (print_p)   {
	    printf("%12.9f %12.9f %12.9f %12.9f %12.9f %12.6f", min, avg, med, max, sd, precision);
	} else   {
	    printf("%12.9f %12.9f %12.9f %12.9f %12.9f", min, avg, med, max, sd);
	}

}  /* end of stats() */
