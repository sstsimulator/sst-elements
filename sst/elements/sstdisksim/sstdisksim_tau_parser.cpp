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

#include "sstdisksim_tau_parser.h"
#include <TAU_tf.h>
#include <iostream>
#include <stdio.h>
#include <map>
using namespace std;

typedef std::map<size_t, size_t> arg_map;
arg_map fd_map;

map< pair<int,int>, int, less< pair<int,int> > > EOF_Trace;
bool EndOfTrace = false; 

sstdisksim_trace_entries __list;
unsigned __types[_END_CALLS][_END_ARGS];
__argWithState __tmp_vals[_END_CALLS][_END_ARGS];

// This variable is used to hack our way around a TAU "feature" to allow spikes for viz
// tools.
unsigned __vizhack[_END_CALLS][_END_ARGS];

void __setargs( int i )
{
  // true only for those args that are not used in the call
  switch(i)
  {
  case _CALL_READ:
  case _CALL_WRITE:
    __tmp_vals[i][_ARG_FD].set = false;
    __tmp_vals[i][_ARG_COUNT].set = false;

    __tmp_vals[i][_ARG_OFFSET].set = true;
    __tmp_vals[i][_ARG_WHENCE].set = true;
    break;

  case _CALL_FSYNC:
  case _CALL_OPEN:
  case _CALL_CLOSE:
    __tmp_vals[i][_ARG_FD].set = false;

    __tmp_vals[i][_ARG_COUNT].set = true;
    __tmp_vals[i][_ARG_OFFSET].set = true;
    __tmp_vals[i][_ARG_WHENCE].set = true;
    break;

  case _CALL_LSEEK:
    __tmp_vals[i][_ARG_FD].set = false;
    __tmp_vals[i][_ARG_OFFSET].set = false;
    __tmp_vals[i][_ARG_WHENCE].set = false;

    __tmp_vals[i][_ARG_COUNT].set = true;
    break;
  default:
    exit(-1);
  }
}

// implementation of callback routines 
int EnterState(void *userData, double time, 
	       unsigned int nid, unsigned int tid, unsigned int stateid)
{
  return 0;
}

int LeaveState(void *userData, double time, unsigned int nid, unsigned int tid, unsigned int stateid)
{
  return 0;
}


int ClockPeriod( void*  userData, double clkPeriod )
{
  return 0;
}

int DefThread(void *userData, unsigned int nid, unsigned int threadToken,
	      const char *threadName )
{
  return 0;
}

int EndTrace( void *userData, unsigned int nid, unsigned int threadid)
{
  //  EOF_Trace[pair<int,int> (nid,threadid) ] = 1; /* flag it as over */
  /* yes, it is over */
  map < pair<int, int>, int, less< pair<int,int> > >::iterator it;
  EndOfTrace = true; /* Lets assume that it is over */
  for (it = EOF_Trace.begin(); it != EOF_Trace.end(); it++)
  { /* cycle through all <nid,tid> pairs to see if it really over */
    if ((*it).second == 0)
      {
	EndOfTrace = false; /* not over! */
      /* If there's any processor that is not over, then the trace is not over */
      }
  }
  return 0;
}

int DefStateGroup( void *userData, unsigned int stateGroupToken, 
		   const char *stateGroupName )
{
  return 0;
}

int DefState( void *userData, unsigned int stateToken, const char *stateName, 
	      unsigned int stateGroupToken )
{
  return 0;
}

