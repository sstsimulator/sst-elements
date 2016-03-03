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

#include <stdio.h>
#include <stdlib.h>	/* For qsort() */
#include <math.h>
#if STANDALONECPLUSPLUS
extern "C" {
#endif
#include "stat_p.h"




/*

95% of the area of a t distribution is within
2.78 standard deviations of the mean for a sample size of 
at least 4.
95% of the area of a normal distribution is within
1.96 standard deviations of the mean. this will happen
for a t dist if sample size is large enough

df	0.95	0.99
====================
2	4.303	9.925
3	3.182	5.841
4	2.776	4.604
5	2.571	4.032
8	2.306	3.355
10	2.228	3.169
20	2.086	2.845
50	2.009	2.678
100	1.984	2.626
We will do at least 6 (df=n-1=5) iterations to need no more than 2.571 SD's. 

*/



double 
stat_p(int N, double tot, double tot_squared)
{

double tval[]= {1000, 1000, 4.303, 3.182, 2.776, 2.571};
double tVal;
double precision;
double mean;
double sd;
double se;
int ii;
double halfCI;


    if (N <= 0)   {
	return tval[0];
    }

    mean= tot / N;

    if ((tot_squared / N - mean * mean) < 0.0)   {
	return tval[0];
    }

    sd= sqrt(tot_squared / N - mean * mean);
    se= sd / sqrt(N);

    ii= N - 1;	/* Array offset starts at 0 */
    if (ii > 1 && ii < 6)   {
	tVal= tval[ii];
    } else if (ii <= 8)   {
	tVal= 2.306;
    } else if (ii <= 10)   {
	tVal= 2.228;
    } else if (ii <= 20)   {
	tVal= 2.086;
    } else if (ii <= 50)   {
	tVal= 2.009;
    } else if (ii <= 100)   {
	tVal= 1.984;
    } else   {
	tVal= 1.960;
    }

    halfCI= tVal * se;
    precision= halfCI / mean;

    return precision;

}  /* end of stat_p() */



static int
cmpdouble(const void *a, const void *b)
{

    if (*((double *)a) < *((double *)b))  {
	return -1;
    } else if (*((double *)a) > *((double *)b))  {
	return 1;
    } else   {
	return 0;
    }

}  /* end of cmpdouble() */



void
disp_stats(double *results, int cnt, double precision)
{

double min;
double max;
double med;
double avg;
double sd;
int i;


    min= 999999999.99;
    max= -999999999.99;
    avg= 0.0;
    med= 0.0;
    sd= 0.0;

    /* Sort the list */
    qsort(results, cnt, sizeof(double), &cmpdouble);

    for (i= 0; i < cnt; i++)   {
	avg= avg + results[i];
	if (results[i] < min)   {
	    min= results[i];
	}
	if (results[i] > max)   {
	    max= results[i];
	}
    }
    avg= avg / cnt;
    if (cnt % 2 == 0)   {
	/* even */
	med= (results[cnt / 2] + results[(cnt / 2) - 1]) / 2.0;;
    } else   {
	/* odd */
	med= results[cnt / 2];
    }

    /* Calculate the standard deviation */
    for (i= 0; i < cnt; i++)   {
	sd= sd + ((results[i] - avg) * (results[i] - avg));
    }
    sd= sqrt(sd / cnt);

    printf("%9.3f    %9.3f    %9.3f    %9.3f    %9.3f    %12.6f",
	min, avg, med, max, sd, precision);

}  /* end of disp_stats() */
#if STANDALONECPLUSPLUS
}
#endif
