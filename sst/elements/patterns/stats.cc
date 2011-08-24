/*
** Some helper functions to calculate and print statistics
*/
#include <stdio.h>
#include <math.h>
#include "stats.h"



void
print_stats(std::list<double> t)
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
		med= *it;
	    }
	    avg= avg + *it;
	    cnt++;
	}
	avg= avg / num_sets;

	// Calulate the standard deviation
	sd= 0.0;
	for (it= t.begin(); it != t.end(); it++)   {
	    sd= sd + ((*it - avg) * (*it - avg));
	}
	sd= sqrt(sd / num_sets);
	min= t.front();
	max= t.back();

	printf("%12.9f %12.9f %12.9f %12.9f %12.9f\n", min, avg, med, max, sd);

}  /* end of stats() */



