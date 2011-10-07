#include <stdio.h>
#include <math.h>
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
