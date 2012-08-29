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

#include <sstream>
#include <fstream>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

#include "sst/core/element.h"

#include "ObjectRetrievalEvent.h"
#include "CommunicationEvent.h"
#include "nodeComponent.h"
#include "JobFaultEvent.h"
#include "JobKillEvent.h"

#include "misc.h"

using namespace SST;

nodeComponent::nodeComponent(ComponentId_t id, Params_t& params) :
  Component(id), jobNum(-1) {

  if ( params.find("nodeNum") == params.end() ) {
    _abort(event_test,"couldn't find the nodeNum param for this node\n");
  }
  nodeNum = strtol( params[ "nodeNum" ].c_str(), NULL, 0 );
  ID = params[ "id" ];
  nodeType = params[ "type" ];
  faultLogFileName = params[ "faultLogFileName" ];
  symptomLogFileName = params[ "symptomLogFileName" ];

  Scheduler = configureLink( "Scheduler", new Event::Handler<nodeComponent>(this, &nodeComponent::handleEvent) );
  Builder = configureLink( "Builder", new Event::Handler<nodeComponent>( this, &nodeComponent::handleEvent ) );
  SelfLink = configureSelfLink("linkToSelf", new Event::Handler<nodeComponent>(this, &nodeComponent::handleSelfEvent));
  FaultLink = configureSelfLink("SelfFaultLink", new Event::Handler<nodeComponent>(this, &nodeComponent::handleFaultEvent));
    
  SST::Link * tmp;
  char port[ 16 ];
	
	for( int counter = 0; counter == 0 || tmp != NULL; counter ++ ){
	  sprintf( port, "Parent%d", counter );
	  
	  tmp = configureLink( port,
		     new Event::Handler<nodeComponent>(this,
							 &nodeComponent::
							 handleEvent) );
		
		if( tmp ){
		  ParentFaultLinks.push_back( tmp );
		}
	}
	
	for( int counter = 0; counter == 0 || tmp != NULL; counter ++ ){
	  sprintf( port, "Child%d", counter );
	  
	  tmp = configureLink( port,
		     new Event::Handler<nodeComponent>(this,
							 &nodeComponent::
							 handleEvent) );

		
		if( tmp ){
		  ChildFaultLinks.push_back( tmp );
		}
	}
							 
							 
  SelfLink->setDefaultTimeBase(registerTimeBase("1 s"));

  assert( (((int) ChildFaultLinks.size()) > 0) || Scheduler);
  assert(SelfLink);
  
  
  
  
    // for reading in the Fault names and lambdas from the CSV in the XML
  boost::tokenizer< boost::escaped_list_separator<char> > Tokenizer( params[ "faults" ] );
  vector<string> tokens;
  tokens.assign( Tokenizer.begin(), Tokenizer.end() );

  for( unsigned int counter = 0; counter < tokens.size(); counter += 2 ){
    boost::algorithm::trim( tokens.at( counter ) );
    boost::algorithm::trim( tokens.at( counter + 1 ) );
    Faults.insert( std::pair<std::string, float>( tokens.at( counter ), atof( tokens.at( counter + 1 ).c_str() ) ) );
    
    sendNextFault( tokens.at( counter ) );
  }
  
  
  
  
  Tokenizer = boost::tokenizer< boost::escaped_list_separator<char> >( params[ "symptomLogProb" ] );
  tokens.assign( Tokenizer.begin(), Tokenizer.end() );
 
  for( unsigned int counter = 0; counter < tokens.size(); counter += 2 ){
    boost::algorithm::trim( tokens.at( counter ) );
    boost::algorithm::trim( tokens.at( counter + 1 ) );
    symptomLogProbability.insert( std::pair<std::string, float>( tokens.at( counter ), atof( tokens.at( counter + 1 ).c_str() ) ) );
  }


  Tokenizer = boost::tokenizer< boost::escaped_list_separator<char> >( params[ "jobKillProb" ] );
  tokens.assign( Tokenizer.begin(), Tokenizer.end() );

  for( unsigned int counter = 0; counter < tokens.size(); counter += 2 ){
    boost::algorithm::trim( tokens.at( counter ) );
    boost::algorithm::trim( tokens.at( counter + 1 ) );
    jobKillProbability.insert( std::pair<std::string, float>( tokens.at( counter ), atof( tokens.at( counter + 1 ).c_str() ) ) );
  }

  //set our clock
  setDefaultTimeBase(registerTimeBase("1 s"));
}


nodeComponent::nodeComponent() :
    Component(-1)
{
    // for serialization only
}


void nodeComponent::addLink( SST::Link * link, enum linkTypes type ){
//  asm( "int $3" );
  if( type == PARENT ){
    ParentFaultLinks.push_back( link );
  }else if( type == CHILD ){
    ChildFaultLinks.push_back( link );
  }
}


void nodeComponent::rmLink( SST::Link * link, enum linkTypes type ){
  internal_error( "Can't remove links just yet\n" );
}


