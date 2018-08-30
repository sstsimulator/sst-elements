// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/rng/mersenne.h"
#include "schedComponent.h" 

#include <thread>
#include <chrono>
#include <cstring>

#include <iostream> //debug

#include <sst/core/element.h>
#include <sst/core/params.h>

#include "Allocator.h"
#include "AllocInfo.h"
#include "Factory.h"
#include "FST.h"
#include "InputParser.h"
#include "Job.h"
#include "Machine.h"
#include "SimpleMachine.h"
#include "misc.h"
#include "Scheduler.h"
#include "Snapshot.h" //NetworkSim
#include "Statistics.h"
#include "TaskMapper.h"
#include "taskMappers/SimpleTaskMapper.h"
#include "TaskMapInfo.h"

#include "events/ArrivalEvent.h"
#include "events/CompletionEvent.h"
#include "events/CommunicationEvent.h"
#include "events/FaultEvent.h"
#include "events/FinalTimeEvent.h"
#include "events/JobKillEvent.h"
#include "events/JobStartEvent.h"
#include "events/SnapshotEvent.h"

using namespace std;
using namespace SST;
using namespace SST::Scheduler;

extern int yumyumFaultRand48Seed;
extern int yumyumErrorLogRand48Seed;
extern int yumyumErrorLatencyRand48Seed;
extern int yumyumErrorCorrectionRand48Seed;
extern int yumyumJobKillRand48Seed;

//necessary for sorting a list of pointers to completion events
//can't overload < because that does not work with pointers
bool compareCEPointers(CompletionEvent* ev1, CompletionEvent* ev2) 
{
    return ev1 -> jobNum < ev2 -> jobNum; 
}

schedComponent::~schedComponent()
{
    delete stats;
    delete scheduler;
    delete rng;
    if (FSTtype > 0) delete calcFST;
    if(snapshot != NULL) delete snapshot;
}

int readSeed( Params & params, std::string paramName ){
    if( !params.find<std::string>( paramName ).empty() ){
        return params.find<int64_t>(paramName);
    }else if( !params.find<std::string>( "seed" ).empty() ){
        return params.find<int64_t>( "seed" );
    }else{
        return time( NULL );
    }
}


