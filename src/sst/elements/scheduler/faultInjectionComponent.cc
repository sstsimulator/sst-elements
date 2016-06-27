// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include "faultInjectionComponent.h"
#include "events/CommunicationEvent.h"
#include "output.h"
#include "misc.h"

#include <fstream>
#include <iostream>
#include <stdlib.h>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>

#include <sst/core/params.h>

using namespace SST;
using namespace SST::Scheduler;

namespace SST{
	namespace Scheduler{
		class sendFailsEvent : public SST::Event{
		    NotSerializable(sendFailsEvent)
		};
	};
};


faultInjectionComponent::~faultInjectionComponent(){}


faultInjectionComponent::faultInjectionComponent( ComponentId_t id, Params& params ) : Component(id){
	//set up output
	schedout.init( "", 8, 0, Output::STDOUT );

	//set up the all-important self-link
	selfLink = configureSelfLink( "linkToSelf", new Event::Handler<faultInjectionComponent>( this, &faultInjectionComponent::handleSelfLink ) );
	selfLink->setDefaultTimeBase( registerTimeBase( SCHEDULER_TIME_BASE ) );

	schedout.output("Scheduler looking for link...");
	bool retrievedLastNode = false;
	for( int nodeCount = 0; !retrievedLastNode; nodeCount ++ ){
		char nodeName[ 50 ];
		snprintf( nodeName, 50, "nodeLink%d", nodeCount );
		schedout.output( " %s", nodeName );
		SST::Link *nodeLink = configureLink( nodeName, SCHEDULER_TIME_BASE, new Event::Handler<faultInjectionComponent, int>( this, &faultInjectionComponent::handleNodeLink, nodeCount ) );
		if( nodeLink ){
			nodeLinks.push_back( nodeLink );
		}else{
			schedout.output( "(no %s)\n", nodeName );
			retrievedLastNode = true;
		}
	}

	if( !params.find<std::string>( "faultInjectionFilename" ).empty() )
		failFilename = params.find<std::string>("faultInjectionFilename");
	else
		schedout.fatal( CALL_INFO, 1, "faultInjectionComponent: faultInjectionFilename must be provided\n" );

	resumeSimulationToken = "YYRESUME";
	if( !params.find<std::string>( "resumeSimToken" ).empty() )
		resumeSimulationToken = params.find<std::string>("resumeSimToken");

	if( !params.find<std::string>( "injectionFrequency" ).empty() )
		failFrequency = atoi( params.find<std::string>("injectionFrequency").c_str() );
	else
		schedout.fatal( CALL_INFO, 1, "faultInjectionComponent: injectionFrequency must be specified\n" );

	failPollFreq = 1000;
	if( !params.find<std::string>( "filePollFreq" ).empty() )
		failPollFreq = atoi( params.find<std::string>("filePollFreq").c_str() );

	fileLastWritten = 0;
}


faultInjectionComponent::faultInjectionComponent() : Component( -1 ){}


void faultInjectionComponent::setup(){
	for( std::vector<SST::Link *>::iterator nodeIter = nodeLinks.begin(); nodeIter != nodeLinks.end(); nodeIter ++ ){
		SST::Event * getID = new CommunicationEvent( RETRIEVE_ID);
		(*nodeIter)->send( getID );
	}

	selfLink->send( failFrequency, new sendFailsEvent() );
}


void faultInjectionComponent::handleNodeLink( SST::Event * event, int node ){
	if( dynamic_cast<CommunicationEvent *>( event ) ){
		CommunicationEvent * commEvent = dynamic_cast<CommunicationEvent *>( event );
		if( commEvent->reply == true && commEvent->CommType == RETRIEVE_ID ){
			nodeIDs.insert( nodeIDs.begin() + node, *(std::string *)commEvent->payload );
		}
		delete event;
	}
}


std::map<std::string, std::string> * faultInjectionComponent::readFailFile(){
	bool fileContainsToken = false;
	std::map<std::string, std::string> * nodes = new std::map<std::string, std::string>();

	// empty the file to say we're ready for input
	std::fstream failFileOut( failFilename.c_str(), std::fstream::out | std::fstream::trunc);
	failFileOut.close();


	// block until the token exists in the file.  It should be the last line.
	while( !fileContainsToken ){
		if( boost::filesystem::exists( failFilename ) && fileLastWritten < boost::filesystem::last_write_time( failFilename ) ){
			fileContainsToken = false;
			std::ifstream failFile;
			std::string fileLine;

			failFile.open( failFilename.c_str() );
			
			while( std::getline( failFile, fileLine ) ){
				fileContainsToken |= (fileLine.find( resumeSimulationToken ) != std::string::npos);
			}

			failFile.close();

			if( fileContainsToken ){
				fileLastWritten = boost::filesystem::last_write_time( failFilename );
			}else{
				boost::this_thread::sleep( boost::posix_time::milliseconds( failPollFreq ) );
			}
		}
	}

	std::ifstream failFile;
	std::string fileLine;

	failFile.open( failFilename.c_str() );
	
	while( std::getline( failFile, fileLine ) ){
		if( fileLine.find( resumeSimulationToken ) == std::string::npos ){
			boost::tokenizer<boost::escaped_list_separator<char> > Tokenizer( fileLine );
			std::vector<std::string> tokens;
			tokens.assign( Tokenizer.begin(), Tokenizer.end() );

			if( 2 != tokens.size() ){
				schedout.fatal( CALL_INFO, 1, "malformed line in failure file: %s\n", fileLine.c_str() );
			}

			boost::algorithm::trim( tokens.at( 0 ) );
			boost::algorithm::trim( tokens.at( 1 ) );

			nodes->insert( std::pair<std::string, std::string>( tokens.at( 0 ), tokens.at( 1 ) ) );
		}else{
			break;
		}
	}

	failFile.close();

	return nodes;
}


void faultInjectionComponent::handleSelfLink( SST::Event *event ){
	if( NULL != dynamic_cast<sendFailsEvent *>( event ) ){
		std::map<std::string, std::string> * failNodes = readFailFile();

		for( std::map<std::string, std::string>::iterator nodeIter = failNodes->begin(); nodeIter != failNodes->end(); nodeIter ++ ){
			if( std::find( nodeIDs.begin(), nodeIDs.end(), nodeIter->first ) != nodeIDs.end() ){
				SST::Event * failEvent = new CommunicationEvent( FAIL_JOBS, new std::string( nodeIter->second ) );
				nodeLinks[ std::distance( nodeIDs.begin(), std::find( nodeIDs.begin(), nodeIDs.end(), nodeIter->first ) ) ]->send( failEvent );
			}else{
				schedout.output( "failInjectionComponent: connection to node %s does not exist, ignoring\n", nodeIter->first.c_str() );
			}
		}

		delete failNodes;
		selfLink->send( failFrequency, new sendFailsEvent() );
	}
}