void nodeComponent::disconnectYourself(){
  ChildFaultLinks.empty();
  ParentFaultLinks.empty();
}


std::string nodeComponent::getType(){
  return nodeType;
}

std::string nodeComponent::getID(){
  return ID;
}


void nodeComponent::handleJobKillEvent( JobKillEvent * killEvent ){
  if( killEvent->jobNum == this->jobNum && Scheduler ){
    CompletionEvent *ec = new CompletionEvent(jobNum);
    Scheduler->Send(ec);
  
    killedJobs.insert( std::pair<int, int>( jobNum, jobNum ) );
    jobNum = -1;
  }else{
    internal_error( "Error, asked to kill a job we're not running.\n" );
  }
}



void nodeComponent::handleFaultEvent( SST::Event * ev ){
  if( dynamic_cast<JobFaultEvent*>(ev) ){
    JobFaultEvent * faultEvent = dynamic_cast<JobFaultEvent*>(ev);

    logError( faultEvent );
    
    if( Scheduler && jobNum != -1 ){
      // Kill the job if there is a probability associated with it and a roll of the dice says to do so.
      // Also kill the job if there is no probability associated with it.
      if( jobKillProbability.find( faultEvent->faultType ) == jobKillProbability.end() ||
          drand48() < jobKillProbability.find( faultEvent->faultType )->second ){
        
        //SelfLink->Send( getCurrentSimTime(), new JobKillEvent( this->jobNum ) );

        faultEvent->jobNum = jobNum;
        faultEvent->nodeNumber = nodeNum;
        Scheduler->Send( faultEvent );
          // send the fault on to the scheduler.  It should tell the other nodes to kill the job.

      }
    }else{
      for(std::vector<SST::Link *>::iterator it = ChildFaultLinks.begin(); it != ChildFaultLinks.end(); ++it) {
        (*it)->Send( faultEvent->copy() );
      }
    }
  }


}



// incoming events start a new job
void nodeComponent::handleEvent(Event *ev) {
  
  if( dynamic_cast<CommunicationEvent *>( ev ) ){
    CommunicationEvent * event = dynamic_cast<CommunicationEvent *>( ev );

    event->payload = this->ID;
    event->reply = true;

    Scheduler->Send( event );
    return;
  }else if( dynamic_cast<ObjectRetrievalEvent *>( ev ) ){
    ObjectRetrievalEvent * event = dynamic_cast<ObjectRetrievalEvent *>( ev );

    event->payload = this;

    Builder->Send( event );
    return;
  }


  JobStartEvent *event = dynamic_cast<JobStartEvent*>(ev);
  if (event) {  
    if (jobNum == -1) {
      jobNum = event->jobNum;
      SelfLink->Send(event->time, event);
    } else {
      internal_error("Error?! Already running a job, but given a new one!\n");
    }
  }else if( dynamic_cast<JobFaultEvent*>(ev) ){
    JobFaultEvent * faultEvent = dynamic_cast<JobFaultEvent*>(ev);

    logError( faultEvent );
    
    if( Scheduler && jobNum != -1 ){
      // Kill the job if there is a probability associated with it and a roll of the dice says to do so.
      // Also kill the job if there is no probability associated with it.
      if( jobKillProbability.find( faultEvent->faultType ) == jobKillProbability.end() ||
          drand48() < jobKillProbability.find( faultEvent->faultType )->second ){
        
        //SelfLink->Send( getCurrentSimTime(), new JobKillEvent( this->jobNum ) );

        faultEvent->jobNum = jobNum;
        faultEvent->nodeNumber = nodeNum;
        Scheduler->Send( faultEvent );
          // send the fault on to the scheduler.  It should tell the other nodes to kill the job.

      }
    }else{
      for(std::vector<SST::Link *>::iterator it = ChildFaultLinks.begin(); it != ChildFaultLinks.end(); ++it) {
        (*it)->Send( faultEvent->copy() );
      }
    }

    // log the symptom for the analysis tool
    logSymptom( faultEvent );
   
  }else if( dynamic_cast<JobKillEvent*>(ev) ){
    handleJobKillEvent( dynamic_cast<JobKillEvent*>(ev) );
  }else{
    char errorMessage[ 1024 ];
    snprintf( errorMessage, 1023, "Error! Bad Event Type %s in %s in %s:%d\n", typeid( *ev ).name(), __func__, __FILE__, __LINE__ );
    internal_error( errorMessage );
  }
}


