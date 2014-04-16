// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdio.h>

#include "sst_config.h"
#include "sst/core/serialization.h"
#include "sst/core/element.h"

#include "linkBuilder.h"

#include "CommunicationEvent.h"
#include "nodeComponent.h"
#include "ObjectRetrievalEvent.h"
#include "misc.h"

using namespace SST::Scheduler;


linkBuilder::linkBuilder(SST::ComponentId_t id, SST::Params & params) : Component( id )
{
    /*
       selfLink = configureSelfLink( "linkToSelf", SCHEDULER_TIME_BASE, new SST::Event::Handler<linkBuilder>( this, &linkBuilder::initNodePtrRequests ) );

       SST::Link * newLink;

       std::cout << "Adding new linkBuilder links:";

       for( int counter = 0; counter == 0 || newLink; counter ++ ){
       char newLinkName[ 16 ];
       snprintf( newLinkName, 15, "nodeLink%d", counter );
       newLink = configureLink( newLinkName, SCHEDULER_TIME_BASE, new SST::Event::Handler<linkBuilder>( this, &linkBuilder::handleNewNodePtr ) );
       if( newLink ){
       nodeLinks.push_back( newLink );
       std::cout << " " << newLinkName;
       }
       }

       std::cout << " done." << std::endl;

       selfLink->send( new ObjectRetrievalEvent() ); 
       selfLink->send( 350000, new CommunicationEvent( RETRIEVE_ID ) );*/ 
}


void linkBuilder::connectGraph(Job* job)
{

}


void linkBuilder::disconnectGraph(Job* job)
{

}


void linkBuilder::initNodePtrRequests(SST::Event* event)
{
    /*  ObjectRetrievalEvent * objRetEvent = dynamic_cast<ObjectRetrievalEvent *>( event );
        if( objRetEvent ){
        for( std::vector<SST::Link *>::iterator linkIter = nodeLinks.begin(); linkIter < nodeLinks.end(); linkIter ++ ){
        (*linkIter)->send( objRetEvent->copy() ); 
        }
        }
        else{
        if( nodes.find( "node" ) != nodes.end() &&
        nodes.find( "node" )->second.find( "1.1" ) != nodes.find( "node" )->second.end() &&
        nodes.find( "node" )->second.find( "2.1" ) != nodes.find( "node" )->second.end() &&
        dynamic_cast<nodeComponent *>( nodes.find( "node" )->second.find( "1.1" )->second ) &&
        dynamic_cast<nodeComponent *>( nodes.find( "node" )->second.find( "2.1" )->second ) ){*/
    //SST::Link * newLink1 = configureLink( "test", new SST::Event::Handler<nodeComponent>( dynamic_cast<nodeComponent *>( nodes.find( "node" )->second.find( "1.1" )->second ), &nodeComponent::handleEvent ) );
    //SST::Link * newLink2 = configureLink( "test", new SST::Event::Handler<nodeComponent>( dynamic_cast<nodeComponent *>( nodes.find( "node" )->second.find( "2.1" )->second ), &nodeComponent::handleEvent ) );

    //      std::cout << typeid( dynamic_cast<nodeComponent *>( nodes.find( "node" )->second.find( "2.1" )->second )->SelfLink ).name() << std::endl;

    //      dynamic_cast<nodeComponent *>( nodes.find( "node" )->second.find( "1.1" )->second )->addLink( dynamic_cast<nodeComponent *>( nodes.find( "node" )->second.find( "2.1" )->second )->FaultLink, PARENT );
    //      dynamic_cast<nodeComponent *>( nodes.find( "node" )->second.find( "2.1" )->second )->addLink( dynamic_cast<nodeComponent *>( nodes.find( "node" )->second.find( "1.1" )->second )->FaultLink, CHILD );

    //      std::cout << "linkBuilder was able to send the new link" << std::endl;
    /*    }
          }*/
}


void linkBuilder::handleNewNodePtr(SST::Event* event)
{
    /*  if( dynamic_cast<ObjectRetrievalEvent *>( event ) ){
        ObjectRetrievalEvent * objRetEvent = dynamic_cast<ObjectRetrievalEvent *>( event );

        if( dynamic_cast<linkChanger *>( objRetEvent->payload ) ){
        linkChanger * graphNode = dynamic_cast<linkChanger *>( objRetEvent->payload );

        if( graphNode->getID().length() < 1 ){
        std::cerr << "The linkBuilder recieved an object with no ID.  This probably isn't what you wanted to do." << std::endl;
        }

        if( nodes.find( graphNode->getType() ) == nodes.end() ){
        nodes.insert( std::pair<std::string, nodeMap>( graphNode->getType(), nodeMap() ) );
        }
        nodes.find( graphNode->getType() )->second.insert( std::pair<std::string, linkChanger *>( graphNode->getID(), graphNode ) );
        }
        }
        delete event;*/
}