schedComponent::schedComponent(ComponentId_t id, Params& params) :
    Component(id), snapshot(NULL)
{
    lastfinaltime = ~0;

    if (params.find<std::string>("traceName").empty()) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1,"couldn't find trace name\n");
    }

    //set up output
    schedout.init("", 8, 0, Output::STDOUT);

    // tell the simulator not to end without us
    registerThis();

    // get the PRNG seeds we'll use

    yumyumFaultRand48Seed = readSeed( params, std::string( "faultSeed" ) );
    yumyumErrorLogRand48Seed = readSeed( params, std::string( "errorLogSeed" ) );
    yumyumErrorLatencyRand48Seed = readSeed( params, std::string( "errorLatencySeed" ) );
    yumyumErrorCorrectionRand48Seed = readSeed( params, std::string( "errorCorrectionSeed" ) );
    yumyumJobKillRand48Seed = readSeed( params, std::string( "jobKillSeed" ) );
    // get running time RNG seed
    string runningTimeSeed = "none";
    if( !params.find<std::string>("runningTimeSeed").empty() ){
        runningTimeSeed = params.find<std::string>("runningTimeSeed");
    }
    if( runningTimeSeed.compare("none") != 0 ){
         rng = new SST::RNG::MersenneRNG( strtol(runningTimeSeed.c_str(), NULL, 0) );
    } else {
        rng = new SST::RNG::MersenneRNG();
    }

    //set up the all-important self-link
    selfLink = configureSelfLink("linkToSelf", new Event::Handler<schedComponent>(this, &schedComponent::handleJobArrivalEvent));
    selfLink->setDefaultTimeBase(registerTimeBase(SCHEDULER_TIME_BASE));

    // configure links
    bool done = 0;
    int nodeCount = 0;
    // connect links till we can't find any
    schedout.output("Scheduler looking for link...");
    while (!done) {
        char name[50];
        snprintf(name, 50, "nodeLink%d", nodeCount);
        schedout.output(" %s", name);
        SST::Link *l = configureLink(name, SCHEDULER_TIME_BASE, new Event::Handler<schedComponent,int>(this, &schedComponent::handleCompletionEvent, nodeCount));
        if (l) {
            nodes.push_back(l);
            nodeCount++;
        } else {
            schedout.output("(no %s)", name);
            done = 1;
        }
    }
    schedout.output("\n");
    schedout.output("Scheduler Detects %d nodes\n", (int)nodes.size());

    Factory factory;
    machine = factory.getMachine(params, nodes.size());
    scheduler = factory.getScheduler(params, nodes.size(), *machine);
    theAllocator = factory.getAllocator(params, machine, this);
    theTaskMapper = factory.getTaskMapper(params, machine);
    FSTtype = factory.getFST(params);
    timePerDistance = factory.getTimePerDistance(params);

    string trace = params.find<std::string>("traceName");
    if (FSTtype > 0) {
        calcFST = new FST(FSTtype);  //must call calcFST -> setup() once we know the number of jobs (in other words, in setup())
    } else {
        calcFST = NULL;
    }

    //setup NetworkSim
    doDetailedNetworkSim = false;
    if (!params.find<std::string>("detailedNetworkSim").empty()){
        string temp_string = params.find<std::string>("detailedNetworkSim");
        if (temp_string.compare("ON") == 0){
            doDetailedNetworkSim = true;
            schedout.output("schedComp:Detailed Network sim is ON\n");
            snapshot = new Snapshot();
            // look for a file that lists all jobs that has been completed in ember
            if (params.find<std::string>("completedJobsTrace").empty()){
                schedout.fatal(CALL_INFO, 1, "detailedNetworkSim: You must specify a completedJobsTrace name!\n");
            }
            // look for a file that lists all jobs that are still running on ember
            if (params.find<std::string>("runningJobsTrace").empty()){
                schedout.fatal(CALL_INFO, 1, "detailedNetworkSim: You must specify a runningJobsTrace name!\n");
            }
        }
    }

    if (NULL != dynamic_cast<SimpleMachine*>(machine)) {
        char timestring[] = "time";
        stats = new Statistics(machine, scheduler, theAllocator, theTaskMapper, trace, timestring, false, calcFST);
    } else {
        if (doDetailedNetworkSim == false){
            char timestring[] = "time,alloc";
            stats = new Statistics(machine, scheduler, theAllocator, theTaskMapper, trace, timestring, false, calcFST);
        } else {
            char timestring[] = "time,alloc,snapshot.xml";
            stats = new Statistics(machine, scheduler, theAllocator, theTaskMapper, trace, timestring, false, calcFST);
        }
    }

    useYumYumSimulationKill = !params.find<std::string>("useYumYumSimulationKill").empty();
    YumYumSimulationKillFlag = false;

    if (!params.find<std::string>("YumYumPollWait").empty()) {
        YumYumPollWait = atoi(params.find<std::string>("YumYumPollWait").c_str() );
    }else{
        YumYumPollWait = 250;
    }

    useYumYumTraceFormat = !params.find<std::string>("useYumYumTraceFormat").empty();
    printYumYumJobLog = !params.find<std::string>("printYumYumJobLog").empty();
    printJobLog = !params.find<std::string>("printJobLog").empty();

    jobParser = new JobParser(machine, params, &useYumYumSimulationKill, &YumYumSimulationKillFlag, &doDetailedNetworkSim);

    machine -> reset();
    scheduler -> reset();

    jobLogFileName = params.find<std::string>("jobLogFileName");
    
    schedout.output("\n");
}


