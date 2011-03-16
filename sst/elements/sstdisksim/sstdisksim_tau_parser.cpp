// Copyright 2009-2011 Sandia Corporation. Under the terms
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

// implementation of callback routines 
int EnterState(void *userData, double time, 
	       unsigned int nodeid, unsigned int tid, unsigned int stateid)
{
  //  printf("Entered state %d time %g nid %d tid %d\n", 
  //	 stateid, time, nodeid, tid);
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

int DefThread(void *userData, unsigned int nodeid, unsigned int threadToken,
	      const char *threadName )
{
  //  printf("DefThread nid %d tid %d, thread name %s\n", 
  //	 nodeid, threadToken, threadName);
  return 0;
}

int EndTrace( void *userData, unsigned int nodeid, unsigned int threadid)
{
  //  printf("EndTrace nid %d tid %d\n", nodeid, threadid);
  return 0;
}

int DefStateGroup( void *userData, unsigned int stateGroupToken, 
		   const char *stateGroupName )
{
  //  printf("StateGroup groupid %d, group name %s\n", stateGroupToken, 
  //		  stateGroupName);
  return 0;
}

int DefState( void *userData, unsigned int stateToken, const char *stateName, 
	      unsigned int stateGroupToken )
{
  //  printf("DefState stateid %d stateName %s stategroup id %d\n",
  //		  stateToken, stateName, stateGroupToken);
  return 0;
}

int DefUserEvent( void *userData, unsigned int userEventToken,
		  const char *userEventName, int monotonicallyIncreasing )
{
  //  printf("DefUserEvent event id %d user event name %s, monotonically increasing = %d\n", userEventToken,
  //	 userEventName, monotonicallyIncreasing);
  return 0;
}

int EventTrigger( void *userData, double time, 
		  unsigned int nodeToken,
		  unsigned int threadToken,
		  unsigned int userEventToken,
		  long long userEventValue)
{
  //  printf("EventTrigger: time %g, nid %d tid %d event id %d triggered value %lld \n", time, nodeToken, threadToken, userEventToken, userEventValue);
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
  //  printf("Message Send: time %g, nid %d, tid %d dest nid %d dest tid %d messageSize %d messageComm %d messageTag %lld \n", time, sourceNodeToken,
  //	 sourceThreadToken, destinationNodeToken,
  //	 destinationThreadToken, messageSize, messageComm, messageTag);
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
  //  printf("Message Recv: time %g, nid %d, tid %d dest nid %d dest tid %d messageSize %d messageComm %d messageTag %lld \n", time, sourceNodeToken,
  //	 sourceThreadToken, destinationNodeToken,
  //	 destinationThreadToken, messageSize, messageComm, messageTag);
  return 0;
}

sstdisksim_tau_parser::sstdisksim_tau_parser(const char* trc_file, const char* edf_file)
{
  Ttf_FileHandleT fh;

  int recs_read, pos;
  Ttf_CallbacksT cb;

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

  printf("Position returned %d\n", pos);

  recs_read = Ttf_ReadNumEvents(fh, cb, 4);
  printf("Read %d records\n", recs_read);

  recs_read = Ttf_ReadNumEvents(fh, cb, 100);
  printf("Read %d records\n", recs_read);

  Ttf_CloseFile(fh);
}

/******************************************************************************/
sstdisksim_tau_parser::~sstdisksim_tau_parser()
{
  filestream.close();
}

/******************************************************************************/
sstdisksim_event*
sstdisksim_tau_parser::getNextEvent()
{
  static int tmpCount = 0;
  tmpCount++;

  if ( tmpCount > 100 )
    return NULL;

  sstdisksim_event* ev = new sstdisksim_event();
  ev->completed = 0;

  if ( tmpCount == 100 )
  {
    ev->etype = DISKSIMEND;
    return ev;
  }

  
  // Todo:  Add reading of events from a file here.
  // Erase this after starting to read from the file
  if ( tmpCount % 3 == 0 )
    ev->etype = DISKSIMREAD;
  else
    ev->etype = DISKSIMWRITE;
  ev->pos = 0;
  ev->count = 1000;
  ev->devno = 0;
  // End erase

  return ev;
}
