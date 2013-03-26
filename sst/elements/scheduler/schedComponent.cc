// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>
#include <fstream>
#include <iostream>

#include <iostream>
#include <boost/tokenizer.hpp>        // for reading YumYum jobs
#include <boost/algorithm/string.hpp>
#include <cstring>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

//#include <QFileSystemWatcher>

#include "sst/core/element.h"


#include "unistd.h"             // for sleep


#include "misc.h"
#include "Factory.h"
#include "MachineMesh.h"
#include "MachineMesh.h"

#include "JobKillEvent.h"
#include "JobFaultEvent.h"
#include "CommunicationEvent.h"

using namespace SST;
using namespace std;

Machine* schedComponent::getMachine() {
  return machine;
}

schedComponent::schedComponent(ComponentId_t id, Params_t& params) :
  Component(id) {
    lastfinaltime = ~0;

  if ( params.find("traceName") == params.end() ) {
    _abort(event_test,"couldn't find trace name\n");
  }


  // tell the simulator not to end without us
  registerExit();

  selfLink = configureSelfLink("linkToSelf", new Event::Handler<schedComponent>(this, &schedComponent::handleJobArrivalEvent) );
  selfLink->setDefaultTimeBase(registerTimeBase("1 s"));

  // configure links
  bool done = 0;
  int count = 0;
  // connect links till we can't find any
  printf("Scheduler looking for link...");
  while (!done) {
    char name[50];
    snprintf(name, 50, "nodeLink%d", count);
    printf(" %s", name);
    SST::Link *l = configureLink( name, new Event::Handler<schedComponent,int>(this, &schedComponent::handleCompletionEvent, count) );
    if (l) {
      SST::Event * getID = new CommunicationEvent( ID );
      l->Send( getID );
      nodes.push_back(l);
      count++;
    } else {
      printf("(no %s)", name);
      done = 1;
    }
  }

  printf("\n");
  Factory factory;
  machine = factory.getMachine(params, nodes.size(), this);
  scheduler = factory.getScheduler(params, nodes.size());
  theAllocator = factory.getAllocator(params, machine);
  string trace = params[ "traceName" ].c_str();

  if(dynamic_cast<MachineMesh*>(machine) == NULL)
  {
    char timestring[] = "time";
    stats = new Statistics(machine, scheduler, theAllocator, trace, timestring);
  }
  else
  {
    //the alloc output only works on a mesh because it calculates L1 distances
    char timestring[] = "time,alloc";
    stats = new Statistics(machine, scheduler, theAllocator, trace, timestring);
  }
  
  useYumYumSimulationKill = params.find( "useYumYumSimulationKill" ) != params.end();
  YumYumSimulationKillFlag = false;

  if( params.find( "YumYumPollWait" ) != params.end() ){
    YumYumPollWait = atoi( params.find( "YumYumPollWait" )->second.c_str() );
  }else{
    YumYumPollWait = 250;
  }
  
  useYumYumTraceFormat = params.find( "useYumYumTraceFormat" ) != params.end();
  printYumYumJobLog = params.find( "printYumYumJobLog" ) != params.end();
  printJobLog = params.find( "printJobLog" ) != params.end();
  
  char* inputDir = getenv("SIMINPUT");
  if(inputDir != NULL) {
    string fullName = inputDir + trace;
    jobListFileName = fullName;
  }else{
    jobListFileName = trace;
  }
    
  jobListFileNamePath = boost::filesystem::path( jobListFileName.c_str() );
  LastJobFileModTime = boost::filesystem::last_write_time( jobListFileNamePath );
 
  printf("\nScheduler Detects %d nodes\n", (int)nodes.size());
    
  machine -> reset();
  scheduler -> reset();

  lastJobRead[ 0 ] = '\0';

  readJobs();

  if( useYumYumTraceFormat ){
    CommunicationEvent * CommEvent = new CommunicationEvent( START_FILE_WATCH );
    CommEvent->payload = jobListFileName;
    selfLink->Send( CommEvent );
  }
  
  jobLogFileName = params[ "jobLogFileName" ];
}

int schedComponent::Setup() {return 0;}

schedComponent::schedComponent() :
    Component(-1)
{
    // for serialization only
}