void schedComponent::setup()
{
    for( vector<SST::Link *>::iterator nodeIter = nodes.begin(); nodeIter != nodes.end(); nodeIter ++ ){
        // ask the newly-connected node for its ID
        SST::Event * getID = new CommunicationEvent( RETRIEVE_ID );
        (*nodeIter)->send( getID );

        CommunicationEvent * setNetworkSim = new CommunicationEvent( SET_DETAILED_NETWORK_SIM );
        setNetworkSim-> reply = doDetailedNetworkSim;
        (*nodeIter)->send( setNetworkSim );
    }
    // done setting up the links, now read the job list
    jobs = jobParser -> parseJobs(getCurrentSimTime());

    // NetworkSim: record the total number of jobs as the jobs vector changes over time
    numJobs = (int) jobs.size();
    
    if (doDetailedNetworkSim){
        //parse the ember completed/running job traces
        emberFinishedJobs = jobParser -> parseJobsEmberCompleted();
        emberRunningJobs = jobParser -> parseJobsEmberRunning();
        ignoreUntilTime = jobParser -> ignoreUntilTime;
        
        schedout.output("The jobs that have been completed in ember\n");
        for(std::map<int, unsigned long>::iterator it = emberFinishedJobs.begin(); it != emberFinishedJobs.end(); it++){
            schedout.output("Job %d: %lu\n", it->first, it->second);
        }

        schedout.output("ignoreUntilTime: %llu\n", this->ignoreUntilTime);

        schedout.output("The jobs that are still running in ember\n");
        for(std::map<int, std::pair<unsigned long, int>>::iterator iter = emberRunningJobs.begin(); iter != emberRunningJobs.end(); iter++){
            schedout.output("Job %d: soFarRunningTime: %lu currentMotifCount: %d\n", iter->first, iter->second.first, iter->second.second);
        }
    }
    
    for(int i = 0; i < (int) jobs.size(); i++)
    {
        ArrivalEvent* ae = new ArrivalEvent(jobs[i]->getArrivalTime(), i);
        if (useYumYumTraceFormat) {
            selfLink -> send(0, ae); 
        } else {
            selfLink -> send(jobs[i]->getArrivalTime(), ae); 
        }
    }
        
    if( useYumYumTraceFormat ){
        char* inputDir = getenv("SIMINPUT");
        string jobListFileName;
        if (inputDir != NULL) {
            jobListFileName = inputDir + trace;
        } else {
            jobListFileName = trace;
        }
        // in case there were no jobs in the joblist
        if( jobs.empty() ){
            unregisterYourself();
        }
        CommunicationEvent * commEvent = new CommunicationEvent( START_FILE_WATCH );
        commEvent->payload = & jobListFileName;
        selfLink->send( commEvent ); 
    }

    if (FSTtype > 0){
        calcFST -> setup(jobs.size());
    }
}


schedComponent::schedComponent() : Component(-1)
{
    // for serialization only
}


void schedComponent::registerThis()
{
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
    registrationStatus = true;
}


void schedComponent::unregisterThis()
{
    primaryComponentOKToEndSim();
    registrationStatus = false;
}


bool schedComponent::isRegistered()
{
    return registrationStatus;
}


// If we aren't using the Yum Yum functionality, this will just unregister and return;
// Otherwise, this will block, checking if there are new jobs at intervals defined by YumYumPollWait, or if the YumYumSimulationKillFlag has been set.
void schedComponent::unregisterYourself()
{
    if (useYumYumSimulationKill) {
        while( YumYumSimulationKillFlag != true && jobs.empty() ){
            std::this_thread::sleep_for( std::chrono::milliseconds( YumYumPollWait ) );
            if (jobParser -> checkJobFile()) {
                jobs = jobParser -> parseJobs(getCurrentSimTime());
                if (!jobs.empty()) {
                    break;
                }
            }
            if (YumYumSimulationKillFlag) {
                unregisterThis();
            }
        }
    } else {
        unregisterThis();
    }

    if (!isRegistered() && jobLog.is_open()) {
        jobLog.close();
    }
}


