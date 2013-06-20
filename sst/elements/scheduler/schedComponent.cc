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

#include "sst_config.h"

#include "sst/core/serialization/element.h"
#include <assert.h>
#include <fstream>

#include <boost/tokenizer.hpp>        // for reading YumYum jobs
#include <boost/algorithm/string.hpp>
#include <cstring>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

//#include <QFileSystemWatcher>

#include "sst/core/element.h"
#include "schedComponent.h" 


#include "unistd.h"             // for sleep
#include <stdlib.h>

#include <sst/core/event.h>

#include "Allocator.h"
#include "AllocInfo.h"
#include "ArrivalEvent.h"
#include "CompletionEvent.h"
#include "CommunicationEvent.h"
#include "Factory.h"
#include "FaultEvent.h"
#include "FinalTimeEvent.h"
#include "Job.h"
#include "JobKillEvent.h"
#include "JobStartEvent.h"
#include "Machine.h"
#include "MachineMesh.h"
#include "misc.h"
#include "Scheduler.h"
#include "Statistics.h"


using namespace std;
using namespace SST;
using namespace SST::Scheduler;

  // from nodeComponent.cc
extern unsigned short int * yumyumRand48State;

Machine* schedComponent::getMachine() {
  return machine;
}
schedComponent::~schedComponent()
{
    delete stats;
    delete scheduler;
}
schedComponent::schedComponent(ComponentId_t id, Params_t& params) :
  Component(id) {
    lastfinaltime = ~0;

  if ( params.find("traceName") == params.end() ) {
    _abort(event_test,"couldn't find trace name\n");
  }

  // tell the simulator not to end without us
  registerThis();


  // get the PRNG seed we'll use
  
  long int seed;
  yumyumRand48State = (unsigned short *) malloc( 3 * sizeof( short ) );
  
  if( params.find( "seed" ) == params.end() ){
    seed = time( NULL );
  }else{
    seed = atoi( params[ "seed" ].c_str() );
  }

  yumyumRand48State[0] = 0x330E;
  yumyumRand48State[1] = seed & 0xFFFF;
  yumyumRand48State[2] = seed >> 16;

  //set up the all-important self-link
  selfLink = configureSelfLink("linkToSelf", new Event::Handler<schedComponent>(this, &schedComponent::handleJobArrivalEvent) );
  selfLink->setDefaultTimeBase( registerTimeBase( SCHEDULER_TIME_BASE ) );

  // configure links
  bool done = 0;
  int nodeCount = 0;
  // connect links till we can't find any
  printf("Scheduler looking for link...");
  while (!done) {
    char name[50];
    snprintf(name, 50, "nodeLink%d", nodeCount);
    printf(" %s", name);
    SST::Link *l = configureLink( name, SCHEDULER_TIME_BASE, new Event::Handler<schedComponent,int>(this, &schedComponent::handleCompletionEvent, nodeCount) );
    if (l) {
      
      nodes.push_back(l);
      nodeCount++;
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
 
  printf("\nScheduler Detects %d nodes\n", (int)nodes.size());

  machine -> reset();
  scheduler -> reset();

  jobLogFileName = params[ "jobLogFileName" ];
}


void schedComponent::setup(){

  for( vector<SST::Link *>::iterator nodeIter = nodes.begin(); nodeIter != nodes.end(); nodeIter ++ ){
          // ask the newly-connected node for its ID
      SST::Event * getID = new CommunicationEvent( RETRIEVE_ID );
      (*nodeIter)->send( getID ); 
  }

  // done setting up the links, now read the job list
    
  lastJobRead[ 0 ] = '\0';
 
  readJobs();

  if( useYumYumTraceFormat ){

      // in case there were no jobs in the joblist
    if( jobs.empty() ){
      unregisterYourself();
    }

    CommunicationEvent * CommEvent = new CommunicationEvent( START_FILE_WATCH );
    CommEvent->payload = & jobListFileName;
    selfLink->send( CommEvent ); 
  }

}


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
  if(!input.is_open()){
    char errorMesg[ 1024 ];
    snprintf( errorMesg, 1024, "Unable to open job trace file: %s", jobListFileName.c_str() );
    errorMesg[ 1023 ] = '\0';
    error( errorMesg );
  }

  LastJobFileModTime = boost::filesystem::last_write_time( jobListFileNamePath );
 
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
bool schedComponent::validateJob( Job * j, vector<Job> * jobs, long runningTime ){
  char mesg[100];
  bool ok = true;
  if (j -> getProcsNeeded() <= 0) {
    sprintf(mesg, "Job %ld  requests %d processors; ignoring it",
            j -> getJobNum(), j -> getProcsNeeded());
    warning(mesg);
    jobs->pop_back();
    ok = false;
  }
  if (ok && runningTime < 0) {  //time 0 also strange, but perhaps rounded down     
    sprintf(mesg, "Job %ld  has running time of %ld; ignoring it",
            j -> getJobNum(), runningTime);
    warning(mesg);
    jobs->pop_back();
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
    ArrivalEvent* ae = new ArrivalEvent(j -> getArrivalTime(), jobs->size()-1);
    selfLink->send(0 , ae); 
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
    if( useYumYumTraceFormat ){
      jobs.push_back( Job( getCurrentSimTime() + 1, procs, duration, duration + 1, std::string( ID ) ) );
    }else{
      jobs.push_back( Job( getCurrentSimTime(), procs, duration, duration, std::string( ID ) ) );
    }
  }

  // validate the job to make sure that the specified machine can actually run it. 
  Job* j = &jobs.back();
  
  return validateJob( j, &jobs, abs( (long)duration ) );
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
  
  return validateJob( j, &jobs, runningTime );
}


void schedComponent::registerThis(){
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
  registrationStatus = true;
}


void schedComponent::unregisterThis(){
  primaryComponentOKToEndSim();
  registrationStatus = false;
}


bool schedComponent::isRegistered(){
  return registrationStatus;
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
        unregisterThis();
      }
    }
  }else{
    unregisterThis();
  }

  if( !isRegistered() && jobLog.is_open() ){
    jobLog.close();
  }

}


