#ifndef _PFFT_R2_DIT_H_
#define _PFFT_R2_DIT_H_

#include "timestats_structures.h"

time_stat* pfft_r2_dit(int N, int validation,int numranks,int myrank,int rc);
void usage(char *pname);

#endif  /*  _PFFT_R2_DIT_H_ */