void schedComponent::startNextJob()
{
    CommunicationEvent * commEvent = new CommunicationEvent (START_NEXT_JOB);
    if (useYumYumTraceFormat) {
        selfLink -> send(1, commEvent); 
    } else {
        selfLink -> send(0, commEvent); 
    }
}


// incoming events are scanned and deleted. ev is the returned event,
// node is the node it came from.
void schedComponent::handleCompletionEvent(Event *ev, int node) 
{
    if (dynamic_cast<CommunicationEvent *>(ev)) {
        CommunicationEvent * commEvent = dynamic_cast<CommunicationEvent *> (ev);
        if( commEvent->CommType == RETRIEVE_ID &&
            commEvent->reply == true ){
                nodeIDs.insert(nodeIDs.begin() + node, *(std::string*)commEvent->payload);
        }
        delete ev;
    } else if (dynamic_cast<CompletionEvent*>(ev)) {
        CompletionEvent *event = dynamic_cast<CompletionEvent*>(ev);
        int jobNum = event->jobNum;
        if (NULL == runningJobs[jobNum].tmi) {
            runningJobs.erase(jobNum);
            delete ev;
            return; //this event has already stopped (probably faulted)
        }

        if (0 == (--(runningJobs[jobNum].i))) {
            runningJobs[jobNum].tmi -> job -> hasRun = true;
            if( printJobLog ){
                logJobFinish( runningJobs[ jobNum ] );
            }
            finishingcomp.push_back(event->copy());
            FinalTimeEvent *fte = new FinalTimeEvent();
            if(runningJobs[jobNum].tmi->job->getStartTime() == getCurrentSimTime())
                fte->forceExecute = true;
            selfLink->send(0, fte); //send back an event at the same time so we know it finished 
        }
        if (jobNum == jobs.back()->jobNum) {
            if (jobs.empty()) {
                unregisterYourself();
            }
        }
        delete ev;
    } else {
        //If it is not a completion it is (hopefully) a fault.
        //as far as the scheduler is concerned this is treated the same way
        //however, must send a kill event to tell all nodes with this job to
        //stop running (not all the nodes may have failed but all must stop)
        //for now, the job is dropped forever
        FaultEvent *event = dynamic_cast<FaultEvent*>(ev);
        if (event) {
            int jobNum = event->jobNum;
            if (jobNum == -1) {
                return; // the node that faulted was not running a job, nothing for 
                //scheduler, allocator, machine to do
                delete ev;
            }
            //force all nodes running the job to kill it

            TaskMapInfo *tmi = runningJobs[jobNum].tmi;
            if (tmi == NULL)
            {
                runningJobs.erase(jobNum);
                delete ev;
                return;
            }
            //if ai is NULL we already killed this job

            runningJobs[jobNum].tmi->job->hasRun = true;

            if (printJobLog) {
                logJobFault(runningJobs[jobNum], event);
            }

            for (int i = 0; i < tmi->job->getProcsNeeded(); ++i) {
                JobKillEvent *ec = new JobKillEvent(tmi -> job -> getJobNum());

                nodes[tmi->allocInfo->nodeIndices[i]] -> send(ec); 
            }

            machine -> deallocate(tmi);
            theAllocator -> deallocate(tmi->allocInfo);
            if (useYumYumTraceFormat) {
                stats->jobFinishes(tmi, getCurrentSimTime() + 1);
                scheduler->jobFinishes(tmi->job, getCurrentSimTime() + 1, *machine);
            } else {
                stats->jobFinishes(tmi, getCurrentSimTime() );
                scheduler->jobFinishes(tmi->job, getCurrentSimTime() , *machine);
            }
            //the job is done and deleted from our records; don't need
            delete runningJobs.find( jobNum )->second.tmi; 
            
            //its allocinfo again
            runningJobs.erase(jobNum);

            startNextJob();

            if (jobNum == jobs.back()->jobNum) {
                while (!jobs.empty() && 
                       runningJobs.find(jobs.back()->jobNum) == runningJobs.end() && 
                       jobs.back()->hasRun == true) {
                    delete jobs.back();
                    jobs.pop_back();
                }
                if (jobs.empty()) {
                    unregisterYourself();             // This is the one that actually does the unregistering.
                }
            }

            delete ev;
        } else {
            schedout.fatal(CALL_INFO, 1, "S: Error! Bad Event Type!\n");
        }
    }  
}

