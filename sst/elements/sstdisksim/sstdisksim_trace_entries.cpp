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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sstdisksim_trace_entries.h"

sstdisksim_trace_entries::sstdisksim_trace_entries()
{
  strcpy(__calls[_CALL_READ].name, "READ");
  __calls[_CALL_READ].call = _CALL_READ;

  strcpy(__calls[_CALL_WRITE].name, "WRITE");
  __calls[_CALL_WRITE].call = _CALL_WRITE;

  strcpy(__calls[_CALL_FSYNC].name, "FSYNC");
  __calls[_CALL_FSYNC].call = _CALL_FSYNC;

  strcpy(__calls[_CALL_CLOSE].name, "CLOSE");
  __calls[_CALL_CLOSE].call = _CALL_CLOSE;

  strcpy(__calls[_CALL_OPEN].name, "OPEN");
  __calls[_CALL_OPEN].call = _CALL_OPEN;

  strcpy(__calls[_CALL_LSEEK].name, "LSEEK");
  __calls[_CALL_LSEEK].call = _CALL_LSEEK;

  head=tail=NULL;
}

void
sstdisksim_trace_entries::add_entry(__call call, __argument args[_END_ARGS])
{
  if ( call > _END_CALLS )
  {
    exit(1);
    return;
  }

  struct sstdisksim_trace_type* cur = new sstdisksim_trace_type;
  cur->call = &(__calls[call]);
  cur->next = NULL;


  for ( int i = 0; i < _END_ARGS; i++ )
  {
    cur->args[i].l = args[i].l;
  }

  if ( head == NULL )
    head = tail = cur;
  else
  { 
    tail->next = cur;
    tail = cur;
  }
}

sstdisksim_trace_type*
sstdisksim_trace_entries::pop_entry()
{
  sstdisksim_trace_type* retval = head;
  if ( head != NULL )
  {    
    head = head->next;
  }

  return retval;
}
