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

#include "sstdisksim_posix_call.h"

sstdisksim_posix_calls::sstdisksim_posix_calls()
{
  strcpy(__calls[_CALL_READ].name, "READ");
  __calls[_CALL_READ].call = _CALL_READ;

  strcpy(__calls[_CALL_PREAD].name, "PREAD");
  __calls[_CALL_PREAD].call = _CALL_PREAD;

  strcpy(__calls[_CALL_PREAD64].name, "PREAD64");
  __calls[_CALL_PREAD64].call = _CALL_PREAD64;

  strcpy(__calls[_CALL_FREAD].name, "FREAD");
  __calls[_CALL_FREAD].call = _CALL_FREAD;

  strcpy(__calls[_CALL_READV].name, "READV");
  __calls[_CALL_READV].call = _CALL_READV;

  strcpy(__calls[_CALL_WRITE].name, "WRITE");
  __calls[_CALL_WRITE].call = _CALL_WRITE;

  strcpy(__calls[_CALL_PWRITE].name, "PWRITE");
  __calls[_CALL_PWRITE].call = _CALL_PWRITE;

  strcpy(__calls[_CALL_PWRITE64].name, "PWRITE64");
  __calls[_CALL_PWRITE64].call = _CALL_PWRITE64;

  strcpy(__calls[_CALL_WRITEV].name, "WRITEV");
  __calls[_CALL_WRITEV].call = _CALL_WRITEV;

  strcpy(__calls[_CALL_FWRITE].name, "FWRITE");
  __calls[_CALL_FWRITE].call = _CALL_FWRITE;

  strcpy(__calls[_CALL_FSYNC].name, "FSYNC");
  __calls[_CALL_FSYNC].call = _CALL_FSYNC;

  strcpy(__calls[_CALL_CLOSE].name, "CLOSE");
  __calls[_CALL_CLOSE].call = _CALL_CLOSE;

  strcpy(__calls[_CALL_FCLOSE].name, "FCLOSE");
  __calls[_CALL_FCLOSE].call = _CALL_FCLOSE;

  strcpy(__calls[_CALL_OPEN].name, "OPEN");
  __calls[_CALL_OPEN].call = _CALL_OPEN;

  strcpy(__calls[_CALL_OPEN64].name, "OPEN64");
  __calls[_CALL_OPEN64].call = _CALL_OPEN64;

  strcpy(__calls[_CALL_CREAT].name, "CREAT");
  __calls[_CALL_CREAT].call = _CALL_CREAT;

  strcpy(__calls[_CALL_CREAT64].name, "CREAT64");
  __calls[_CALL_CREAT64].call = _CALL_CREAT64;

  strcpy(__calls[_CALL_OPEN64].name, "FOPEN64");
  __calls[_CALL_OPEN64].call = _CALL_OPEN64;

  strcpy(__calls[_CALL_FOPEN].name, "FOPEN");
  __calls[_CALL_FOPEN].call = _CALL_FOPEN;

  strcpy(__calls[_CALL_LSEEK].name, "LSEEK");
  __calls[_CALL_LSEEK].call = _CALL_LSEEK;

  strcpy(__calls[_CALL_LSEEK64].name, "LSEEK64");
  __calls[_CALL_LSEEK64].call = _CALL_LSEEK64;

  strcpy(__calls[_CALL_FSEEK].name, "FSEEK");
  __calls[_CALL_FSEEK].call = _CALL_FSEEK;


  head=tail=NULL;
}

void
sstdisksim_posix_calls::add_entry(sstdisksim_posix_event* ev)
{
  sstdisksim_posix_call call;

  call.call = ev->call;
  call.args[_ARG_FD].l = ev->arg_fd;
  call.args[_ARG_COUNT].l = ev->arg_count;
  call.args[_ARG_OFFSET].l = ev->arg_offset;
  call.args[_ARG_WHENCE].l = ev->arg_whence;

  add_entry(call);
}

void
sstdisksim_posix_calls::add_entry(sstdisksim_posix_call call)
{
  if ( call.call > _END_CALLS )
  {
    exit(1);
    return;
  }

  struct sstdisksim_posix_call* cur = new sstdisksim_posix_call;
  cur->call = call.call;
  cur->next = NULL;


  for ( int i = 0; i < _END_ARGS; i++ )
  {
    cur->args[i].l = call.args[i].l;
  }

  if ( head == NULL )
    head = tail = cur;
  else
  { 
    tail->next = cur;
    tail = cur;
  }
}

sstdisksim_posix_call*
sstdisksim_posix_calls::pop_entry()
{
  sstdisksim_posix_call* retval = head;
  if ( head != NULL )
  {    
    head = head->next;
  }

  return retval;
}