void schedComponent::startNextJob(){
  CommunicationEvent * CommEvent = new CommunicationEvent( START_NEXT_JOB );
  if( useYumYumTraceFormat ){
    selfLink->send( 1, CommEvent ); 
  }else{
    selfLink->send( 0, CommEvent ); 
  }
//  while( scheduler -> tryToStart(theAllocator, getCurrentSimTime(), machine, stats) );
}


// incoming events are scanned and deleted. ev is the returned event,
// node is the node it came from.
void schedComponent::handleCompletionEvent(Event *ev, int node) {

  if( dynamic_cast<CommunicationEvent *>( ev ) ){
    CommunicationEvent * CommEvent = dynamic_cast<CommunicationEvent *>( ev );
    nodeIDs.insert( nodeIDs.begin() + node, *(std::string*)CommEvent->payload );
    delete ev;
  }
  else if(dynamic_cast<CompletionEvent*>(ev))
  {
    CompletionEvent *event = dynamic_cast<CompletionEvent*>(ev);
    int jobNum = event->jobNum;
    if(runningJobs[jobNum].ai == NULL)
    {
      runningJobs.erase(jobNum);
      delete ev;
      return; //this event has already stopped (probably faulted)
    }

    if (0 == (--(runningJobs[jobNum].i))) {
      runningJobs[jobNum].ai -> job -> hasRun = true;
       if( printJobLog ){
        logJobFinish( runningJobs[ jobNum ] );
       }
      finishingcomp.push_back(event->copy());
      FinalTimeEvent *fte = new FinalTimeEvent();
      if(runningJobs[jobNum].ai->job->getStartTime() == getCurrentSimTime())
        fte->forceExecute = true;
      selfLink->send(0, fte); //send back an event at the same time so we know it finished 
    }
    if(jobNum == jobs.back().jobNum)
    {
      if(jobs.empty())
      {
        unregisterYourself();
      }
    }
    delete ev;
  }
 else {
    //If it is not a completion it is (hopefully) a fault.
    //as far as the scheduler is concerned this is treated the same way
    //however, must send a kill event to tell all nodes with this job to
    //stop running (not all the nodes may have failed but all must stop)
    //for now, the job is dropped forever
    FaultEvent *event = dynamic_cast<FaultEvent*>(ev);
    if (event) {
      int jobNum = event->jobNum;
      if(jobNum == -1 ){
        return; // the node that faulted was not running a job, nothing for 
                //scheduler, allocator, machine to do
        delete ev;
      }
      //force all nodes running the job to kill it


      AllocInfo *ai = runningJobs[jobNum].ai;
      if(ai == NULL)
      {
        runningJobs.erase(jobNum);
        delete ev;
        return;
      }
      //if ai is NULL we already killed this job
      
      runningJobs[ jobNum ].ai->job->hasRun = true;
      
      if( printJobLog ){
        logJobFault( runningJobs[ jobNum ], event );
      }

      for (int i = 0; i < ai->job->getProcsNeeded(); ++i) {
        JobKillEvent *ec = new JobKillEvent(ai->job->getJobNum());

        nodes[ai->nodeIndices[i]]->send(ec); 
      }

      machine->deallocate(ai);
      theAllocator->deallocate(ai);
      if( useYumYumTraceFormat ){
        stats->jobFinishes(ai, getCurrentSimTime() + 1);
        scheduler->jobFinishes(ai->job, getCurrentSimTime() + 1, machine);
      }else{
        stats->jobFinishes(ai, getCurrentSimTime() );
        scheduler->jobFinishes(ai->job, getCurrentSimTime() , machine);
      }
      delete runningJobs.find( jobNum )->second.ai; //the job is done and deleted from our records; don't need
      runningJobs.erase(jobNum);
      //its allocinfo again

      startNextJob();

      if(jobNum == jobs.back().jobNum)
      {
        while(!jobs.empty() && runningJobs.find(jobs.back().jobNum) == runningJobs.end() && jobs.back().hasRun == true){
          jobs.pop_back();
        }
        if(jobs.empty())
        {
          unregisterYourself();             // This is the one that actually does the unregistering.
        }
      }

      delete ev;
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
    }else if( CommEvent->CommType == START_NEXT_JOB ){
      while( scheduler -> tryToStart(theAllocator, getCurrentSimTime(), machine, stats) );
    }
    delete ev;
    return;
  }

  ArrivalEvent *arevent = dynamic_cast<ArrivalEvent*>(ev);
  if(arevent)
  {
    finishingarr.push_back(arevent);
    FinalTimeEvent* fte = new FinalTimeEvent();
    fte->forceExecute = useYumYumSimulationKill;	// due to a race condition, we may need to force this FTE if we're still expecting jobs
    selfLink->send(0, fte); //send back an event at the same time so we know it finished 
  }
  else
  {
    FinalTimeEvent* fev = dynamic_cast<FinalTimeEvent*>(ev);
    if(fev)
    {
      if(lastfinaltime == getCurrentSimTime() && !fev->forceExecute){
        delete ev;
        return; //ignore duplicate final time events (unless the event forces us
                //to handle them)
      }

      else
        lastfinaltime = getCurrentSimTime();

      while(!finishingcomp.empty())
      {
        int finishedJobNum = finishingcomp.front()->jobNum;
        AllocInfo *ai = runningJobs[finishedJobNum].ai;
        runningJobs.erase(finishedJobNum);
        machine->deallocate(ai);
        theAllocator->deallocate(ai);
        if( useYumYumTraceFormat ){
          stats->jobFinishes(ai, getCurrentSimTime() + 1);
          scheduler->jobFinishes(ai->job, getCurrentSimTime() + 1, machine);
        }else{
          stats->jobFinishes(ai, getCurrentSimTime() );
          scheduler->jobFinishes(ai->job, getCurrentSimTime() , machine);
        }
        delete ai;


/*        for( vector<Job>::iterator iter = jobs.begin(); iter != jobs.end(); iter ++ ){
          if( (*iter).getJobNum() == finishedJobNum ){
            jobs.erase( iter );
            break;
          }
        }*/

        if(finishedJobNum == jobs.back().jobNum)
        {
          while(!jobs.empty() && runningJobs.find(jobs.back().jobNum) == runningJobs.end() && jobs.back().hasRun == true){
            jobs.pop_back();
          }

          if(jobs.empty())
          {
            unregisterYourself();
          }
        }
        delete finishingcomp.front();
        finishingcomp.pop_front();
      } 
      //arrivals are handled strictly after finishes at the same time to
      //match the Java
      //each time step these are cleared so we don't need to worry about
      //events not at the same time step, and they should already be sorted
      //by number because they are given to SST (and therefore come back) that way.
      while(!finishingarr.empty())
      {
        finishingarr.front() -> happen(machine, theAllocator, scheduler, stats, &jobs[finishingarr.front()->getJobIndex()]);
        delete finishingarr.front();
        finishingarr.pop_front();
      }      
      delete ev; 

      startNextJob();
    }
    else
      error("Arriving event was not an arrival nor finaltime event");
  }
}

