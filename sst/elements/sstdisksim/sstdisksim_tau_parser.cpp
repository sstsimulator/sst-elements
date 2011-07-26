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

#include <sst/core/component.h>
#include <sst/core/simulation.h>


using namespace std;

map< pair<int,int>, int, less< pair<int,int> > > EOF_Trace;
bool EndOfTrace = false; 

sstdisksim_posix_calls __list;
unsigned __types[_END_CALLS][_END_ARGS];
__argWithState __tmp_vals[_END_CALLS][_END_ARGS];

SST::Link* __model;


// This variable is used to hack our way around a TAU "feature" to allow spikes for viz
// tools.
unsigned __vizhack[_END_CALLS][_END_ARGS];

/******************************************************************************/
void __setargs( int i )
{
  // true only for those args that are not used in the call
  switch(i)
  {
  case _CALL_FREAD:
  case _CALL_READV:
  case _CALL_READ:
  case _CALL_WRITE:
  case _CALL_WRITEV:
  case _CALL_FWRITE:
    __tmp_vals[i][_ARG_FD].set = false;
    __tmp_vals[i][_ARG_COUNT].set = false;

    __tmp_vals[i][_ARG_OFFSET].set = true;
    __tmp_vals[i][_ARG_WHENCE].set = true;
    break;

  case _CALL_FSYNC:
  case _CALL_CREAT:
  case _CALL_CREAT64:
  case _CALL_FOPEN:
  case _CALL_FOPEN64:
  case _CALL_OPEN:
  case _CALL_OPEN64:
  case _CALL_CLOSE:
  case _CALL_FCLOSE:
    __tmp_vals[i][_ARG_FD].set = false;

    __tmp_vals[i][_ARG_COUNT].set = true;
    __tmp_vals[i][_ARG_OFFSET].set = true;
    __tmp_vals[i][_ARG_WHENCE].set = true;
    break;

  case _CALL_FSEEK:
  case _CALL_LSEEK:
  case _CALL_LSEEK64:
    __tmp_vals[i][_ARG_FD].set = false;
    __tmp_vals[i][_ARG_OFFSET].set = false;
    __tmp_vals[i][_ARG_WHENCE].set = false;

    __tmp_vals[i][_ARG_COUNT].set = true;
    break;

  case _CALL_PREAD:
  case _CALL_PREAD64:
  case _CALL_PWRITE:
  case _CALL_PWRITE64:
    __tmp_vals[i][_ARG_FD].set = false;
    __tmp_vals[i][_ARG_OFFSET].set = false;
    __tmp_vals[i][_ARG_COUNT].set = false;

    __tmp_vals[i][_ARG_WHENCE].set = true;
    break;

  default:
    printf("Arg %d invalid \n", i);
    exit(-1);
    break;
  }
}

/******************************************************************************/
// implementation of callback routines 
int EnterState(void *userData, double time, 
	       unsigned int nid, unsigned int tid, unsigned int stateid)
{
  return 0;
}

/******************************************************************************/
int LeaveState(void *userData, double time, unsigned int nid, unsigned int tid, unsigned int stateid)
{
  return 0;
}


/******************************************************************************/
int ClockPeriod( void*  userData, double clkPeriod )
{
  return 0;
}

/******************************************************************************/
int DefThread(void *userData, unsigned int nid, unsigned int threadToken,
	      const char *threadName )
{
  return 0;
}

/******************************************************************************/
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

/******************************************************************************/
int DefStateGroup( void *userData, unsigned int stateGroupToken, 
		   const char *stateGroupName )
{
  return 0;
}

/******************************************************************************/
int DefState( void *userData, unsigned int stateToken, const char *stateName, 
	      unsigned int stateGroupToken )
{
  return 0;
}

