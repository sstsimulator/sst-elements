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
#include "nodeComponent.h"

#include <stdlib.h>
#include <assert.h>
#include <sstream>
#include <fstream>
#include <climits>
#include <cmath>
#include <stdio.h>

#include "sst/core/element.h"
#include <sst/core/params.h>
#include <sst/core/stringize.h>

#include "events/CommunicationEvent.h"
#include "events/CompletionEvent.h"
#include "events/FaultEvent.h"
#include "events/JobStartEvent.h"
#include "events/JobKillEvent.h"
#include "events/ObjectRetrievalEvent.h"

#include "output.h"
#include "misc.h"

using namespace SST;
using namespace SST::Scheduler;
using namespace std;

int yumyumFaultRand48Seed;
int yumyumErrorLogRand48Seed;
int yumyumErrorLatencyRand48Seed;
int yumyumErrorCorrectionRand48Seed;
int yumyumJobKillRand48Seed;

static std::ofstream faultLog;
static std::ofstream errorLog;

void readCSVpairsIntoMap( Tokenizer< escaped_list_separator > Tokenizer, std::map<std::string, float> * Map )
{
    std::vector<std::string> tokens;
    tokens.assign( Tokenizer.begin(), Tokenizer.end() );

    for ( unsigned int counter = 0; counter < tokens.size(); counter += 2 ){
        trim( tokens.at( counter ) );
        trim( tokens.at( counter + 1 ) );
        Map -> insert( std::pair<std::string, float>( tokens.at( counter ), atof( tokens.at( counter + 1 ).c_str() ) ) );
    }
}



void readDelaysIntoMap( Tokenizer< escaped_list_separator > Tokenizer, std::map<std::string, std::pair<unsigned int, unsigned int> > * FaultLatencyBounds, int nodeNum ){
    std::vector<std::string> tokens;
    tokens.assign(Tokenizer.begin(), Tokenizer.end());

    for(unsigned int counter = 0; counter + 2 < tokens.size(); counter += 3){
        trim(tokens.at(counter));
        trim(tokens.at(counter + 1));
        trim(tokens.at(counter + 2));

        std::string faultName = tokens.at(counter);
        unsigned int lowerLatencyBound = atoi(tokens.at(counter + 1).c_str());
        unsigned int upperLatencyBound = atoi(tokens.at(counter + 2).c_str());

        if (upperLatencyBound < lowerLatencyBound){
            //std::cerr << "nodeNum " << nodeNum << "'s fault delay upper bound is lower than its lower bound: " << tokens.at(counter) << ", " << tokens.at(counter + 1) << ", " << tokens.at(counter + 2) <<  std::endl;
            //error("Bad delay bounds");
            schedout.fatal(CALL_INFO, 1, "nodeNum %d's fault delay upper bound is lower than its lower bound: %s, %s, %s\nBad delay bounds", nodeNum, tokens.at(counter).c_str(), tokens.at(counter + 1).c_str(), tokens.at(counter + 2).c_str());
        }

        FaultLatencyBounds -> insert(std::pair<std::string, std::pair<unsigned int, unsigned int> >(faultName, std::pair<unsigned int, unsigned int>(lowerLatencyBound, upperLatencyBound)));
    }
}



