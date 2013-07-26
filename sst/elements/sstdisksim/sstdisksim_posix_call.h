// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SSTDISKSIM_POSIX_CALL_H_
#define _SSTDISKSIM_POSIX_CALL_H_

#include <stdlib.h>
#include <sst/core/serialization.h>
#include <sst/core/event.h>

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

struct sstdisksim_posix_call
{
  __call call;
  __argument args[_END_ARGS];
  sstdisksim_posix_call* next;
};

class sstdisksim_posix_event;
class sstdisksim_posix_calls
{
public:
  sstdisksim_posix_calls();
  void add_entry(sstdisksim_posix_event* ev);
  void add_entry(sstdisksim_posix_call call);
  sstdisksim_posix_call* pop_entry();

  sstdisksim_posix_call* head;
  sstdisksim_posix_call* tail;  

 private:
  sstdisksim_posix_name __calls[_END_CALLS];
};

class sstdisksim_posix_event : public SST::Event{
 public:
  __call call;
  long arg_fd;
  long arg_count;
  long arg_offset;
  long arg_whence;

  sstdisksim_posix_event* next_event;

 private:
  friend class boost::serialization::access;
  template<class Archive>
    void
    serialize( Archive & ar, const unsigned int version )
    {
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
      ar & BOOST_SERIALIZATION_NVP(arg_fd);
      ar & BOOST_SERIALIZATION_NVP(arg_count);
      ar & BOOST_SERIALIZATION_NVP(arg_offset);
      ar & BOOST_SERIALIZATION_NVP(arg_whence);
      ar & BOOST_SERIALIZATION_NVP(next_event);
    }
};

#endif // _SSTDISKSIM_POSIX_CALL_H_