void schedComponent::handleJobArrivalEvent(Event *ev) 
{    
    schedout.debug(CALL_INFO, 4, 0, "arrival event\n");
    
    CommunicationEvent * commEvent = dynamic_cast<CommunicationEvent *>(ev);
    ArrivalEvent *arevent = dynamic_cast<ArrivalEvent*>(ev);
    FinalTimeEvent* fev = dynamic_cast<FinalTimeEvent*>(ev);
    SnapshotEvent* sev = dynamic_cast<SnapshotEvent*>(ev);
    
    if (NULL != commEvent) {
        schedout.debug(CALL_INFO, 4, 0, "comm event\n");

        if (commEvent->CommType == LOG_JOB_START) {
        } else if (commEvent -> CommType == START_FILE_WATCH) {
        } else if (commEvent -> CommType == START_NEXT_JOB) {
            bool startingNewJob = true;
            bool newJobsStarted = false; //NetworkSim: no new jobs have started yet
            while (startingNewJob) {
                Job* nextJob =  scheduler->tryToStart(getCurrentSimTime(), *machine);
                if(nextJob != NULL){
                    startJob(nextJob);
                    newJobsStarted = true; //NetworkSim: at least one new job has started and we will take a snapshot in startJob() function
                    if (FSTtype > 0) {
                        calcFST->jobStarts(nextJob, getCurrentSimTime());
                    }
                } else {
                    startingNewJob = false;
                    //NetworkSim: if we could not start any job, we will still take a snapshot at t = ignoreUntilTime
                    if (doDetailedNetworkSim && !newJobsStarted && !emberRunningJobs.empty() && getCurrentSimTime() == ignoreUntilTime){
                        // find the closest job arrival time after the current time
                        unsigned long NextArrivalTime = 0;
                        for(int i = (jobNumLastArrived + 1); i < numJobs; i++){
                            NextArrivalTime = jobs[i]->getArrivalTime();
                            if (NextArrivalTime > ignoreUntilTime){
                                break;
                            }
                        }
                        snapshot->append(getCurrentSimTime(), NextArrivalTime, runningJobs);
                        schedout.output("Next Job is arriving at %lu\n", NextArrivalTime);
                        unregisterYourself();
                    }                 
                }
            }
        }
        delete ev;
    } else if (NULL != arevent) {
        finishingarr.push_back(arevent);
        //NetworkSim: keep track of the last job that has arrived
        jobNumLastArrived = arevent->getJobIndex();
        
        FinalTimeEvent* fte = new FinalTimeEvent();
        if (useYumYumSimulationKill xor YumYumSimulationKillFlag) {
            fte -> forceExecute = true; // avoid intermittent hangs when using yumyum
        }
        else {
            fte -> forceExecute = false; // otherwise keep previous behavior
        }
        //send back an event at the same time so we know it finished 
        selfLink -> send(0, fte); 
    } else if (NULL != fev) {
        if (lastfinaltime == getCurrentSimTime() && !fev -> forceExecute) {
            delete ev;
            return; //ignore duplicate final time events 
            //(unless the event forces us to handle them)
        } else {
            lastfinaltime = getCurrentSimTime();
        }
        if (finishingcomp.size() > 1) {
            //finishingcomp.sort(compareCEPointers); 
        }

        while (!finishingcomp.empty()) {
            int finishedJobNum = finishingcomp.front() -> jobNum;
            TaskMapInfo *tmi = runningJobs[finishedJobNum].tmi;
            runningJobs.erase(finishedJobNum);
            machine->deallocate(tmi);
            theAllocator->deallocate(tmi->allocInfo);
            
            if (useYumYumTraceFormat) {
                stats -> jobFinishes(tmi, getCurrentSimTime() + 1);
                scheduler -> jobFinishes(tmi->job, getCurrentSimTime() + 1, *machine);
            } else {
                if (FSTtype > 0){
                    calcFST -> jobCompletes(tmi->job);
                }
                stats -> jobFinishes(tmi, getCurrentSimTime());
                scheduler -> jobFinishes(tmi->job, getCurrentSimTime(), *machine);
            }
            delete tmi;

            if (finishedJobNum == jobs.back()->jobNum) {
                while (!jobs.empty() && 
                       runningJobs.find(jobs.back()->jobNum) == runningJobs.end() && 
                       jobs.back()->hasRun == true) {
                    delete jobs.back();
                    jobs.pop_back();
                }

                if (jobs.empty()) {
                    unregisterYourself();
                }
            }
            delete finishingcomp.front();
            finishingcomp.pop_front();
        } 
        //arrivals are handled strictly after finishes at the same time to
        //match the Java simulator
        //each time step these are cleared so we don't need to worry about
        //events not at the same time step, and they should already be sorted
        //by number because they are given to SST (and therefore come back) that way.
        while (!finishingarr.empty()) {
            Job* arrivingjob = jobs[finishingarr.front() -> getJobIndex()];
            if (FSTtype == 2) { 
                //relaxed, so do FST before we tell the scheduler about the job
                calcFST -> jobArrives(arrivingjob, scheduler, machine);
                finishingarr.front() -> happen(*machine, theAllocator, 
                                               scheduler, stats, arrivingjob);
            } else if (FSTtype == 1){
                finishingarr.front() -> happen(*machine, theAllocator, 
                                               scheduler, stats, arrivingjob);
                calcFST -> jobArrives(arrivingjob, scheduler, machine);
            } else {
                finishingarr.front() -> happen(*machine, theAllocator, 
                                               scheduler, stats, arrivingjob);
            }
            delete finishingarr.front();
            finishingarr.pop_front();
        }      
        delete ev; 

        startNextJob();
    } else if (NULL != sev){ //snapshot event
        //dump snapshot to file
        schedout.output("%llu:Snapshot event received...Appending snapshot...\n", getCurrentSimTime());        
        snapshot->append(sev->time, sev->nextJobArrivalTime, sev->runningJobs);
        delete ev;
        unregisterYourself();
    } else {
        schedout.fatal(CALL_INFO, 1, "Arriving event was not an arrival nor finaltime event");
    }
    schedout.debug(CALL_INFO, 4, 0, "finishing arrival event\n");
}