void schedComponent::readJobs(){
  ifstream input;
 
  input.open( jobListFileName.c_str() );
 
  if(!input.is_open())
    input.open(trace.c_str());  //try without directory                     
  if(!input.is_open())
    error("Unable to open file " + trace);
 
  string line;
  while(!input.eof()) {
    getline(input, line);
    if( useYumYumTraceFormat ){
      newYumYumJobLine( line );
    }else{
      newJobLine( line );
    }
  }
  input.close();

}


/*
 * Returns true if the job list file has been modified since the last time it was checked, false otherwise
 */
bool schedComponent::checkJobFile(){
  if( boost::filesystem::exists( jobListFileNamePath ) ){
    time_t lastWritten = boost::filesystem::last_write_time( jobListFileNamePath );
    if( lastWritten > LastJobFileModTime ){
      LastJobFileModTime = lastWritten;
      return true;
    }
  }
  return false;
}



/*
  Ensures that j can be scheduled.  Assumes that j is at jobs.back()

  If j can be scheduled, it is scheduled
*/
bool schedComponent::validateJob( Job * j, vector<Job> jobs, long runningTime ){
  char mesg[100];
  bool ok = true;
  if (j -> getProcsNeeded() <= 0) {
    sprintf(mesg, "Job %ld  requests %d processors; ignoring it",
            j -> getJobNum(), j -> getProcsNeeded());
    warning(mesg);
    jobs.pop_back();
    ok = false;
  }
  if (ok && runningTime < 0) {  //time 0 also strange, but perhaps rounded down     
    sprintf(mesg, "Job %ld  has running time of %ld; ignoring it",
            j -> getJobNum(), runningTime);
    warning(mesg);
    jobs.pop_back();
    ok = false;
  }
  if(ok && j -> getProcsNeeded() > machine -> getNumFreeProcessors()) {
    sprintf(mesg,
            "Job %ld requires %d processors but only %d are in the machine",
            j -> getJobNum(), j -> getProcsNeeded(),
            machine -> getNumFreeProcessors());
    error(mesg);
  }
  if (ok) {
    ArrivalEvent* ae = new ArrivalEvent(j -> getArrivalTime(), jobs.size()-1);
    selfLink->Send(0 , ae);
  }
  
  return ok;
}


/*
  Reads in a joblist in the YumYum format.
  
  All jobs will have an arrival time of getCurrentSimTime().
  
  The YumYum format is something like:
  [Job ID], [Job duration], [Procs needed]
*/
bool schedComponent::newYumYumJobLine( std::string line ){
  boost::algorithm::trim( line );
  
  if( line.compare( "YYKILL" ) == 0 ){
    YumYumSimulationKillFlag = true;
    useYumYumSimulationKill = false;
    return false;
  }

  if(line.find_first_of("1234567890") == string::npos)
    return false;

  char ID[ JobIDlength ];
  uint64_t duration;
  uint64_t procs;
      
  boost::tokenizer< boost::escaped_list_separator<char> > Tokenizer( line );
  vector<string> tokens;
  tokens.assign( Tokenizer.begin(), Tokenizer.end() );

  boost::algorithm::trim( tokens.at( 0 ) );
  boost::algorithm::trim( tokens.at( 1 ) );
  boost::algorithm::trim( tokens.at( 2 ) );
  
  strncpy( ID, tokens.at( 0 ).c_str(), JobIDlength );
  if( JobIDlength > 0 ){
    ID[ JobIDlength - 1 ] = '\0';
  }

  uint64_t currentJobID;
  uint64_t lastJobID;

  sscanf( ID, "%lu", &currentJobID );
  sscanf( lastJobRead, "%lu", &lastJobID );

  if( lastJobRead[ 0 ] != '\0' && currentJobID <= lastJobID ){
    return false;       // We have read this job before, don't do it again.
  }

  sscanf( tokens.at( 1 ).c_str(), "%lu", &duration );
  sscanf( tokens.at( 2 ).c_str(), "%lu", &procs );

  strcpy( lastJobRead, ID );

  if( tokens.size() != 3 ){
    error( "Poorly formatted input line: " + line );
  }else{
    jobs.push_back( Job( getCurrentSimTime(), procs, duration, duration, std::string( ID ) ) );
  }

  // validate the job to make sure that the specified machine can actually run it. 
  Job* j = &jobs.back();
  
  return validateJob( j, jobs, abs( (long)duration ) );
}