nodeComponent::nodeComponent(ComponentId_t id, Params& params) :
    Component(id), jobNum(-1) 
{
    schedout.init("", 8, ~0, Output::STDOUT);

    if (params.find<std::string>("nodeNum").empty()) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1,"couldn't find the nodeNum param for this node\n");
    }
    nodeNum = params.find<int64_t>("nodeNum",0);
    ID = params.find<std::string>("id");
    nodeType = params.find<std::string>("type");
    faultLogFileName = params.find<std::string>("faultLogFileName");
    errorLogFileName = params.find<std::string>("errorLogFileName");

    Scheduler = configureLink("Scheduler", SCHEDULER_TIME_BASE, new Event::Handler<nodeComponent>(this, &nodeComponent::handleEvent));
    Builder = configureLink("Builder", SCHEDULER_TIME_BASE, new Event::Handler<nodeComponent>( this, &nodeComponent::handleEvent ));
    failureInjector = configureLink("faultInjector", SCHEDULER_TIME_BASE, new Event::Handler<nodeComponent>( this, &nodeComponent::handleEvent ));
    SelfLink = configureSelfLink("linkToSelf", SCHEDULER_TIME_BASE, new Event::Handler<nodeComponent>(this, &nodeComponent::handleSelfEvent));
    FaultLink = configureSelfLink("SelfFaultLink", SCHEDULER_TIME_BASE, new Event::Handler<nodeComponent>(this, &nodeComponent::handleFaultEvent));

    SST::Link * tmp;
    char port[16];

    for (int counter = 0; counter == 0 || tmp != NULL; counter++){
        sprintf(port, "Parent%d", counter);

        tmp = configureLink(port, SCHEDULER_TIME_BASE, new Event::Handler<nodeComponent>(this, &nodeComponent::handleEvent));
        if (tmp){
            ParentFaultLinks.push_back(tmp);
        }
    }

    for(int counter = 0; counter == 0 || tmp != NULL; counter ++){
        sprintf(port, "Child%d", counter);

        tmp = configureLink(port, SCHEDULER_TIME_BASE, new Event::Handler<nodeComponent>(this, &nodeComponent::handleEvent));
        if (tmp){
            ChildFaultLinks.push_back(tmp);
        }
    }

    if(!( (((int) ChildFaultLinks.size()) > 0) || Scheduler) ){
        schedout.fatal(CALL_INFO, 1, "Node number %d has neither children nor a link to the scheduler.\nInvalid node", nodeNum);
        //std::cerr << "Node number " << nodeNum << " has neither children nor a link to the scheduler." << std::endl;
        //error("Invalid node");
    }


    SelfLink -> setDefaultTimeBase(registerTimeBase(SCHEDULER_TIME_BASE));

    readCSVpairsIntoMap(Tokenizer< escaped_list_separator >(params.find<std::string>("faultActivationRate")), &Faults);
    readDelaysIntoMap(Tokenizer< escaped_list_separator >(params.find<std::string>("errorPropagationDelay")), &FaultLatencyBounds, nodeNum);
    readCSVpairsIntoMap(Tokenizer< escaped_list_separator >(params.find<std::string>("errorCorrectionProbability")), &errorCorrectionProbability);
    readCSVpairsIntoMap(Tokenizer< escaped_list_separator >(params.find<std::string>("errorMessageProbability")), &errorLogProbability);
    readCSVpairsIntoMap(Tokenizer< escaped_list_separator >(params.find<std::string>("jobFailureProbability")), &jobKillProbability);

	yumyumFaultRand48State = (unsigned short *) malloc(3 * sizeof(short));
	yumyumErrorLogRand48State = (unsigned short *) malloc(3 * sizeof(short));
	yumyumErrorLatencyRand48State = (unsigned short *) malloc(3 * sizeof(short));
	yumyumErrorCorrectionRand48State = (unsigned short *) malloc(3 * sizeof(short));
	yumyumJobKillRand48State = (unsigned short *) malloc(3 * sizeof(short));

	faultsActivated = false;

    //set our clock
    setDefaultTimeBase(registerTimeBase(SCHEDULER_TIME_BASE));

    doDetailedNetworkSim = false; //NetworkSim: by default it is false
}


