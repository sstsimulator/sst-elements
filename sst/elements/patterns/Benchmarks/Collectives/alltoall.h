#ifndef _MY_ALLTOALL_H_
#define _MY_ALLTOALL_H_


void my_alltoall(double *in, double *result, int msg_len, int nranks, int my_rank);


#endif  // _MY_ALLTOALL_H_