bool schedComponent::newJobLine( std::string line ){
  if(line.find_first_not_of(" \t\n") == string::npos)
    return false;

  unsigned long arrivalTime;
  int procsNeeded;
  unsigned long runningTime;
  unsigned long estRunningTime;
  int num = sscanf(line.c_str(), "%ld %d %ld %ld", &arrivalTime,
                   &procsNeeded, &runningTime, &estRunningTime);
  if(num < 3)
    error("Poorly formatted input line: " + line);
  if(num == 3)
    jobs.push_back(Job(arrivalTime, procsNeeded, runningTime, runningTime));
  else   //read all 4                                                        
    jobs.push_back(Job(arrivalTime, procsNeeded, runningTime, estRunningTime));

  //validate                                                                  
  Job* j = &jobs.back();
  
  return validateJob( j, jobs, runningTime );
}



// If we aren't using the Yum Yum functionality, this will just unregister and return;
// Otherwise, this will block, checking if there are new jobs at intervals defined by YumYumPollWait, or if the YumYumSimulationKillFlag has been set.
void schedComponent::unregisterYourself(){
  if( useYumYumSimulationKill ){
    while( YumYumSimulationKillFlag != true && jobs.empty() ){
      boost::this_thread::sleep( boost::posix_time::milliseconds( YumYumPollWait ) );
      if( checkJobFile() ){
        readJobs();
        if( !jobs.empty() ){
          break;
        }
      }if( YumYumSimulationKillFlag ){
        unregisterExit();
      }
    }
  }else{
    unregisterExit();
  }
}



// incoming events are scanned and deleted. ev is the returned event,
// node is the node it came from.
void schedComponent::handleCompletionEvent(Event *ev, int node) {

  if( dynamic_cast<CommunicationEvent *>( ev ) ){
    CommunicationEvent * CommEvent = dynamic_cast<CommunicationEvent *>( ev );
    nodeIDs.insert( nodeIDs.begin() + node, CommEvent->payload );
  }
  else if(dynamic_cast<CompletionEvent*>(ev))
  {
    CompletionEvent *event = dynamic_cast<CompletionEvent*>(ev);
    int jobNum = event->jobNum;
    if(runningJobs[jobNum].ai == NULL)
    {
      runningJobs.erase(jobNum);
      return; //this event has already stopped (probably faulted)
    }

    if ((--(runningJobs[jobNum].i)) == 0) {
       if( printJobLog ){
        logJobFinish( runningJobs.at( jobNum ) );
       }
      finishingcomp.push_back(event);
      FinalTimeEvent *fte = new FinalTimeEvent();
      if(runningJobs[jobNum].ai->job->getStartTime() == getCurrentSimTime())
        fte->forceExecute = true;
      selfLink->Send(0, fte); //send back an event at the same time so we know it finished
    }
    if(jobNum == jobs.back().jobNum)
    {
      if(jobs.empty())
      {
        unregisterYourself();
      }
    }

  }
 else {
    //If it is not a completion it is (hopefully) a fault.
    //as far as the scheduler is concerned this is treated the same way
    //however, must send a kill event to tell all nodes with this job to
    //stop running (not all the nodes may have failed but all must stop)
    //for now, the job is dropped forever
    JobFaultEvent *event = dynamic_cast<JobFaultEvent*>(ev);
    if (event) {
      int jobNum = event->jobNum;
      if(jobNum == -1 )
        return; // the node that faulted was not running a job, nothing for 
                //scheduler, allocator, machine to do
      //force all nodes running the job to kill it

      AllocInfo *ai = runningJobs[jobNum].ai;
      if(ai == NULL)
      {
        runningJobs.erase(jobNum);
        return;
      }
      //if ai is NULL we already killed this job
      
      if( printJobLog ){
        logJobFault( runningJobs[ jobNum ], event );
      }

      for (int i = 0; i < ai->job->getProcsNeeded(); ++i) {
        JobKillEvent *ec = new JobKillEvent(ai->job->getJobNum());

        nodes[ai->nodeIndices[i]]->Send(ec);
      }

      runningJobs.erase(jobNum);
      machine->deallocate(ai);
      theAllocator->deallocate(ai);
      stats->jobFinishes(ai, getCurrentSimTime());
      scheduler->jobFinishes(ai->job, getCurrentSimTime(), machine);
      delete ai; //the job is done and deleted from our records; don't need
      //its allocinfo again

      //tries to start job
      AllocInfo* allocInfo;
      do {
        allocInfo = scheduler -> tryToStart(theAllocator, getCurrentSimTime(), machine, stats);
      } while(allocInfo != NULL);
      if(jobNum == jobs.back().jobNum)
      {
        while(!jobs.empty() && runningJobs.find(jobs.back().jobNum) == runningJobs.end())
          jobs.pop_back();
        if(jobs.empty())
        {
          unregisterYourself();             // This is the one that actually does the unregistering.
        }
      }
    } else {
      internal_error("S: Error! Bad Event Type!\n");
    }
  }  
}