void nodeComponent::setup()
{ 
	yumyumFaultRand48State[0] = 0x330E;
	yumyumFaultRand48State[1] = (yumyumFaultRand48Seed ^ nodeNum) & 0xFFFF;
	yumyumFaultRand48State[2] = (yumyumFaultRand48Seed ^ nodeNum) >> 16;

	yumyumErrorLogRand48State[0] = 0x330E;
	yumyumErrorLogRand48State[1] = (yumyumErrorLogRand48Seed ^ nodeNum) & 0xFFFF;
	yumyumErrorLogRand48State[2] = (yumyumErrorLogRand48Seed ^ nodeNum) >> 16;

	yumyumErrorLatencyRand48State[0] = 0x330E;
	yumyumErrorLatencyRand48State[1] = (yumyumErrorLatencyRand48Seed ^ nodeNum) & 0xFFFF;
	yumyumErrorLatencyRand48State[2] = (yumyumErrorLatencyRand48Seed ^ nodeNum) >> 16;

	yumyumErrorCorrectionRand48State[0] = 0x330E;
	yumyumErrorCorrectionRand48State[1] = (yumyumErrorCorrectionRand48Seed ^ nodeNum) & 0xFFFF;
	yumyumErrorCorrectionRand48State[2] = (yumyumErrorCorrectionRand48Seed ^ nodeNum) >> 16;

	yumyumJobKillRand48State[0] = 0x330E;
	yumyumJobKillRand48State[1] = (yumyumJobKillRand48Seed ^ nodeNum) & 0xFFFF;
	yumyumJobKillRand48State[2] = (yumyumJobKillRand48Seed ^ nodeNum) >> 16;

	faultsActivated = true;
	SelfLink -> send(new CommunicationEvent(START_FAULTING));
}


nodeComponent::nodeComponent() : Component(-1)
{
    // for serialization only
}


void nodeComponent::addLink(SST::Link * link, enum linkTypes type){
    //  asm( "int $3" );
    if (type == PARENT){
        ParentFaultLinks.push_back(link);
    }else if (type == CHILD){
        ChildFaultLinks.push_back(link);
    }
}


void nodeComponent::rmLink(SST::Link * link, enum linkTypes type)
{
    schedout.fatal(CALL_INFO, 1, "Can't remove links just yet\n");
    //internal_error("Can't remove links just yet\n");
}


void nodeComponent::disconnectYourself()
{
    ChildFaultLinks.empty();
    ParentFaultLinks.empty();
}


std::string nodeComponent::getType()
{
    return nodeType;
}

std::string nodeComponent::getID()
{
    return ID;
}


void nodeComponent::handleJobKillEvent(JobKillEvent * killEvent)
{
    if (killEvent -> jobNum == this -> jobNum && Scheduler){
        CompletionEvent *ec = new CompletionEvent(jobNum);
        Scheduler -> send(ec); 

        killedJobs.insert(std::pair<int, int>(jobNum,jobNum));
        jobNum = -1;
    } else {
#ifdef JRSDEBUG
        std::cout << "A node was asked to kill a job it was not running.\n" << std::endl;
#endif
    }
}


/*
 * finds the latency that should be used when passing a fault to a child node.
 * if the user didn't specify a bound, it will be zero.
 */
unsigned int nodeComponent::genFaultLatency( std::string faultName )
{
    unsigned int latency = 0;
    if (FaultLatencyBounds.find(faultName) != FaultLatencyBounds.end()){
        std::pair<unsigned int, unsigned int> bounds = (*FaultLatencyBounds.find(faultName)).second;
        if (bounds.first == 0 && bounds.second == 0){    // bounds of [0, 0] were specified, so use latency of zero
            //      std::cout << "no bounds, using 0" << std::endl;
        } else if (bounds.second == bounds.first){    // no upper bound, so just use the lower bound as our latency
            //      std::cout << "same bounds" << std::endl;
            latency = bounds.first;
        } else {      // we have an upper bound, so find a random latency.
            //      std::cout << "both bounds, calculating: " << bounds.first << " --- " << bounds.second << std::endl;
            latency = (((unsigned int)jrand48( yumyumErrorLatencyRand48State )) % (bounds.second - bounds.first + 1)) + bounds.first;
            // random number uniformly distributed between the bounds.
        }
    }

    //  std::cout << "sending fault type " << faultName << " in " << latency << " steps" << std::endl;

    return latency;
}



