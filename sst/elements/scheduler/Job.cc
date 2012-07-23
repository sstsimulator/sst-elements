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

#include "sst/core/serialization/element.h"

#include "Job.h"
#include "Machine.h"
#include "Statistics.h"
#include "exceptions.h"
#include "misc.h"

static long nextJobNum = 0;  //used setting jobNum

Job::Job(istream& input, bool accurateEsts) {
  string line;  
  getline(input, line);

  long arrivalTime;
  int procsNeeded;
  long actualRunningTime;
  long estRunningTime;
  int num = sscanf(line.c_str(), "%ld %d %ld %ld", &arrivalTime, &procsNeeded,
		   &actualRunningTime, &estRunningTime);
  if((num != 3) && (num != 4))
    throw InputFormatException();
  if(accurateEsts || (num == 3))
    estRunningTime = actualRunningTime;

  initialize(arrivalTime, procsNeeded, actualRunningTime,
	     estRunningTime);
}

Job::Job(long arrivalTime, int procsNeeded, long actualRunningTime,
	 long estRunningTime) {
  initialize(arrivalTime, procsNeeded, actualRunningTime, estRunningTime);
}

void Job::initialize(long arrivalTime, int procsNeeded,
		     long actualRunningTime, long estRunningTime) {
  //helper for constructors
  
  //make sure estimate is valid; workload log uses -1 for "no estimate"
  if(estRunningTime < actualRunningTime)
    estRunningTime = actualRunningTime;
  
  this -> arrivalTime = arrivalTime;
  this ->  procsNeeded = procsNeeded;
  this -> actualRunningTime = actualRunningTime;
  this -> estRunningTime = estRunningTime;

  startTime = -1;
  
  jobNum = nextJobNum;
  nextJobNum++;
}

long Job::getStartTime() {
  if(startTime == -1)
    throw InternalErrorException();
  return startTime;
}

string Job::toString() {
  char retVal[100];
  snprintf(retVal, 100, "Job #%ld (%ld, %d, %ld, %ld)", jobNum,
	   arrivalTime, procsNeeded, actualRunningTime, estRunningTime);
  return retVal;
}

void Job::start(long time, Machine* machine, AllocInfo* allocInfo,
		  Statistics* stats) {
  //start the job
  if(startTime != -1) {
    string mesg = "attempt to start an already-running job: ";
    mesg += toString();
    internal_error(mesg);
  }

  machine -> allocate(allocInfo);
  startTime = time;
  stats -> jobStarts(allocInfo, time);

  //Event* retVal = new DepartureEvent(time+actualRunningTime, allocInfo);
  //events -> push(retVal);
}

