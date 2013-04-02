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

/*
 * Class in charge of printing logs
 *
 * Performance issue: the current implementation opens and closes the
 * log file each time a line is added.  This ensures that the log is
 * up to date, but better would be flush files periodically (or at the
 * end of the simulation).
 */

#ifndef __STATISTICS_H__
#define __STATISTICS_H__

#include <string>
using namespace std;

namespace SST {
namespace Scheduler {

class Machine;
class Scheduler;
class Allocator;
class AllocInfo;

class Statistics {
 public:
  Statistics(Machine* machine, Scheduler* sched, Allocator* alloc,
	     string baseName, char* logList);

  virtual ~Statistics();

  static void printLogList(ostream& out);  //print list of possible logs

  void jobArrives(unsigned long time);  //called when a job has arrived

  void jobStarts(AllocInfo* allocInfo, unsigned long time);
  //called every time a job starts

  void jobFinishes(AllocInfo* allocInfo, unsigned long time);
  //called every time a job completes

  void done();  //called after all events have occurred

 private:
  void writeTime(AllocInfo* allocInfo, unsigned long time);
  //write time statistics to file

  
  void writeAlloc(AllocInfo* allocInfo);
  //write allocation information to file

  void writeVisual(string mesg);
  //write to log for visualization
  

  void writeUtil(unsigned long time);
  //method to write utilization statistics to file
  //force it to write last entry by setting time = -1

  void writeWaiting(unsigned long time);
  //possibly add line to log recording number of waiting jobs
  //  (only prints 1 line per time: #waiting jobs after all events at that time)
  //argument is current time or -1 at end of trace

  void initializeLog(string extension);
  void appendToLog(string mesg, string extension);

  string baseName;  //part of name used as base of output files (i.e. baseName.time)
  Machine* machine;
  unsigned long currentTime;
  int procsUsed;    //#processors used at currentTime

  bool* record;  //array giving whether each kind of log should be kept

  int lastUtil;  //last observed utilization value; used to filter out values w/ shared time
  unsigned long lastUtilTime;  //when it first reached this val (-1 = no observations)

  //variables to do the same thing for the number of waiting jobs:
  unsigned long lastWaitTime;  //time for which we next will record #waiting jobs
  long lastWaitJobs;  //# waiting jobs last printed
  int waitingJobs;    //current guess of what to record for that time
  int tempWaiting;    //actual number of waiting jobs right now

  string fileHeader;  //commented out header for all log files

  string outputDirectory;  //directory for all output files
};

}
}
#endif