/*
 * Examines the fault in comparison to this node's correction probability to determine
 * if the error should be corrected or not.
 */
bool nodeComponent::canCorrectError( FaultEvent * error )
{
    return errorCorrectionProbability.find( error -> faultType ) != errorCorrectionProbability.end() &&
        erand48( yumyumErrorCorrectionRand48State ) < (errorCorrectionProbability.find( error -> faultType ) -> second);
}



/*
 * handles faults for this node - propagation, correction, jobkills, etc
 */
void nodeComponent::handleFaultEvent(SST::Event * ev)
{
    if (dynamic_cast<FaultEvent*>(ev)) {
        FaultEvent * faultEvent = dynamic_cast<FaultEvent*>(ev);

#ifdef JRSDEBUG
        cerr << getCurrentSimTime() << ": in handleFaultEvent for node " << this->ID << endl;
#endif

        logError(faultEvent);

        if (!canCorrectError(faultEvent)) {
            if (Scheduler && jobNum != -1) {
                /* Kill the job if:
                 * 
                 * - The fault event doesn't say to kill the job or let it live and EITHER
                 *   - there is no jobKillProbability associated with this fault type OR
                 *   - a random number is under the jobKillProbability
                 * - OR the fault event says to kill the job
                 */
                if (faultEvent -> shouldKillJob == FAULT_EVENT_SHOULDKILL
                    || (jobKillProbability.find( faultEvent -> faultType ) == jobKillProbability.end()
                        || erand48( yumyumJobKillRand48State ) < jobKillProbability.find( faultEvent -> faultType ) -> second) ) {

                    //SelfLink->send( getCurrentSimTime(), new JobKillEvent( this->jobNum ) ); 

                    faultEvent -> jobNum = jobNum;
                    faultEvent -> nodeNumber = nodeNum;
                    Scheduler -> send((unsigned int)genFaultLatency(faultEvent -> faultType), faultEvent -> copy()); 
                    // send the fault on to the scheduler.  It should tell the other nodes to kill the job.

                }
            } else {
                if (faultEvent->shouldKillJob == FAULT_EVENT_UNDECIDED &&
                    jobKillProbability.find( faultEvent -> faultType ) != jobKillProbability.end()) {
                    // undecided if fault should kill a job, and we have a JobKillProbability for this fault type
                    if (erand48(yumyumJobKillRand48State) < jobKillProbability.find(faultEvent -> faultType) -> second) {
                        faultEvent -> shouldKillJob = FAULT_EVENT_SHOULDKILL;
                    }
                }
                for (std::vector<SST::Link *>::iterator it = ChildFaultLinks.begin(); it != ChildFaultLinks.end(); ++it) {
                    (*it) -> send((unsigned int)genFaultLatency( faultEvent -> faultType ), faultEvent -> copy()); 
                }
            }
        }

        delete ev;
    }
}