void schedComponent::finish() {
  scheduler -> done();
  stats -> done();
  theAllocator -> done();
}


void schedComponent::startJob(AllocInfo* ai) {
  Job* j = ai->job;
  int* jobNodes = ai->nodeIndices;

  // send to each person in the node list
  for (int i = 0; i < j->getProcsNeeded(); ++i) {
    JobStartEvent *ec = new JobStartEvent(j->getActualTime(), j->getJobNum());
    nodes[jobNodes[i]]->send(ec); 
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
  if( !jobLog.is_open() ){
    jobLog.open( this->jobLogFileName.c_str(), ios::out );
    if( jobLog.is_open() ){
      jobLog << "jobid, begin, end, failed, compute_nodes" << endl;
        // writing the header here should be safe, since the jobs should start before they end
    }
  }

  if( jobLog.is_open() ){
    jobLog << *iai.ai->job->getID() << "," << getCurrentSimTime() << ",-1,-1,";
    for( int counter = 0; counter < iai.ai->job->getProcsNeeded(); counter ++ ){
      if( counter > 0 ){
        jobLog << " ";
      }
      jobLog << nodeIDs.at( iai.ai->nodeIndices[ counter ] );
    }

    jobLog << endl;
  }else{
    char errorMessage[ 1024 ];
    
    snprintf( errorMessage, 1023, "Couldn't open %s for writing the job log.", this->jobLogFileName.c_str() );
    
    error( errorMessage );
  }
}

void schedComponent::logJobFinish( IAI iai ){
  if( !jobLog.is_open() ){
    jobLog.open( this->jobLogFileName.c_str(), ios::out );
  }

  if( jobLog.is_open() ){
    jobLog << *iai.ai->job->getID() << "," << iai.ai->job->getStartTime() << "," << getCurrentSimTime() << ",0,";
    for( int counter = 0; counter < iai.ai->job->getProcsNeeded(); counter ++ ){
      if( counter > 0 ){
        jobLog << " ";
      }
      jobLog << nodeIDs.at( iai.ai->nodeIndices[ counter ] );
    }

    jobLog << endl;
  }else{
    char errorMessage[ 1024 ];
    
    snprintf( errorMessage, 1023, "Couldn't open %s for writing the job log.", this->jobLogFileName.c_str() );
    
    error( errorMessage );
  } 

  if(getCurrentSimTime() < iai.ai->job->getStartTime()) {
    char errorMessage[ 1024 ];
    snprintf( errorMessage, 1023, "Job begin time larger than end time: jobid=%s, begin=%lu, end=%lu\n", (*iai.ai->job->getID()).c_str(), iai.ai->job->getStartTime(), getCurrentSimTime());
    error( errorMessage );
       exit(1);
  }
}

void schedComponent::logJobFault( IAI iai, FaultEvent * faultEvent ){
  if( !jobLog.is_open() ){
    jobLog.open( this->jobLogFileName.c_str(), ios::out );
  }

  if( jobLog.is_open() ){
    jobLog << *iai.ai->job->getID() << "," << iai.ai->job->getStartTime() << "," << getCurrentSimTime() << ",1,";
    for( int counter = 0; counter < iai.ai->job->getProcsNeeded(); counter ++ ){
      if( counter > 0 ){
        jobLog << " ";
      }
      jobLog << nodeIDs.at( iai.ai->nodeIndices[ counter ] );
    }

    jobLog << endl;
  }else{
    char errorMessage[ 1024 ];
    
    snprintf( errorMessage, 1023, "Couldn't open %s for writing the job log.", this->jobLogFileName.c_str() );
    
    error( errorMessage );
  } 

  if(getCurrentSimTime() < iai.ai->job->getStartTime()) {
    char errorMessage[ 1024 ];
    snprintf( errorMessage, 1023, "Job begin time larger than end time: jobid=%s, begin=%lu, end=%lu\n", (*iai.ai->job->getID()).c_str(), iai.ai->job->getStartTime(), (uint64_t)getCurrentSimTime());
    error( errorMessage );
       exit(1);
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
    primaryComponentOKToEndSim();
    
  }

  // return false so we keep going
  return false;
}
*/

// Element Libarary / Serialization stuff

//BOOST_CLASS_EXPORT(ArrivalEvent)
BOOST_CLASS_EXPORT(schedComponent)