// finish current job
void nodeComponent::handleSelfEvent(Event *ev) {
  JobStartEvent *event = dynamic_cast<JobStartEvent*>(ev);
  if (event) {
    if( killedJobs.erase( event->jobNum ) == 1 ){
    }else if (event->jobNum == jobNum) {
      CompletionEvent *ec = new CompletionEvent(jobNum);
      Scheduler->Send(ec);
      jobNum = -1;
    } else {
      internal_error("Error!! We are not running this job we're supposed to finish!\n");
    }
  }else if( dynamic_cast<JobFaultEvent*>(ev) ){
    JobFaultEvent * faultEvent = dynamic_cast<JobFaultEvent*>(ev);
    
    sendNextFault( faultEvent->faultType );     // handled this fault, send another fault to the future
    
    for(std::vector<SST::Link *>::iterator it = ChildFaultLinks.begin(); it != ChildFaultLinks.end(); ++it) {
      (*it)->Send( faultEvent->copy() );
    }
  
    if( Scheduler && jobNum != 1 ){
      if( jobKillProbability.find( faultEvent->faultType ) == jobKillProbability.end() ||
          drand48() < jobKillProbability.find( faultEvent->faultType )->second ){
  
        //SelfLink->Send( getCurrentSimTime(), new JobKillEvent( this->jobNum ) );
        faultEvent->jobNum = jobNum;
        faultEvent->nodeNumber = nodeNum;
        Scheduler->Send( faultEvent );
          // send the fault on to the scheduler.  It should tell the other nodes to kill the job.
      } 
    }
    
    // log the symptom for the analysis tool
    logSymptom( faultEvent );
    
    
    // last, write the fault to the Fault Log
    if( this->faultLogFileName.length() > 0 ){
      logError( faultEvent );
    }
    

  }else if( dynamic_cast<JobKillEvent*>(ev) ){
    handleJobKillEvent( dynamic_cast<JobKillEvent*>(ev) );
  }else{
    char errorMessage[ 1024 ];
    snprintf( errorMessage, 1023, "Error! Bad Event Type %s in %s in %s:%d\n", typeid( *ev ).name(), __func__, __FILE__, __LINE__ );
    internal_error( errorMessage );
  }
}


double urand()
{
	return(rand() + 1.0)/(RAND_MAX + 1.0);
}

unsigned genexp(double lambda)
{
	double u,x;
	u=urand();
	x=(-1/lambda)*log(u);
	return(x);
}


void nodeComponent::sendNextFault( std::string faultType ){
  if( Faults.find( faultType.c_str() ) == Faults.end() ){
    internal_error( "Error, recieved a fault with a type that is unknown.\n" );
  }
  
  unsigned fail_time = genexp( Faults.find( faultType.c_str() )->second / 31556926.0 );     // lambda scaled to one year
  SelfLink->Send( fail_time / 100, new JobFaultEvent( faultType ) );
  
  /*
      No idea what the magic number means, and it wasn't documented.  It comes from
      
      #define PROPDEPTH 100
      
      in resil.h
  */
}



/*
Uses the probability in symptomLogProbability to determine if a fault should be logged.
If it should be logged, it is logged.
*/
void nodeComponent::logSymptom( JobFaultEvent * faultEvent ){
  std::ofstream symptomLog;
  symptomLog.open( this->symptomLogFileName.c_str(), ios::out | ios::ate | ios::app );

    /* Log the symptom if
     *
     * a) we could open the log
     * b) EITHER there is a symptom logging probability associated with this fault and a random number [0, 1) is less than the probability OR
     * c) there is no probability associated with this type of fault
     */
  if( symptomLog.is_open() ){
    if( (this->symptomLogProbability.find( faultEvent->faultType ) != this->symptomLogProbability.end() &&
          drand48() < this->symptomLogProbability.find( faultEvent->faultType )->second) ||
         this->symptomLogProbability.find( faultEvent->faultType ) == this->symptomLogProbability.end()){
      symptomLog << getCurrentSimTime() << "\t" << this->nodeNum << "\t" << faultEvent->faultType << endl;
    }
    symptomLog.close();
  }else{
      char errorMessage[ 1024 ];
      
      snprintf( errorMessage, 1023, "Couldn't open %s for writing the symptom log for node %d.", this->symptomLogFileName.c_str(), this->nodeNum );
      
      error( errorMessage );
    }
}


/*
Logs the "ground truth" to the fault log.  This is used to measure the accuracy of the Analysis Tool
*/
void nodeComponent::logError( JobFaultEvent * faultEvent ){
  std::ofstream faultLog;
  faultLog.open( this->faultLogFileName.c_str(), ios::out | ios::ate | ios::app );
  if( faultLog.is_open() ){
    faultLog << getCurrentSimTime() << "\t" << this->nodeNum << "\t" << faultEvent->faultType << endl;
    faultLog.close();
  }else{
    char errorMessage[ 1024 ];
    
    snprintf( errorMessage, 1023, "Couldn't open %s for writing the error log for node %d.", this->faultLogFileName.c_str(), this->nodeNum );
    
    error( errorMessage );
  }
}



// Element Libarary / Serialization stuff
    
BOOST_CLASS_EXPORT(JobStartEvent)
BOOST_CLASS_EXPORT(nodeComponent)