// incoming events start a new job
void nodeComponent::handleEvent(Event *ev) {
    if (dynamic_cast<CommunicationEvent *>( ev )){
        CommunicationEvent * event = dynamic_cast<CommunicationEvent*>(ev);
        if( event->CommType == FAIL_JOBS ){
            FaultEvent * specialFault = new FaultEvent( std::string( *(string *)event->payload ) );
            specialFault->shouldKillJob = FAULT_EVENT_SHOULDKILL;
        	logFault( specialFault );
            handleFaultEvent( specialFault );
        }else if (event -> CommType == RETRIEVE_ID) {
            if( event->payload == NULL ){
                event -> payload = &this -> ID;
                event -> reply = true;

                Scheduler -> send(event); 
                return;
            }else{
                event->payload = &this->ID;
                event->reply = true;

                failureInjector->send( event );
                return;
            }
        //NetworkSim: added comm event type to set the deDetailedNetworkSim parameter
        } else if (event -> CommType == SET_DETAILED_NETWORK_SIM) {
            this -> doDetailedNetworkSim = event->reply;
        //end->NetworkSim
        } else {
            schedout.output("WARN: unhandled event\n");
        }

        delete event;
        return;

    } else if (dynamic_cast<ObjectRetrievalEvent*>(ev)){
        ObjectRetrievalEvent * event = dynamic_cast<ObjectRetrievalEvent*>(ev);

        event -> payload = this;

        Builder -> send(event); 
        return;
    }


    JobStartEvent *event = dynamic_cast<JobStartEvent*>(ev);
    if (event) {  
        if (-1 == jobNum) {
            //std::cout << "Received JobStartEvent at Node " << this->nodeNum << std::endl;//NetworkSim: debug
            jobNum = event -> jobNum;
            //NetworkSim: If the job is still running on ember, we do not know the actual running time. Thus, do not send delayed event to yourself.
            if(doDetailedNetworkSim == false || event->emberFinished == true){
                SelfLink -> send(event -> time, event);
            } else {
                delete event;
            }
            //end->NetworkSim
        } else {
            schedout.fatal(CALL_INFO, 1, "Error?! Already running a job, but given a new one!\n");
            //internal_error("Error?! Already running a job, but given a new one!\n");
        }
    } else if(dynamic_cast<FaultEvent*>(ev)){

        handleFaultEvent(ev);

    } else if (dynamic_cast<JobKillEvent*>(ev)) { 
        handleJobKillEvent(dynamic_cast<JobKillEvent*>(ev));
        delete ev;
    } else {
        //char errorMessage[1024];
        //snprintf(errorMessage, 1023,"Error! Bad Event Type %s in %s in %s:%d\n", typeid( *ev ).name(), __func__, __FILE__, __LINE__ );
        //internal_error(errorMessage);
        schedout.fatal(CALL_INFO, 1, "Error! Bad Event Type %s\n", typeid( *ev ).name());
    }
}


/*
 *  Handle all events coming in on the self link
 */
void nodeComponent::handleSelfEvent(Event *ev) 
{
    JobStartEvent *event = dynamic_cast<JobStartEvent*>(ev);
    if (event) {
        if (killedJobs.erase(event -> jobNum) == 1) {
        } else if (event -> jobNum == jobNum) {
            CompletionEvent *ec = new CompletionEvent(jobNum);
            Scheduler -> send(ec); 
            jobNum = -1;
        } else {
            schedout.fatal(CALL_INFO, 1, "Error!! We are not running this job we're supposed to finish!\n");
            //internal_error("Error!! We are not running this job we're supposed to finish!\n");
        }
        delete ev;
    } else if (dynamic_cast<FaultEvent*>(ev)) {
        FaultEvent * faultEvent = dynamic_cast<FaultEvent*>(ev);

        logFault(faultEvent);
        sendNextFault(std::string(faultEvent -> faultType));     // handled this fault, send another fault to the future
        handleFaultEvent(ev);
    } else if (dynamic_cast<JobKillEvent*>(ev)) {
        handleJobKillEvent(dynamic_cast<JobKillEvent*>(ev));
        delete ev;
    } else if (dynamic_cast<CommunicationEvent*>(ev)) {
        CommunicationEvent * commEvent = dynamic_cast<CommunicationEvent*>(ev);
        if (commEvent->CommType == START_FAULTING) {
            // begin sending faults
            for (std::map<std::string, float>::iterator faultIter = Faults.begin(); faultIter != Faults.end(); faultIter ++) {
                sendNextFault((*faultIter).first);
            }
        }
    } else {
        //char errorMessage[1024];
        //snprintf(errorMessage, 1023, "Error! Bad Event Type %s in %s in %s:%d\n", typeid( *ev ).name(), __func__, __FILE__, __LINE__ );
        //internal_error(errorMessage);
        schedout.fatal(CALL_INFO, 1, "Error! Bad Event Type %s\n", typeid( *ev ).name());
    }
}


