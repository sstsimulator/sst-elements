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
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

#include "sst/core/element.h"

#include "nodeComponent.h"
#include "JobFaultEvent.h"
#include "JobKillEvent.h"

#include "misc.h"

using namespace SST;

nodeComponent::nodeComponent(ComponentId_t id, Params_t& params) :
  Component(id), jobNum(-1) {

  if ( params.find("nodeNum") == params.end() ) {
    _abort(event_test,"couldn't find name for this node\n");
  }
  nodeNum = strtol( params[ "nodeNum" ].c_str(), NULL, 0 );
  lambda = strtof( params[ "lambda" ].c_str(), NULL );

  Scheduler = configureLink( "Scheduler", 
		     new Event::Handler<nodeComponent>(this,
							 &nodeComponent::
							 handleEvent) );
  SelfLink = configureSelfLink("linkToSelf", 
			       new Event::Handler<nodeComponent>(this, 
								 &nodeComponent::
								 handleSelfEvent));
	
	SST::Link * tmp;
  char port[ 16 ];
	
	for( int counter = 0; counter == 0 || tmp != NULL; counter ++ ){
	  sprintf( port, "Parent%d", counter );
	  
	  //cout << "looking for " << port << endl;
	  
	  tmp = configureLink( port,
		     new Event::Handler<nodeComponent>(this,
							 &nodeComponent::
							 handleEvent) );
		
		if( tmp ){
		  Parents.push_back( tmp );
		}
	}
	
	for( int counter = 0; counter == 0 || tmp != NULL; counter ++ ){
	  sprintf( port, "Child%d", counter );
	  
	  //cout << "looking for " << port << endl;
	  
	  tmp = configureLink( port,
		     new Event::Handler<nodeComponent>(this,
							 &nodeComponent::
							 handleEvent) );

		
		//cout << tmp << endl;
		
		if( tmp ){
		  Children.push_back( tmp );
		}
	}
							 
							 
  SelfLink->setDefaultTimeBase(registerTimeBase("1 s"));

  assert( (((int) Children.size()) > 0) || Scheduler);
  assert(SelfLink);
  
  
  
  
  boost::tokenizer< boost::escaped_list_separator<char> > Tokenizer( params[ "faults" ] );
  vector<string> tokens;
  tokens.assign( Tokenizer.begin(), Tokenizer.end() );

  for( unsigned int counter = 0; counter < tokens.size(); counter += 2 ){
    boost::algorithm::trim( tokens.at( counter ) );
    boost::algorithm::trim( tokens.at( counter + 1 ) );
    //cout << "read fault: " << tokens.at( 0 ) << endl;
    //cout << "read fault: " << tokens.at( counter ) << endl;
    Faults.insert( std::pair<std::string, float>( tokens.at( counter ), atof( tokens.at( counter + 1 ).c_str() ) ) );
    
    sendNextFault( tokens.at( counter ) );
  }
  
  
  
  
  //cout << "Built node " << nodeNum << " with " << (int) Children.size() << " children and " << (int) Parents.size() << " parents.  Its lambda is " << lambda << "." << endl;
  

  //set our clock
  setDefaultTimeBase(registerTimeBase("1 s"));
}

nodeComponent::nodeComponent() :
    Component(-1)
{
    // for serialization only
}

// incoming events start a new job
void nodeComponent::handleEvent(Event *ev) {
  JobStartEvent *event = dynamic_cast<JobStartEvent*>(ev);
  if (event) {
    //cout << "Node " << nodeNum << " is handling event " << dynamic_cast<JobStartEvent*>(ev)->jobNum << " at time " << getCurrentSimTime() << "." << endl;
    if (jobNum == -1) {
      jobNum = event->jobNum;
      SelfLink->Send(event->time, event);
    } else {
      internal_error("Error?! Already running a job, but given a new one!\n");
    }
  }else if( dynamic_cast<JobFaultEvent*>(ev) ){
    //cout << "Node " << nodeNum << " got a fault event from a parent: " << dynamic_cast<JobFaultEvent*>(ev)->faultType << " at time " << getCurrentSimTime() << "." << endl;
    
    if( Scheduler ){
      dynamic_cast<JobFaultEvent*>(ev)->jobNum= jobNum;
      //printf("nodejobnum: %d\n", jobNum);
      dynamic_cast<JobFaultEvent*>(ev)->nodeNumber = nodeNum;
      Scheduler->Send( ev );
      

        // TODO: throw ev to the scheduler when the scheduler is ready for failure events
      
    }else{
      for(std::vector<SST::Link *>::iterator it = Children.begin(); it != Children.end(); ++it) {
        (*it)->Send( dynamic_cast<JobFaultEvent*>(ev)->copy() );
      }
    }
    
  }else if( dynamic_cast<JobKillEvent*>(ev) ){
    if( dynamic_cast<JobKillEvent*>(ev)->jobNum== this->jobNum ){
      //printf("node killing %d\n", this->jobNum);
      CompletionEvent *ec = new CompletionEvent(jobNum);
      Scheduler->Send(ec);
      jobNum = -1;
    }else{
      //printf("node %d job %d\n", nodeNum, jobNum);
      //internal_error( "Error, asked to kill a job we're not running.\n" );
    }
  }else{
    internal_error("Error! Bad Event Type!\n");
  }
}

// finish current job
void nodeComponent::handleSelfEvent(Event *ev) {
  JobStartEvent *event = dynamic_cast<JobStartEvent*>(ev);
  if (event) {
    if (event->jobNum == jobNum) {
      CompletionEvent *ec = new CompletionEvent(jobNum);
      Scheduler->Send(ec);
      jobNum = -1;
    } else {
      internal_error("Error!! We are not running this job we're supposed to finish!\n");
    }
  }else if( dynamic_cast<JobFaultEvent*>(ev) ){
    //cout << "Node " << nodeNum << " got a fault event: " << dynamic_cast<JobFaultEvent*>(ev)->faultType << " at time " << getCurrentSimTime() << "." << endl;
    
    sendNextFault( dynamic_cast<JobFaultEvent*>(ev)->faultType );
    
    
    for(std::vector<SST::Link *>::iterator it = Children.begin(); it != Children.end(); ++it) {
      //cout << "sending fault on to " << *it << endl;
      (*it)->Send( dynamic_cast<JobFaultEvent*>(ev)->copy() );
    }

  } else {
    internal_error("Error! Bad Event Type!\n");
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
  
  //cout << "Sending a fault into the future, to " << getCurrentSimTime() + (fail_time / 100) << "." << endl;
  
  /*
      No idea what the magic number means, and it wasn't documented.  It comes from
      
      #define PROPDEPTH 100
      
      in resil.h
  */
}



// Element Libarary / Serialization stuff
    
BOOST_CLASS_EXPORT(JobStartEvent)
BOOST_CLASS_EXPORT(nodeComponent)