void schedComponent::handleJobArrivalEvent(Event *ev) {
  if( dynamic_cast<CommunicationEvent *>( ev ) ){
    CommunicationEvent * CommEvent = dynamic_cast<CommunicationEvent *>( ev );

    if( CommEvent->CommType == LOG_JOB_START ){
    }else if( CommEvent->CommType == START_FILE_WATCH ){
    }
    return;
  }

  ArrivalEvent *arevent = dynamic_cast<ArrivalEvent*>(ev);
  if(arevent)
  {
    finishingarr.push_back(arevent);
    FinalTimeEvent* fte = new FinalTimeEvent();
    selfLink->Send(0, fte); //send back an event at the same time so we know it finished
  }
  else
  {
    FinalTimeEvent* fev = dynamic_cast<FinalTimeEvent*>(ev);
    if(fev)
    {
      if(lastfinaltime == getCurrentSimTime() && !fev->forceExecute)
        return; //ignore duplicate final time events (unless the event forces us
                //to handle them)
      else
        lastfinaltime = getCurrentSimTime();

      while(!finishingcomp.empty())
      {
        vector<CompletionEvent*>::iterator it = finishingcomp.begin();
        CompletionEvent* event = *it;
        int jobNum = event->jobNum;
        AllocInfo *ai = runningJobs[jobNum].ai;
        runningJobs.erase(jobNum);
        machine->deallocate(ai);
        theAllocator->deallocate(ai);
        stats->jobFinishes(ai, getCurrentSimTime());
        scheduler->jobFinishes(ai->job, getCurrentSimTime(), machine);
        delete ai;
        if(jobNum == jobs.back().jobNum)
        {
          while(!jobs.empty() && runningJobs.find(jobs.back().jobNum) == runningJobs.end())
            jobs.pop_back();
          if(jobs.empty())
          {
            unregisterYourself();
          }
        }
        finishingcomp.erase(it);
        delete event;
      } 
      //arrivals are handled strictly after finishes at the same time to
      //match the Java
      //each time step these are cleared so we don't need to worry about
      //events not at the same time step, and they should already be sorted
      //by number because they are given to SST (and therefore come back) that way.
      while(!finishingarr.empty())
      {
        vector<ArrivalEvent*>::iterator it = finishingarr.begin();
        ArrivalEvent *event = *it;
        event -> happen(machine, theAllocator, scheduler, stats, &jobs[event->getJobIndex()]);
        finishingarr.erase(it);
        //delete event;
      }      
      //tries to start job now that the scheduler knows about all jobs 
      AllocInfo* allocInfo;
      do {
        allocInfo = scheduler -> tryToStart(theAllocator, getCurrentSimTime(), machine, stats);
      } while(allocInfo != NULL);

    }
    else
      error("Arriving event was not an arrival nor finaltime event");
  }
}

int schedComponent::Finish() {

  scheduler -> done();
  stats -> done();
  theAllocator -> done();
  return 0;
}


void schedComponent::startJob(AllocInfo* ai) {
  Job* j = ai->job;
  int* jobNodes = ai->nodeIndices;

  // send to each person in the node list
  for (int i = 0; i < j->getProcsNeeded(); ++i) {
    JobStartEvent *ec = new JobStartEvent(j->getActualTime(), j->getJobNum());
    nodes[jobNodes[i]]->Send(ec);
  }

  IAI iai;
  iai.i = j->getProcsNeeded();
  iai.ai = ai;
  runningJobs[j->getJobNum()] = iai;

  if( printJobLog ){
    logJobStart( iai );
  }
}