void schedComponent::finish() 
{
    scheduler -> done();
    if (doDetailedNetworkSim) {
        //Write snapshot to file only once at the end
        if (!snapshot -> getSimFinished()) {
            schedout.output("In Sched component: sim paused\n");
            stats -> simPauses(snapshot, getCurrentSimTime());
        } else {
            schedout.output("In Sched component: sim finished\n");
        }
    }
    stats -> done();
    theAllocator -> done();
}

void schedComponent::startJob(Job* job) 
{
    //allocate & update machine
    CommParser commParser = CommParser();
    commParser.parseComm(job);                      //read communication files
    
    if (doDetailedNetworkSim) {
        //update startingMotif for the jobs that are still running on ember
        if(emberRunningJobs.find(job->getJobNum()) != emberRunningJobs.end()){
            job->phaseInfo.soFarRunningTime = emberRunningJobs[job->getJobNum()].first;
            job->phaseInfo.startingMotif = emberRunningJobs[job->getJobNum()].second;
        }
    }
    job->start( getCurrentSimTime() );              //job started flag
    AllocInfo* ai = theAllocator->allocate(job);    //get allocation
    TaskMapInfo* tmi = theTaskMapper->mapTasks(ai); //map tasks
    machine->allocate(tmi);                         //allocate
    scheduler->startNext(getCurrentSimTime(), *machine); //start in scheduler
    stats->jobStarts(tmi, getCurrentSimTime() );    //record stats
    
    //calculate running time with communication overhead
    int* jobNodes = ai->nodeIndices;
    unsigned long actualRunningTime = job->getActualTime();
    
    SimpleMachine* sMachine = dynamic_cast<SimpleMachine*>(machine);

    if (timePerDistance -> at(0) != 0 && NULL == sMachine && NULL != ai) {
        double randomNumber = (rng->nextUniform() * 2 - 1) * timePerDistance->at(2);

        //calculate baseline hopBytes
        AllocInfo* baselineAlloc = machine->getBaselineAllocation(job);
        SimpleTaskMapper baselineMapper = SimpleTaskMapper(*machine);
        TaskMapInfo* baselineMap = baselineMapper.mapTasks(baselineAlloc);
        double baselineHopBytes = baselineMap->getHopBytes();
        delete baselineMap;

        //calculate running time w/o any communication overhead:
        double baselineRunningTime;
        baselineRunningTime = actualRunningTime / (1 + timePerDistance->at(0) * pow(baselineHopBytes, timePerDistance->at(1) + randomNumber));

        //get new hopBytes
        double hopBytes = tmi->getHopBytes();
        double timeWithComm = baselineRunningTime * (1 + timePerDistance->at(0) * pow(hopBytes, timePerDistance->at(1) + randomNumber));

        //overestimate the fraction to keep min job length at 1 
        actualRunningTime = ceil(timeWithComm);
    }

    if (actualRunningTime > job->getEstimatedRunningTime()) {
        schedout.fatal(CALL_INFO, 1, "Job %lu has running time %lu, which is longer than estimated running time %lu\n", job->getJobNum(), actualRunningTime, job->getEstimatedRunningTime());
    }

    bool emberFinished = false;
    if (doDetailedNetworkSim) {
        //Update actual running time of the job if it has run and finished on ember,
        //define a boolean variable showing that
        if (emberFinishedJobs.find(job->getJobNum()) != emberFinishedJobs.end()) {
            actualRunningTime = emberFinishedJobs[job->getJobNum()];
            emberFinished = true;
        }
    }

    // send to each job in the node list
    for (int i = 0; i < ai->getNodesNeeded(); ++i) {
        JobStartEvent *ec = new JobStartEvent(actualRunningTime, job->getJobNum());
        ec->emberFinished = emberFinished; //NetworkSim: add the emberFinished info to the event
        nodes[jobNodes[i]] -> send(ec); 
    }

    ITMI itmi;
    itmi.i = ai->getNodesNeeded();
    itmi.tmi = tmi;
    runningJobs[job->getJobNum()] = itmi;

    if (printJobLog) {
        logJobStart(itmi);
    }

    if (doDetailedNetworkSim == true && getCurrentSimTime() >= ignoreUntilTime ){
        //Take a snapshot whenever a job is allocated/mapped to start
        SnapshotEvent *se = new SnapshotEvent(getCurrentSimTime(), job->getJobNum());
        se->runningJobs = runningJobs;

        schedout.output("Taking snapshot as Job %ld is starting...\n", job->getJobNum());

        int ii;
        for(ii = (jobNumLastArrived + 1); ii < numJobs; ii++){
            se->nextJobArrivalTime = jobs[ii]->getArrivalTime();
            if (se->nextJobArrivalTime > ignoreUntilTime){
                break;
            }
        }

        if (ii == numJobs) {
            schedout.output("All jobs have arrived!\n");
        } else {
            schedout.output("Next Job: %ld is arriving at %lu\n", jobs[ii]->getJobNum(), se->nextJobArrivalTime);
        }
        selfLink->send(se);
        schedout.output("%llu: Sent snapshot event to self\n", getCurrentSimTime());
    }
}