/******************************************************************************/
// This is where to add new events for tau
int DefUserEvent( void *userData, unsigned int userEventToken,
		  const char *userEventName, int monotonicallyIncreasing )
{
  int call = -1;

  // get call - note that read should follow fread, et cetera
  // as they are different calls.
  if ( strstr ( userEventName, "FREAD") )
    call = _CALL_FREAD;
  else if ( strstr ( userEventName, "READV") )
    call = _CALL_READV;
  else if ( strstr ( userEventName, "READ") )
    call = _CALL_READ;

  else if ( strstr ( userEventName, "FWRITE") )
    call = _CALL_FWRITE;
  else if ( strstr ( userEventName, "WRITEV") )
    call = _CALL_WRITEV;
  else if ( strstr ( userEventName, "WRITE") )
    call = _CALL_WRITE;

  else if ( strstr ( userEventName, "FSYNC") )
    call = _CALL_FSYNC;

  else if ( strstr ( userEventName, "CREAT64") )
    call = _CALL_CREAT64;
  else if ( strstr ( userEventName, "CREAT") )
    call = _CALL_CREAT;

  else if ( strstr ( userEventName, "FOPEN64") )
    call = _CALL_FOPEN64;
  else if ( strstr ( userEventName, "FOPEN") )
    call = _CALL_FOPEN;

  else if ( strstr ( userEventName, "OPEN64") )
    call = _CALL_OPEN64;
  else if ( strstr ( userEventName, "OPEN") )
    call = _CALL_OPEN;

  else if ( strstr ( userEventName, "FCLOSE") )
    call = _CALL_FCLOSE;
  else if ( strstr ( userEventName, "CLOSE") )
    call = _CALL_CLOSE;
 
  else if ( strstr ( userEventName, "LSEEK64") )
    call = _CALL_LSEEK64;
  else if ( strstr ( userEventName, "LSEEK") )
    call = _CALL_LSEEK;

  else if ( strstr ( userEventName, "FSEEK") )
    call = _CALL_FSEEK;

  else if ( strstr ( userEventName, "PREAD64") )
    call = _CALL_PREAD64;
  else if ( strstr ( userEventName, "PREAD") )
    call = _CALL_PREAD;

  else if ( strstr ( userEventName, "PWRITE64") )
    call = _CALL_PWRITE64;
  else if ( strstr ( userEventName, "PWRITE") )
    call = _CALL_PWRITE;

  if ( call < 0 )
    return 0;

  // get arg
  if ( strstr(userEventName, "fd" ) ) 
  {
      __types[call][_ARG_FD] = userEventToken;
      //      printf("call %d arg %d event %d\n", call, _ARG_FD, userEventToken);
  }
  else if ( strstr(userEventName, "ret" ) )
  {
    __types[call][_ARG_COUNT] = userEventToken;
    //    printf("call %d arg %d event %d\n", call, _ARG_COUNT, userEventToken);
  }
  else if ( strstr(userEventName, "offset" ) )
  {
    __types[call][_ARG_OFFSET] = userEventToken;
    //    printf("call %d arg %d event %d\n", call, _ARG_OFFSET, userEventToken);
  }
  else if ( strstr(userEventName, "whence" ) )
  {
    __types[call][_ARG_WHENCE] = userEventToken;
    //    printf("call %d arg %d event %d\n", call, _ARG_WHENCE, userEventToken);
  }


  return 0;
}

/******************************************************************************/
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
	   //	   printf("event %u value %lld\n", userEventToken, userEventValue);
	  for ( int k = 0; k < _END_ARGS; k++ )
	  {
	    if ( __tmp_vals[i][k].set == false )
	      break;
	    else if ( k == _END_ARGS-1 )
	    {
	      sstdisksim_posix_event* call = new sstdisksim_posix_event;
	      call->call = (__call)i;
	      call->arg_fd = __tmp_vals[i][_ARG_FD].arg.l;
	      call->arg_count = __tmp_vals[i][_ARG_COUNT].arg.l;
	      call->arg_whence = __tmp_vals[i][_ARG_WHENCE].arg.l;
	      call->arg_offset = __tmp_vals[i][_ARG_OFFSET].arg.l;
	      
      	      __model->Send(0, call);

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

/******************************************************************************/
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

/******************************************************************************/
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

/******************************************************************************/
sstdisksim_tau_parser::sstdisksim_tau_parser(const char* trc_file, const char* edf_file,
					     SST::Link* model)
{
  Ttf_FileHandleT fh;

  int recs_read, pos;
  Ttf_CallbacksT cb;

  __model = model;
  printf("%p\n", __model);

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