// This is where to add new events for tau
int DefUserEvent( void *userData, unsigned int userEventToken,
		  const char *userEventName, int monotonicallyIncreasing )
{
  if ( strstr(userEventName, "READ") )
  {
    if ( strstr(userEventName, "fd" ) ) 
    {
      __types[_CALL_READ][_ARG_FD] = userEventToken;
    }
    else if ( strstr(userEventName, "Bytes" ) )
    {
      __types[_CALL_READ][_ARG_COUNT] = userEventToken;
    }
  }
  else if ( strstr(userEventName, "WRITE") )
  {
    if ( strstr(userEventName, "fd" ) )
    {
      __types[_CALL_WRITE][_ARG_FD] = userEventToken;
    }
    else if ( strstr(userEventName, "Bytes" ) )
    {
      __types[_CALL_WRITE][_ARG_COUNT] = userEventToken;
    }    
  }
  else if ( strstr(userEventName, "CLOSE") )
  {
    if ( strstr(userEventName, "fd" ) )
    {
      __types[_CALL_CLOSE][_ARG_FD] = userEventToken;
    }
  }
  else if ( strstr(userEventName, "LSEEK") )
  {
    if ( strstr(userEventName, "fd" ) )
    {
      __types[_CALL_LSEEK][_ARG_FD] = userEventToken;
    }
    if ( strstr(userEventName, "offset" ) )
    {
      __types[_CALL_LSEEK][_ARG_OFFSET] = userEventToken;
    }
    if ( strstr(userEventName, "whence" ) )
    {
      __types[_CALL_LSEEK][_ARG_WHENCE] = userEventToken;
    }
  }
  else if ( strcmp(userEventName, "\"Bytes Read\"")==0 )
  {
      __types[_CALL_READ][_ARG_COUNT] = userEventToken;    
  }
  else if ( strcmp(userEventName, "\"Bytes Written\"")==0 )
  {
      __types[_CALL_WRITE][_ARG_COUNT] = userEventToken;
  }
  else
  {
  }


  return 0;
}

int EventTrigger( void *userData, double time, 
		  unsigned int nodeToken,
		  unsigned int threadToken,
		  unsigned int userEventToken,
		  long long userEventValue)
{
  for ( int i = 0; i < _END_CALLS; i++ )
  {
    for ( int j = 0; j < _END_ARGS; j++ )
    {
      if ( userEventToken == __types[i][j] )
      {
	if ( __vizhack[i][j] == 1 )
	{
	  // A real value.  
	  __tmp_vals[i][j].arg.l = userEventValue;
	   __tmp_vals[i][j].set = true;
	  for ( int k = 0; k < _END_ARGS; k++ )
	  {
	    if ( __tmp_vals[i][k].set == false )
	      break;
	    else if ( k == _END_ARGS-1 )
	    {
	      __argument tmp[_END_ARGS];
	      for ( int l = 0; l < _END_ARGS; l++ )
	      {
		tmp[l].l = __tmp_vals[i][l].arg.l;
	      }

	      __list.add_entry((__call)i, tmp);

	      __setargs(i);
	      
	    }
	  }
	}

	__vizhack[i][j]++;
	__vizhack[i][j] = __vizhack[i][j] % 3;
      }
    }
  }

  return 0;
}

int SendMessage ( void*  userData,
		  double time,
		  unsigned int sourceNodeToken, 
		  unsigned int sourceThreadToken, 
		  unsigned int destinationNodeToken,
		  unsigned int destinationThreadToken,
		  unsigned int messageSize,
		  unsigned int messageTag,
		  unsigned int messageComm
		  )
{
  return 0;
}

int RecvMessage ( void*  userData,
		  double time,
		  unsigned int sourceNodeToken, 
		  unsigned int sourceThreadToken, 
		  unsigned int destinationNodeToken,
		  unsigned int destinationThreadToken,
		  unsigned int messageSize,
		  unsigned int messageTag,
		  unsigned int messageComm
		  )
{
  return 0;
}

sstdisksim_tau_parser::sstdisksim_tau_parser(const char* trc_file, const char* edf_file)
{
  Ttf_FileHandleT fh;

  int recs_read, pos;
  Ttf_CallbacksT cb;

  for ( int i = 0; i < _END_CALLS; i++ )
  {
    for ( int j = 0; j < _END_ARGS; j++ )
    {
      __vizhack[i][j] = 0;
      __types[i][j]=0;
    }
    __setargs(i);
  }

  // open trace file 
  fh = Ttf_OpenFileForInput(trc_file, edf_file);
  if (!fh)
  {
    printf("ERROR:Ttf_OpenFileForInput fails");
    exit(1);
  }

  // Fill the callback struct 
  cb.UserData = 0;
  cb.DefClkPeriod = ClockPeriod;
  cb.DefThread = DefThread;
  cb.DefStateGroup = DefStateGroup;
  cb.DefState = DefState;
  cb.EndTrace = EndTrace;
  cb.EnterState = EnterState;
  cb.LeaveState = LeaveState;
  cb.DefUserEvent = DefUserEvent;
  cb.EventTrigger = EventTrigger;
  cb.SendMessage = SendMessage;
  cb.RecvMessage = RecvMessage;

  pos = Ttf_RelSeek(fh,2);

  /* Go through each record until the end of the trace file */
  do {
    recs_read = Ttf_ReadNumEvents(fh,cb, 1024);
  }
  while ((recs_read >=0) && (!EndOfTrace));

  Ttf_CloseFile(fh);
}

