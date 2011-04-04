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

void 
sstdisksim_trace_entries::print_entries()
{
  struct sstdisksim_trace_type* cur = head;

  while ( cur != NULL )
  {  
    printf("%s ", cur->call->name);

    int i = 0;
    while ( i < _END_ARGS && (cur->call->fmt)[i] != '\0' )
    {
      switch (cur->call->fmt[i])
      {
      case 's':
	printf("string: %s ", cur->args[i].s);
	break;
      case 'l':
	printf("long: %ld ", cur->args[i].l);
	break;
      case 'i':
	printf("int: %d ", cur->args[i].i);
	break;
      case 'u':
	printf("uint: %u ", cur->args[i].u);
	break;
      case 't':
	printf("sizet: %Zd ", cur->args[i].t);
	break;
      case 'p':
	printf("pointer: %p ", cur->args[i].p);
	break;
      default:
	exit(-1);
      }
      i++;
    }

    printf("\n");

    cur = cur->next;
  }
}

sstdisksim_trace_entries::sstdisksim_trace_entries()
{
  strcpy(__calls[_CALL_READ].name, "READ");
  strcpy(__calls[_CALL_READ].fmt, "ipt");
  __calls[_CALL_READ].call = _CALL_READ;

  strcpy(__calls[_CALL_WRITE].name, "WRITE");
  strcpy(__calls[_CALL_WRITE].fmt, "ipt");
  __calls[_CALL_WRITE].call = _CALL_WRITE;

  strcpy(__calls[_CALL_FSYNC].name, "FSYNC");
  strcpy(__calls[_CALL_FSYNC].fmt, "ipt");
  __calls[_CALL_FSYNC].call = _CALL_FSYNC;

  strcpy(__calls[_CALL_OPEN].name, "OPEN");
  strcpy(__calls[_CALL_OPEN].fmt, "si");
  __calls[_CALL_OPEN].call = _CALL_OPEN;

  strcpy(__calls[_CALL_CLOSE].name, "CLOSE");
  strcpy(__calls[_CALL_CLOSE].fmt, "i");
  __calls[_CALL_CLOSE].call = _CALL_CLOSE;

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
    cur->args[i] = args[i];
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
    head = head->next;

  return retval;
}
