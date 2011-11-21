#ifndef _MY_ALLREDUCE_H_
#define _MY_ALLREDUCE_H_

#include "collective_patterns/collective_topology.h"


void my_allreduce(double *in, double *result, int msg_len, Collective_topology *ctopo);


#endif  // _MY_ALLREDUCE_H_