/******************************************************************************/
sstdisksim_tau_parser::~sstdisksim_tau_parser()
{
}

/******************************************************************************/
sstdisksim_event*
sstdisksim_tau_parser::getNextEvent()
{
  sstdisksim_event* ev = new sstdisksim_event();
  ev->completed = 0;
  ev->devno = 0;  // TODO: figure out device used

  long long cur_pos = 0;
  arg_map::iterator iter;
  
  sstdisksim_trace_type* cur_event = __list.pop_entry();
  
  bool looping = true;
  while ( looping == true && cur_event != NULL )
  {
    switch(cur_event->call->call)
    {
    case _CALL_READ:
      ev->etype = DISKSIMREAD;
      ev->count = cur_event->args[_ARG_COUNT].t;
      ev->pos = 0;  

      iter = fd_map.find(cur_event->args[_ARG_FD].t);

      if ( iter != fd_map.end() )
      {
	ev->pos =  iter->second;
	fd_map.erase(cur_event->args[_ARG_FD].t);
      }

      //      printf("READ pos %lld \n", ev->pos);

      fd_map.insert(std::pair<size_t, long>(cur_event->args[_ARG_FD].t,
					    cur_event->args[_ARG_COUNT].t + ev->pos));
      looping = false;

      break;

    case _CALL_WRITE:
      ev->etype = DISKSIMWRITE;
      ev->count = cur_event->args[_ARG_COUNT].t;
      iter = fd_map.find(cur_event->args[_ARG_FD].t);
      ev->pos = 0;  

      if ( iter != fd_map.end() )
      {
	ev->pos =  iter->second;
	fd_map.erase(cur_event->args[_ARG_FD].t);
      }
      
      //      printf("WRITE pos %lld \n", ev->pos);

      fd_map.insert(std::pair<size_t, long>(cur_event->args[_ARG_FD].t,
					    cur_event->args[_ARG_COUNT].t + ev->pos));
      looping = false;
      break;

    case _CALL_CLOSE:
      fd_map.erase(cur_event->args[_ARG_FD].t);
      free(cur_event);
      cur_event = __list.pop_entry();
      break;

    case _CALL_LSEEK:
      switch ( cur_event->args[_ARG_WHENCE].i )
	{
	case 0: //seek_set
	  cur_pos = cur_event->args[_ARG_OFFSET].l;
	  break;
	case 1: //seek_cur
	  iter = fd_map.find(cur_event->args[_ARG_FD].t);
	  cur_pos = cur_event->args[_ARG_OFFSET].l + iter->second;
	  break;
	  
	case 2: //seek_end - unsupported because we don't know file sizes
	default:
	  cur_pos = 0;
	  break;
	}
      
      iter = fd_map.find(cur_event->args[_ARG_FD].t);
      fd_map.erase(cur_event->args[_ARG_FD].t);
      fd_map.insert(std::pair<size_t, long>(cur_event->args[_ARG_FD].t,
					    cur_pos));
      //      printf("LSEEK pos %lld \n", cur_pos);
      free(cur_event);
      cur_event = __list.pop_entry();

      break;    

    case _CALL_FSYNC:
    case _CALL_OPEN:
    default:
      free(cur_event);
      cur_event = __list.pop_entry();
      break;
    }
  }

  if ( cur_event == NULL )
  {
    free(ev);
    return NULL;
  }

  free(cur_event);
  
  
  /*
  //skippy test override
  static int __i = 0;
  static int __pos = 0;
  if ( __i < 1000 )
  {
    __pos += __i*512;
    ev->etype = DISKSIMWRITE;
    ev->pos = 512 + __pos;
    ev->count = 512;  
    __i++;
  }
  else 
    return NULL;
  */

  return ev;
}
