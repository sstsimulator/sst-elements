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

#ifndef _SSTDISKSIM_TRACE_ENTRIES_H_
#define _SSTDISKSIM_TRACE_ENTRIES_H_

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
  _CALL_WRITE,
  _CALL_FSYNC,
  _CALL_CLOSE,
  _CALL_OPEN,
  _END_CALLS
};

enum __arg
{
  _ARG_FD = 0,
  _ARG_COUNT,
  //  _ARG_RET,
  _END_ARGS
};

struct sstdisksim_trace_call
{
  __call call;
  char name[16];
  char fmt[16];
};

struct sstdisksim_trace_type
{
  sstdisksim_trace_call* call;
  __argument args[_END_ARGS];
  sstdisksim_trace_type* next;
};

class sstdisksim_trace_entries
{
public:
  sstdisksim_trace_entries();
  void print_entries();
  void add_entry(__call call, __argument args[_END_ARGS]);
  sstdisksim_trace_type* pop_entry();

  sstdisksim_trace_type* head;
  sstdisksim_trace_type* tail;  
  sstdisksim_trace_call __calls[_END_CALLS];
};

#endif // _SSTDISKSIM_TRACE_ENTRIES_H_
