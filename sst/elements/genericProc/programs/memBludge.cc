// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdio.h>
#include <pthread.h>
#include "ppcPimCalls.h"

const int THR = 32;
const int BLK = 1024*64;
const int SZ = (1024*64) * 32 * 16;

pthread_t thr[32];
pthread_attr_t attr;
int *array;

void *tFunc(void* a) {
  int i;
  PIM_quickPrint((int)a,0,0);
  int offset = (int)a * BLK;
  for (i = 0; i < BLK; i+=8) {
    array[i + offset]++;
  }
  printf("test %d\n", (int)a); fflush(stdout);
  return 0;
}

int main() {
  void *ret;
  int i;

  array = (int*)malloc(SZ*sizeof(int)+32);
  printf("test\n"); fflush(stdout);

  for (i = 0; i < THR; ++i) {
    pthread_create(&thr[i], 0, tFunc, (void*)i);
  }
  for (i = 0; i < THR; ++i) {
    pthread_join(thr[i], &ret);
  }
  PIM_quickPrint(1,1,1);

  return 0;
}