void schedComponent::logJobStart(ITMI itmi)
{
    if (!jobLog.is_open()) {
        jobLog.open( this->jobLogFileName.c_str(), ios::out );
        if (jobLog.is_open()) {
            jobLog << "jobid, begin, end, failed, compute_nodes" << endl;
            // writing the header here should be safe, since the jobs should start before they end
        }
    }

    if (jobLog.is_open()) {
        jobLog << *itmi.tmi->job->getID() << "," << getCurrentSimTime() << ",-1,-1,";
        for( int counter = 0; counter < itmi.tmi->job->getProcsNeeded(); counter ++ ){
            if (counter > 0) {
                jobLog << " ";
            }
            jobLog << nodeIDs.at( itmi.tmi->allocInfo->nodeIndices[ counter ] );
        }
        jobLog << endl;
    } else {
        schedout.fatal(CALL_INFO, 1, "Couldn't open %s for writing the job log.", this->jobLogFileName.c_str());
    }
}

void schedComponent::logJobFinish(ITMI itmi)
{
    if (!jobLog.is_open()) {
        jobLog.open( this->jobLogFileName.c_str(), ios::out );
    }

    if (jobLog.is_open()) {
        jobLog << *itmi.tmi->job->getID() << "," << itmi.tmi->job->getStartTime() << "," << getCurrentSimTime() << ",0,";
        for( int counter = 0; counter < itmi.tmi->job->getProcsNeeded(); counter ++ ){
            if (counter > 0) {
                jobLog << " ";
            }
            jobLog << nodeIDs.at( itmi.tmi->allocInfo->nodeIndices[ counter ] );
        }
        jobLog << endl;
    } else {
        schedout.fatal(CALL_INFO, 1, "Couldn't open %s for writing the job log.", this->jobLogFileName.c_str());
    } 

    if (getCurrentSimTime() < itmi.tmi->job->getStartTime()) {
        schedout.fatal(CALL_INFO, 1, "Job begin time larger than end time: jobid=%s, begin=%lu, end=%" PRIu64 "\n", (*itmi.tmi -> job -> getID()).c_str(), itmi.tmi -> job -> getStartTime(), getCurrentSimTime());
    }
}

