#include "sst/core/serialization.h"

#include "fileDrivenFaultsComponent.h"
#include "CommunicationEvent.h"
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
		};
	};
};


fileDrivenFaultsComponent::~fileDrivenFaultsComponent(){}


fileDrivenFaultsComponent::fileDrivenFaultsComponent( ComponentId_t id, Params& params ) : Component(id){
	//set up output
	schedout.init( "", 8, 0, Output::STDOUT );

	//set up the all-important self-link
	selfLink = configureSelfLink( "linkToSelf", new Event::Handler<fileDrivenFaultsComponent>( this, &fileDrivenFaultsComponent::handleSelfLink ) );
	selfLink->setDefaultTimeBase( registerTimeBase( SCHEDULER_TIME_BASE ) );

	schedout.output("Scheduler looking for link...");
	bool retrievedLastNode = false;
	for( int nodeCount = 0; !retrievedLastNode; nodeCount ++ ){
		char nodeName[ 50 ];
		snprintf( nodeName, 50, "nodeLink%d", nodeCount );
		schedout.output( " %s", nodeName );
		SST::Link *nodeLink = configureLink( nodeName, SCHEDULER_TIME_BASE, new Event::Handler<fileDrivenFaultsComponent, int>( this, &fileDrivenFaultsComponent::handleNodeLink, nodeCount ) );
		if( nodeLink ){
			nodeLinks.push_back( nodeLink );
		}else{
			schedout.output( "(no %s)\n", nodeName );
			retrievedLastNode = true;
		}
	}

	if( params.find( "failureFilename" ) != params.end() )
		failFilename = params[ "failureFilename" ];
	else
		schedout.fatal( CALL_INFO, 1, "fileDrivenFaultsComponent: failureFilename must be provided" );

	resumeSimulationToken = "YYRESUME";
	if( params.find( "resumeSimToken" ) != params.end() )
		resumeSimulationToken = params[ "resumeSimToken" ];

	if( params.find( "injectionFrequency" ) != params.end() )
		failFrequency = atoi( params[ "injectionFrequency" ].c_str() );
	else
		schedout.fatal( CALL_INFO, 1, "fileDrivenFaultsComponent: injectionFrequency must be specified" );

	failPollFreq = 1000;
	if( params.find( "filePollFreq" ) != params.end() )
		failPollFreq = atoi( params[ "filePollFreq" ].c_str() );

	fileLastWritten = 0;
}


fileDrivenFaultsComponent::fileDrivenFaultsComponent() : Component( -1 ){}


void fileDrivenFaultsComponent::setup(){
	for( std::vector<SST::Link *>::iterator nodeIter = nodeLinks.begin(); nodeIter != nodeLinks.end(); nodeIter ++ ){
		SST::Event * getID = new CommunicationEvent( RETRIEVE_ID, (void*)(!NULL) );
		(*nodeIter)->send( getID );
	}

	selfLink->send( failFrequency, new sendFailsEvent() );
}


void fileDrivenFaultsComponent::handleNodeLink( SST::Event * event, int node ){
	if( dynamic_cast<CommunicationEvent *>( event ) ){
		CommunicationEvent * commEvent = dynamic_cast<CommunicationEvent *>( event );
		if( commEvent->reply == true && commEvent->CommType == RETRIEVE_ID ){
			nodeIDs.insert( nodeIDs.begin() + node, *(std::string *)commEvent->payload );
		}
		delete event;
	}
}


std::map<std::string, std::string> * fileDrivenFaultsComponent::readFailFile(){
	bool fileContainsToken = false;
	std::map<std::string, std::string> * nodes = new std::map<std::string, std::string>();

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
				schedout.fatal( CALL_INFO, 1, "malformed line in failure file: %s", fileLine.c_str() );
			}

			boost::algorithm::trim( tokens.at( 0 ) );
			boost::algorithm::trim( tokens.at( 1 ) );

			nodes->insert( std::pair<std::string, std::string>( tokens.at( 0 ), tokens.at( 1 ) ) );
		}else{
			break;
		}
	}

	failFile.close();
	


	std::fstream failFileOut( failFilename.c_str(), std::fstream::out | std::fstream::trunc);
	failFileOut.close();

	return nodes;
}


void fileDrivenFaultsComponent::handleSelfLink( SST::Event *event ){
	if( NULL != dynamic_cast<sendFailsEvent *>( event ) ){
		std::map<std::string, std::string> * failNodes = readFailFile();

		for( std::map<std::string, std::string>::iterator nodeIter = failNodes->begin(); nodeIter != failNodes->end(); nodeIter ++ ){
			SST::Event * failEvent = new CommunicationEvent( FAIL_JOBS, new std::string( nodeIter->second ) );
			nodeLinks[ std::distance( nodeIDs.begin(), std::find( nodeIDs.begin(), nodeIDs.end(), nodeIter->first ) ) ]->send( failEvent );
		}

		delete failNodes;
		selfLink->send( failFrequency, new sendFailsEvent() );
	}
}