void schedComponent::logJobStart( IAI iai ){
  std::ofstream jobLog;
  jobLog.open( this->jobLogFileName.c_str(), ios::out | ios::ate | ios::app );
  if( jobLog.is_open() ){
    jobLog << iai.ai->job->getID() << "," << getCurrentSimTime() << ",-1,-1,";
    for( int counter = 0; counter < iai.ai->job->getProcsNeeded(); counter ++ ){
      if( counter > 0 ){
        jobLog << " ";
      }
      jobLog << nodeIDs.at( iai.ai->nodeIndices[ counter ] );
    }

    jobLog << endl;
    jobLog.close();
  }else{
    char errorMessage[ 1024 ];
    
    snprintf( errorMessage, 1023, "Couldn't open %s for writing the job log.", this->jobLogFileName.c_str() );
    
    error( errorMessage );
  }
}

void schedComponent::logJobFinish( IAI iai ){
  std::ofstream jobLog;
  jobLog.open( this->jobLogFileName.c_str(), ios::out | ios::ate | ios::app );
  if( jobLog.is_open() ){
    jobLog << iai.ai->job->getID() << "," << iai.ai->job->getStartTime() << "," << getCurrentSimTime() << ",0,";
    for( int counter = 0; counter < iai.ai->job->getProcsNeeded(); counter ++ ){
      if( counter > 0 ){
        jobLog << " ";
      }
      jobLog << nodeIDs.at( iai.ai->nodeIndices[ counter ] );
    }

    jobLog << endl;
    jobLog.close();
  }else{
    char errorMessage[ 1024 ];
    
    snprintf( errorMessage, 1023, "Couldn't open %s for writing the job log.", this->jobLogFileName.c_str() );
    
    error( errorMessage );
  } 
}

void schedComponent::logJobFault( IAI iai, JobFaultEvent * faultEvent ){
  std::ofstream jobLog;
  jobLog.open( this->jobLogFileName.c_str(), ios::out | ios::ate | ios::app );
  if( jobLog.is_open() ){
    jobLog << iai.ai->job->getID() << "," << iai.ai->job->getStartTime() << "," << getCurrentSimTime() << ",1,";
    for( int counter = 0; counter < iai.ai->job->getProcsNeeded(); counter ++ ){
      if( counter > 0 ){
        jobLog << " ";
      }
      jobLog << nodeIDs.at( iai.ai->nodeIndices[ counter ] );
    }

    jobLog << endl;
    jobLog.close();
  }else{
    char errorMessage[ 1024 ];
    
    snprintf( errorMessage, 1023, "Couldn't open %s for writing the job log.", this->jobLogFileName.c_str() );
    
    error( errorMessage );
  } 
}



/*
bool schedComponent::clockTic( Cycle_t ) {

  // send test jobs
  if (getCurrentSimTime() == 1) {
    // send to node 0
    // create the event
    SWFEvent *e = new SWFEvent;
    e->JobNumber = 1;
    e->RunTime = 10;
    // create the list of target nodes
    targetList_t nodeList;
    nodeList.push_back(0);
    // start the job
    startJob(e, nodeList);
  } else if (getCurrentSimTime() == 12) {
  } else if (getCurrentSimTime() == 13) {
    // send to odd nodes
    // create the event
    SWFEvent *e = new SWFEvent;
    e->JobNumber = 1;
    e->RunTime = 5;
    // create the list of target nodes
    targetList_t nodeList;
    for (int i = 0; i < nodes.size(); ++i) {
      if (i & 0x1) {
	nodeList.push_back(i);
      }
    }
    // start the job
    startJob(e, nodeList);
  }

  // end simulation after 1 hour of simulated time
  if (getCurrentSimTime() >= 3600) {
    unregisterExit();
  }

  // return false so we keep going
  return false;
}
*/

// Element Libarary / Serialization stuff

//BOOST_CLASS_EXPORT(ArrivalEvent)
BOOST_CLASS_EXPORT(schedComponent)