void schedComponent::logJobFault(ITMI itmi, FaultEvent * faultEvent)
{
    if (!jobLog.is_open()) {
        jobLog.open( this->jobLogFileName.c_str(), ios::out );
    }

    if (jobLog.is_open()) {
        jobLog << *itmi.tmi->job->getID() << "," << itmi.tmi->job->getStartTime() << "," << getCurrentSimTime() << ",1,";
        for( int counter = 0; counter < itmi.tmi->job->getProcsNeeded(); counter ++ ){
            if (counter > 0) {
                jobLog << " ";
            }
            jobLog << nodeIDs.at( itmi.tmi->allocInfo->nodeIndices[ counter ] );
        }

        jobLog << endl;
    }else{
        schedout.fatal(CALL_INFO, 1, "Couldn't open %s for writing the job log.", this->jobLogFileName.c_str() );
    } 

    if (getCurrentSimTime() < itmi.tmi->job->getStartTime()) {
        schedout.fatal(CALL_INFO, 1, "Job begin time larger than end time: jobid=%s, begin=%lu, end=%" PRIu64 "\n", (*itmi.tmi -> job -> getID()).c_str(), itmi.tmi->job->getStartTime(), (uint64_t)getCurrentSimTime());
    }
}

// Element Libarary / Serialization stuff

//BOOST_CLASS_EXPORT(SST::Scheduler::ArrivalEvent)
//BOOST_CLASS_EXPORT(SST::Scheduler::schedComponent)