double urand( unsigned short int * seed ) 
{
    //	return(rand() + 1.0)/(RAND_MAX + 1.0);
    return (((int)nrand48(seed)) + 1.0) / (pow(2.0, 31) + 1.0);
    // using nrand48 which I can use to better control the seed.
}

SimTime_t genexp(double lambda, unsigned short int * seed)
{
    double u,x;
    u = urand( seed );
    x = (-1 / lambda) * log(u);
    return std::abs(x);
}


void nodeComponent::sendNextFault(std::string faultType)
{
    if (Faults.find( faultType.c_str() ) == Faults.end()) {
        schedout.fatal(CALL_INFO, 1, "Error, recieved a fault with a type that is unknown.\n");
        //internal_error("Error, recieved a fault with a type that is unknown.\n");
    }

    uint32_t lambdaScale = 86400;     // lambda scaled from seconds to days

    SimTime_t fail_time = genexp(Faults.find(faultType.c_str()) -> second / lambdaScale, yumyumFaultRand48State);

#ifdef JRSDEBUG
    cerr << getCurrentSimTime() << ": in sendNextFault for node " << this -> ID << " with lambda " << Faults.find(faultType.c_str()) -> second << ", next faultTime is " << fail_time << endl;
#endif

    /* A Lambda near zero causes the next time to be -Inf.  That doesn't translate well to integer types and we get a fault time of zero.
     * Check for this (and any other floating point errors) and don't throw the fault if an error happened.
     * Very large fail_times also appear to cause the core to send the fault back to us as though it were zero.
     * Don't just check if the fault time is zero, the PRNG could be at fault for that one, and it would be OK.
     */
    if (std::isfinite(-1 / (Faults.find( faultType.c_str() ) -> second / lambdaScale))) {
        SelfLink -> send(fail_time, new FaultEvent(faultType)); 
    }
}



/*
   Uses the probability in errorLogProbability to determine if a fault should be logged.
   If it should be logged, it is logged.
   */
void nodeComponent::logError(FaultEvent * faultEvent)
{
    if (!errorLog.is_open()) {
#ifdef JRSDEBUG
        cerr << getCurrentSimTime() << ": in logError.open for node " << this->ID << endl;
#endif
        errorLog.open(this -> errorLogFileName.c_str());
        if (!errorLog.is_open()) {
            cerr << "Failed to open " << this -> errorLogFileName << endl;
            exit(1);
        }
        errorLog << "time,host,type" << endl;
    }

    /* Log the symptom if
     *
     * a) we could open the log
     * b) EITHER there is a symptom logging probability associated with this fault and a random number [0, 1) is less than the probability OR
     * c) there is no probability associated with this type of fault
     */
    if ((this -> errorLogProbability.find(faultEvent -> faultType) != this -> errorLogProbability.end() &&
         erand48(yumyumErrorLogRand48State) < this -> errorLogProbability.find(faultEvent -> faultType) -> second) ||
        this -> errorLogProbability.find(faultEvent -> faultType) == this -> errorLogProbability.end()) {
        errorLog << getCurrentSimTime() << "," << this -> ID << "," << faultEvent -> faultType << endl;
    }
}


/*
   Logs the "ground truth" to the fault log.  This is used to measure the accuracy of the Analysis Tool
   */
void nodeComponent::logFault(FaultEvent * faultEvent)
{
    if (this -> faultLogFileName.length() > 0) {
        if (!faultLog.is_open()) {
#ifdef JRSDEBUG
            cerr << getCurrentSimTime() << ": in logFault.open for node " << this -> ID << endl;
#endif
            faultLog.open(this -> faultLogFileName.c_str());
            if (!faultLog.is_open()) {
                cerr << "Failed to open " << this -> faultLogFileName << endl;
                exit(1);
            }
            faultLog << "time,host,type" << endl;
        }

        faultLog << getCurrentSimTime() << "," << this -> ID << "," << faultEvent -> faultType << endl;
    }
}



// Element Libarary / Serialization stuff

//BOOST_CLASS_EXPORT(SST::Scheduler::nodeComponent)

