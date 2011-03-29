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
    while ( i < __ARG_MAX && (cur->call->fmt)[i] != '\0' )
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
  strcpy(__calls[READ].name, "READ");
  strcpy(__calls[READ].fmt, "ipt");

  strcpy(__calls[WRITE].name, "WRITE");
  strcpy(__calls[WRITE].fmt, "ipt");

  strcpy(__calls[FSYNC].name, "FSYNC");
  strcpy(__calls[FSYNC].fmt, "ipt");

  strcpy(__calls[OPEN].name, "OPEN");
  strcpy(__calls[OPEN].fmt, "si");

  strcpy(__calls[CLOSE].name, "CLOSE");
  strcpy(__calls[CLOSE].fmt, "i");

  head=tail=NULL;
}

void
sstdisksim_trace_entries::add_entry(__call call, __argument args[__ARG_MAX])
{
  if ( call > END )
  {
    exit(1);
    return;
  }

  struct sstdisksim_trace_type* cur = new sstdisksim_trace_type;
  cur->call = &(__calls[call]);
  cur->next = NULL;

  for ( int i = 0; i < __ARG_MAX; i++ )
    cur->args[i] = args[i];

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
