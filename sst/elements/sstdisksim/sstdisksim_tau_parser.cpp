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
using namespace std;

map< pair<int,int>, int, less< pair<int,int> > > EOF_Trace;
bool EndOfTrace = false; 

sstdisksim_trace_entries __list;
unsigned __types[END_CALLS][END_ARGS];
__argWithState __tmp_vals[END_CALLS][END_ARGS];

// This variable is used to hack our way around a TAU "feature" to allow spikes for viz
// tools.
unsigned __vizhack[END_CALLS][END_ARGS];


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
      __types[READ][FD] = userEventToken;
      printf ( "RF %d %d\n", userEventToken, __types[READ][FD] );
    }
    else if ( strstr(userEventName, "Bytes" ) )
    {
      __types[READ][COUNT] = userEventToken;
      printf ( "RC %d %d\n", userEventToken, __types[READ][COUNT] );
    }
  }
  else if ( strstr(userEventName, "WRITE") )
  {
    if ( strstr(userEventName, "fd" ) )
    {
      __types[WRITE][FD] = userEventToken;
      printf ( "WF %d %d\n", userEventToken, __types[WRITE][FD] );
    }
    else if ( strstr(userEventName, "Bytes" ) )
    {
      __types[WRITE][COUNT] = userEventToken;
      printf ( "WC %d %d\n", userEventToken, __types[WRITE][COUNT] );
    }    
  }

  return 0;
}

int EventTrigger( void *userData, double time, 
		  unsigned int nodeToken,
		  unsigned int threadToken,
		  unsigned int userEventToken,
		  long long userEventValue)
{
  for ( int i = 0; i < END_CALLS; i++ )
  {
    for ( int j = 0; j < END_ARGS; j++ )
    {
      if ( userEventToken == __types[i][j] )
      {
	printf("Cur Token: %zd\n", userEventValue);
	if ( __vizhack[i][j] == 1 )
	{
	  // A real value.  
	  __tmp_vals[i][j].arg.l = userEventValue;
	   __tmp_vals[i][j].set = true;
	  for ( int k = 0; k < END_ARGS; k++ )
	  {
	    if ( __tmp_vals[i][k].set == false )
	      break;
	    else if ( k == END_ARGS-1 )
	    {
	      __argument tmp[END_ARGS];
	      for ( int a = 0; a < END_ARGS; a++ )
	      {
		tmp[a].l = __tmp_vals[i][a].arg.l;
		printf("Value: %zd\n", tmp[a].t);
	      }
	      __list.add_entry((__call)i, tmp);
	      for ( int a = 0; a < END_ARGS; a++ )
		__tmp_vals[i][a].set = false;
	    }
	  }
	}
	else
	{
	  // A throwout value because of viz hacks in the trace files.
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

  for ( int i = 0; i < END_CALLS; i++ )
    for ( int j = 0; j < END_ARGS; j++ )
    {
      __tmp_vals[i][j].set = false;
      __vizhack[i][j] = 0;
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
    if (recs_read != 0)
      cout <<"Read "<<recs_read<<" records"<<endl;
  }
  while ((recs_read >=0) && (!EndOfTrace));

  Ttf_CloseFile(fh);
}

/******************************************************************************/
sstdisksim_tau_parser::~sstdisksim_tau_parser()
{
  printf("READ fd: %d bytes: %d WRITE fd: %d bytes: %d\n", __types[READ][FD],
	 __types[READ][COUNT], __types[WRITE][FD], __types[WRITE][COUNT]);
  
}

/******************************************************************************/
sstdisksim_event*
sstdisksim_tau_parser::getNextEvent()
{
  sstdisksim_event* ev = new sstdisksim_event();
  ev->completed = 0;
  ev->devno = 0;  // TODO: figure out device used
  ev->pos = 0;  // TODO: figure out position

  
  sstdisksim_trace_type* cur_event = __list.pop_entry();
 
  if ( cur_event == NULL )
    return NULL;

  bool looping = true;
  while ( looping == true && cur_event != NULL )
  {
    switch(cur_event->call->call)
    {
    case READ:
      ev->etype = DISKSIMREAD;
      ev->count = cur_event->args[2].t;
      looping = false;
      printf("Sending read event with count %zd\n", ev->count);
      break;
    case WRITE:
      ev->etype = DISKSIMWRITE;
      ev->count = cur_event->args[2].t;
      looping = false;
      printf("Sending write event with count %zd\n", ev->count);
      break;
    default:
      free(cur_event);
      cur_event = __list.pop_entry();
      break;
    }
  }

  free(cur_event);
  
  /*
  //test code
  static int tmpCount = 0;
  tmpCount++;

  if ( tmpCount > 100 )
    return NULL;
  
  
  if ( tmpCount == 100 )
  {
    ev->etype = DISKSIMEND;
    return ev;
  }

  if ( tmpCount % 3 == 0 )
    ev->etype = DISKSIMREAD;
  else
    ev->etype = DISKSIMWRITE;
  ev->pos = 0;
  ev->count = 1000;
  ev->devno = 0;
  // end test code
  */

  return ev;
}
