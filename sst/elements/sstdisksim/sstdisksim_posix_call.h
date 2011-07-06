// Copyright 2011 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2011, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SSTDISKSIM_POSIX_CALL_H_
#define _SSTDISKSIM_POSIX_CALL_H_

#include <stdlib.h>

union __argument 
{
  char* s;
  long long l;
  int i;
  unsigned int u;
  size_t t;
  void* p;
};

struct __argWithState
{
  __argument arg;
  bool set;
};

enum __call
{
  _CALL_READ = 0,
  _CALL_FREAD,
  _CALL_PREAD,
  _CALL_PREAD64,
  _CALL_READV,
  _CALL_FWRITE,
  _CALL_WRITE,
  _CALL_WRITEV,
  _CALL_PWRITE,
  _CALL_PWRITE64,
  _CALL_FSYNC,
  _CALL_CLOSE,
  _CALL_FCLOSE,
  _CALL_OPEN,
  _CALL_OPEN64,
  _CALL_CREAT,
  _CALL_CREAT64,
  _CALL_FOPEN,
  _CALL_FOPEN64,
  _CALL_LSEEK,
  _CALL_LSEEK64,
  _CALL_FSEEK,
  _END_CALLS
};

enum __arg
{
  _ARG_FD = 0,
  _ARG_COUNT,
  _ARG_OFFSET,
  _ARG_WHENCE,
  _END_ARGS
};

struct sstdisksim_posix_name
{
  __call call;
  char name[16];
};

struct sstdisksim_posix_type
{
  sstdisksim_posix_name* call;
  __argument args[_END_ARGS];
  sstdisksim_posix_type* next;
};

class sstdisksim_posix_call
{
public:
  sstdisksim_posix_call();
  void add_entry(__call call, __argument args[_END_ARGS]);
  sstdisksim_posix_type* pop_entry();

  sstdisksim_posix_type* head;
  sstdisksim_posix_type* tail;  
  sstdisksim_posix_name __calls[_END_CALLS];
};

#endif // _SSTDISKSIM_POSIX_CALL_H_
