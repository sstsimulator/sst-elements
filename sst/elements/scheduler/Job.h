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

#ifndef __JOB_H__
#define __JOB_H__

#include <iostream>
#include <string>
#include <queue>
#include "Statistics.h"  //needed for friend declaration
using namespace std;

class AllocInfo;
class Machine;

class Job {
public:
  Job(istream& input, bool accurateEsts);
  Job(long arrivalTime, int procsNeeded, long actualRunningTime,
      long estRunningTime);

  long getArrivalTime() { return arrivalTime; }
  long getStartTime();
  int getProcsNeeded() { return procsNeeded; }
  long getJobNum() { return jobNum; }

  //two versions depending on whether allocation is considered:
  long getEstimatedRunningTime() { return estRunningTime; }
  long getEstimatedRunningTime(AllocInfo* allocInfo);

  string toString();

  void start(long time, Machine* machine, AllocInfo* allocInfo,
	       Statistics* stats);
  //start the job

private:
  long arrivalTime;        //when the job arrived
  int procsNeeded;         //how many processors it uses
  long actualRunningTime;  //how long it runs
  long estRunningTime;     //user estimated running time
  long startTime;	     //when the job started (-1 if not running)
  
  long jobNum;             //ID number unique to this job

  long getActualTime() { return actualRunningTime; }

  //helper for constructors:
  void initialize(long arrivalTime, int procsNeeded,
		  long actualRunningTime, long estRunningTime);

  friend class Statistics;
  friend class schedComponent;
  //so statistics and schedComponent can use actual job times

  friend class boost::serialization::access;
  template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {
      ar & BOOST_SERIALIZATION_NVP(arrivalTime);
      ar & BOOST_SERIALIZATION_NVP(procsNeeded);
      ar & BOOST_SERIALIZATION_NVP(actualRunningTime);
      ar & BOOST_SERIALIZATION_NVP(estRunningTime);
      ar & BOOST_SERIALIZATION_NVP(startTime);
      ar & BOOST_SERIALIZATION_NVP(jobNum);
    }
  
};


#endif
