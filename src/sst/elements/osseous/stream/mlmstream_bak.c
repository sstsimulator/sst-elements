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

#include "mlm.h"
#include <memory.h>
#include <sint.h>
#include <stdio.h>
#include <stdlib.h>
#include <uint.h>
#include <utility>
#include <vector>
#include <inttypes.h>

int main(int argc, char *argv[]) {

  const int LENGTH = 32768;

  ariel_enable();

  printf("Allocating arrays of size %d elements.\n", LENGTH);
  double *a = (double *)mlm_malloc(sizeof(double) * LENGTH, 0);
  double *b = (double *)mlm_malloc(sizeof(double) * LENGTH, 0);
  double *fast_c = (double *)mlm_malloc(sizeof(double) * LENGTH, 0);

  UInt<4> io_ins_0, io_ins_1, io_ins_2, io_ins_3, io_out;
  UInt<1> io_load, io_shift;
  UInt<4> *inp_ptr;
  UInt<1> *ctrl_ptr;
  //TYPEINFO *inp_info, *ctrl_info;

  mlm_set_pool(1);

  printf("Allocation for fast_c is %llu\n", (unsigned long long int)fast_c);
  double *c = (double *)malloc(sizeof(double) * LENGTH);
  printf("Done allocating arrays.\n");

  int i;
  for (i = 0; i < LENGTH; ++i) {
    a[i] = i;
    b[i] = LENGTH - i;
    c[i] = 0;
  }

  // Issue a memory copy
  mlm_memcpy(fast_c, c, sizeof(double) * LENGTH);

  printf("Perfoming the fast_c compute loop...\n");
#pragma omp parallel for
  for (i = 0; i < LENGTH; ++i) {
    // printf("issuing a write to: %llu (fast_c)\n", ((unsigned long long int)
    // &fast_c[i]));
    fast_c[i] = 2.0 * a[i] + 1.5 * b[i];
  }

  // Now copy results back
  mlm_Tag copy_tag = mlm_memcpy(c, fast_c, sizeof(double) * LENGTH);
  mlm_waitComplete(copy_tag);

  io_shift = UInt<1>(1);
  io_ins_0 = UInt<4>(9);
  io_ins_1 = UInt<4>(4);
  io_ins_2 = UInt<4>(2);
  io_ins_3 = UInt<4>(7);
  io_load = UInt<1>(0);

  size_t inp_size = sizeof(UInt<4>) * 5;
  size_t ctrl_size = sizeof(UInt<1>) * 2;
  RTL_shmem_info *shmem = new RTL_shmem_info(inp_size, ctrl_size);

  inp_ptr = (UInt<4>*)shmem->get_inp_ptr();
  ctrl_ptr = (UInt<1>*)shmem->get_ctrl_ptr();

  TYPEINFO inp_info; // = new TYPEINFO();   
  TYPEINFO ctrl_info; // = new TYPEINFO(); 

  ctrl_info.push_back(make_pair("UInt", 1));
  memcpy(ctrl_ptr, (void *)(&io_shift), sizeof(UInt<1>));
  //*ctrl_ptr = io_shift;
  ctrl_ptr++;

  inp_info.push_back(make_pair("UInt", 4));
  //mlm_memcpy(inp_ptr, (void *)(&io_ins_0), sizeof(UInt<4>));
  *inp_ptr = io_ins_0;
  inp_ptr++;

  inp_info.push_back(make_pair("UInt", 4));
  memcpy(inp_ptr, (void *)(&io_ins_1), sizeof(UInt<4>));
  //*inp_ptr = io_ins_1;
  inp_ptr++;

  inp_info.push_back(make_pair("UInt", 4));
  memcpy(inp_ptr, (void *)(&io_ins_2), sizeof(UInt<4>));
  //*inp_ptr = io_ins_2;
  inp_ptr++;

  inp_info.push_back(make_pair("UInt", 4));
  memcpy(inp_ptr, (void *)(&io_ins_3), sizeof(UInt<4>));
  //*inp_ptr = io_ins_3;

  ctrl_info.push_back(make_pair("UInt", 1));
  memcpy(ctrl_ptr, (void *)(&io_load), sizeof(UInt<1>));
  //*ctrl_ptr = io_load;

  memcpy((void*)shmem->get_inp_info(), (void*)(&inp_info), sizeof(inp_info));
  //*shmem->get_inp_info() = inp_info;
  //*shmem->get_ctrl_info() = ctrl_info;
  memcpy((void*)shmem->get_ctrl_info(), (void*)(&ctrl_info), sizeof(ctrl_info));

  Update_RTL_Params *params = new Update_RTL_Params();
  params->storetomem(shmem);
  params->check(shmem);
  //fprintf(stderr, "\nGoing to call start RTL sim");
  //fprintf(stderr, "\nSize of rtl_inp_info in mlmstream is: %" PRIu64, shmem->get_inp_info()->size());
  ariel_fence();
  start_RTL_sim(shmem);
  bool *check = (bool *)shmem->get_inp_ptr();
  fprintf(stderr, "\n\n Checking mlmstream: %d\n", *check);
  printf("\nstart_RTL_sim called\n");

  io_load = UInt<1>(1);
  memcpy(ctrl_ptr, (void *)(&io_load), sizeof(UInt<1>));

  params->perform_update(false, true, true, true, true, false, false, 1);
  params->storetomem(shmem);
  update_RTL_signals(shmem);
  printf("\nupdate_RTL_signals called\n");

  io_load = UInt<1>(0);
  memcpy(ctrl_ptr, (void *)(&io_load), sizeof(UInt<1>));

  params->perform_update(false, true, true, true, true, true, true, 5);
  params->storetomem(shmem);
  update_RTL_signals(shmem);
  io_ins_3 = UInt<4>(5);
  //memcpy(inp_ptr, (void *)(&io_ins_3), sizeof(UInt<4>));
  update_RTL_signals(shmem);
  io_ins_3 = UInt<4>(9);
  //memcpy(inp_ptr, (void *)(&io_ins_3), sizeof(UInt<4>));
  update_RTL_signals(shmem);

  delete shmem;
  delete params;
  //delete inp_info;
  //delete ctrl_info;

  double sum = 0;
  for (i = 0; i < LENGTH; ++i) {
    sum += c[i];
  }

  printf("Sum of arrays is: %f\n", sum);
  printf("Freeing arrays...\n");

  mlm_free(a);
  mlm_free(b);
  free(c);

  printf("Done.\n");

  return 0;
}
