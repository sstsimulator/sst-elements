/*
** Some helper functions to calculate and print statistics
*/
#ifndef _STATS_H_
#define _STATS_H_

#include <list>

void print_stats(std::list<double> t);
void print_stats(std::list<double> t, double precision);
void print_stats(std::list<double> t, double precision, bool print_p);

#endif // _STATS_H_
