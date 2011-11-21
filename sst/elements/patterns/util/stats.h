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
