// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "hades.h"

#include <sst/core/debug.h>
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/link.h>

#include <stdlib.h>

#include <iostream>
#include <fstream>

#include "ioapi.h"
#include "functionSM.h"
#include "entry.h"
#include "nodeInfo.h"

#include "funcSM/api.h"

using namespace SST::Firefly;
using namespace SST;

Hades::Hades( Component* owner, Params& params ) :
    MessageInterface(),
    m_state( WaitFunc ),
    m_io( NULL )
{
    int verboseLevel = params.find_integer("verboseLevel",0);
    Output::output_location_t loc = 
            (Output::output_location_t)params.find_integer("debug", 0);

    m_dbg.init("@t:Hades::@p():@l ", verboseLevel, 0, loc );

    m_enterLink = owner->configureSelfLink("HadesEnterLink", "1 ps",
        new Event::Handler<Hades>(this,&Hades::enterEventHandler));

    Params ioParams = params.find_prefix_params("ioParams." );

    std::string moduleName = params.find_string("ioModule"); 
    m_io = dynamic_cast<IO::Interface*>(owner->loadModuleWithComponent( 
                        moduleName, owner, ioParams ) );
    if ( !m_io ) {
        m_dbg.fatal(CALL_INFO,0," Unable to find Hermes '%s'\n",
                                        moduleName.c_str());
    }

    m_io->setReturnLink( m_enterLink );

    Params nodeParams = params.find_prefix_params("nodeParams.");
    
    m_nodeInfo = new NodeInfo( nodeParams );

    m_dbg.verbose(CALL_INFO,1,0,"numCores %d, coreNum %d\n",
                        m_nodeInfo->numCores(), m_nodeInfo->coreNum());

    int numRanks = params.find_integer("numRanks");
    if ( numRanks <= 0 ) {
        m_dbg.fatal(CALL_INFO,0,"How many global ranks?\n");
    }
    m_dbg.verbose(CALL_INFO,1,0,"numRanks %d\n", numRanks);

    std::string nidListFileName = params.find_string("nidListFile");

    m_dbg.verbose(CALL_INFO,1,0,"nidListFile `%s`\n",nidListFileName.c_str());

    std::ifstream nidListFile( nidListFileName.c_str());
    if ( ! nidListFile.is_open() ) {
        m_dbg.verbose(CALL_INFO,0,1,"Unable to open nid list '%s'\n",
                                        nidListFileName.c_str() );
    }

    Group* group;

    std::string policy = params.find_string("policy");
    m_dbg.verbose(CALL_INFO,1,0,"load policy `%s`\n",policy.c_str());

    if ( 0 == policy.compare("adjacent") ) {
        group = initAdjacentMap(numRanks, m_nodeInfo->numCores(), nidListFile);
#if 0
    } else if ( 0 == policy.compare("roundRobin") ) {
        group = initRoundRobinMap(numRanks, m_nodeInfo->numCores(), nidListFile); 
#endif
    } else {
        group = NULL; // get rid of compiler warning
        m_dbg.fatal(CALL_INFO,0,"unknown load policy `j%s` ",
                                            policy.c_str() );
    }
    m_info.addGroup( Hermes::GroupWorld, group);

    nidListFile.close();

    m_out = new Out( m_io );

    //****************************
    Params tmpParams;
    int protoNum = 0;
    tmpParams = params.find_prefix_params("longMsgProtocol.");
    m_protocolM[ protoNum ] = 
        dynamic_cast<ProtocolAPI*>(owner->loadModuleWithComponent(
                            "firefly.LongMsgProto", owner, tmpParams ) );

    m_protocolM[ protoNum ]->init( m_out, &m_info, m_enterLink );
    m_protocolMapByName[ m_protocolM[ protoNum ]->name() ] =
                                                m_protocolM[ protoNum ];

    //****************************
    ++protoNum;
    tmpParams = params.find_prefix_params("ctrlMsg.");
    m_protocolM[ protoNum ] = 
        dynamic_cast<ProtocolAPI*>(owner->loadModuleWithComponent(
                            "firefly.CtrlMsg", owner, tmpParams ) );

    m_protocolM[ protoNum ]->init( m_out, &m_info, m_enterLink );
    m_protocolMapByName[ m_protocolM[ protoNum ]->name() ] = 
                                                m_protocolM[ protoNum ];

    m_sendIter = m_protocolM.begin();

    Params funcParams = params.find_prefix_params("functionSM.");

    m_functionSM = new FunctionSM( funcParams, owner, m_info, m_enterLink,
                                    m_protocolMapByName );
    
}

void Hades::printStatus( Output& out )
{
    std::map<int,ProtocolAPI*>::iterator iter= m_protocolM.begin();
    for ( ; iter != m_protocolM.end(); ++iter ) {
        iter->second->printStatus(out);
    }
    m_functionSM->printStatus( out );
}

void Hades::_componentSetup()
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    Group* group = m_info.getGroup(Hermes::GroupWorld);
    for ( unsigned int i = 0; i < group->size(); i++ ) {
        m_dbg.verbose(CALL_INFO,2,0,"group rank %d, %d %d\n",
                                    i,group->getNodeId( i ),myNodeId() );
        if ( group->getNodeId( i ) == myNodeId() && 
                group->getCoreId( i ) == m_nodeInfo->coreNum() ) 
        {
            m_dbg.verbose(CALL_INFO,1,0,"set rank to %d\n",i);
            group->setMyRank( i );
            break;
        }
    } 

    char buffer[100];
    snprintf(buffer,100,"@t:%d:%d:Hades::@p():@l ",myNodeId(), myWorldRank());
    m_dbg.setPrefix(buffer);

    m_dbg.verbose(CALL_INFO,1,0, "myRank %d\n",
                m_info.getGroup(Hermes::GroupWorld)->getMyRank() );
    
    std::map<int,ProtocolAPI*>::iterator iter= m_protocolM.begin();
    for ( ; iter != m_protocolM.end(); ++iter ) {
        iter->second->setup();
    }

    m_functionSM->setup();
}

Group* Hades::initAdjacentMap( int numRanks, 
            int numCores, std::ifstream& nidFile )
{
    Group* group = new Group( numRanks );

    m_dbg.verbose(CALL_INFO,2,0,"numRanks=%d numCores=%d\n",numRanks,numCores);

    int nid = -1;
    for ( int node = 0; node < numRanks/numCores; node++ ) {

        if ( ! nidFile.is_open()  ) { 
            ++nid;
        } else {
            std::string line;
            getline( nidFile, line );
            int ret = sscanf( line.c_str(), "%d", &nid ); 
            if( ret != 1 ) {
                _abort(Hades, "ERROR: nidList is not long enough, "
                                        "want %d %d\n", numRanks, ret);
            }
        }

        for ( int core = 0; core < numCores; core++ ) {
            group->set( node * numCores + core, nid, core );
            m_dbg.verbose(CALL_INFO,2,0,"rank %d is on nid %d\n",
                                            node * numCores + core , nid);
        }
    }
    return group;
}

Group* Hades::initRoundRobinMap( int numRanks, 
            int numCores, std::ifstream& nidFile )
{
    Group* group = new Group( numRanks );
    m_dbg.verbose(CALL_INFO,1,0,"numRanks=%d numCores=%d\n",
                            numRanks, numCores);

    return group;
}

void Hades::_componentInit(unsigned int phase )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    m_io->_componentInit(phase);
}

Hermes::RankID Hades::myWorldRank() 
{
    int rank = _myWorldRank();
    m_dbg.verbose(CALL_INFO,1,0,"rank=%d\n",rank);
    if ( -1 == rank ) {
        m_dbg.fatal(CALL_INFO,0,"%s() rank not set yet\n",__func__);
        return -1; 
    } else {
        return rank;
    }
}


void Hades::enterEventHandler( SST::Event* )
{
    m_dbg.verbose(CALL_INFO,1,0,"%s\n",m_state==WaitFunc?"WaitFunc":"WaitIO");
    m_state = WaitFunc;

    if ( runRecv() ) {
       return;
    }

    if ( runSend() ) {
        return;
    }

    m_dbg.verbose(CALL_INFO,1,0,"call m_io->wait()\n");
    m_io->wait( );
    m_state = WaitIO;
}

bool Hades::runSend( )
{
    std::map<int,ProtocolAPI*>::iterator start = currentSendIterator();

do { 

    m_dbg.verbose(CALL_INFO,1,0,"check protocol[%d] %s\n", 
                currentSendIterator()->first,
                currentSendIterator()->second->name().c_str() );

    IO::NodeId dest = currentSendIterator()->second->canSend();
    int protocol = currentSendIterator()->first;

    advanceSendIterator();

    if ( IO::AnyId != dest ) {

        IORequest* req = new IORequest;

        m_dbg.verbose(CALL_INFO,1,0,"SendStream started\n");

        req->protoType = protocol;
        req->nodeId = dest;

        std::vector<IoVec> ioVec; 
        ioVec.resize(1);
        ioVec[0].ptr = &req->protoType;
        ioVec[0].len = sizeof(req->protoType); 

        // we will get req back as the argument to sendWireHdrDone 
        req->callback = new IO_Functor( this, &Hades::sendWireHdrDone, req );

        m_io->sendv( req->nodeId, ioVec, req->callback );
        return true;
    }
} while ( start != currentSendIterator() ) ;

    return false;
}

IO::Entry* Hades::sendWireHdrDone( IO::Entry* e )
{
    IORequest* req = static_cast<IORequest*>(e);

    m_dbg.verbose(CALL_INFO,1,0,"%s\n",
                m_protocolM[req->protoType]->name().c_str());

    m_protocolM[ req->protoType ]->startSendStream( req->nodeId ); 

    delete e;
    return NULL;
}

bool Hades::runRecv()
{
    m_dbg.verbose(CALL_INFO,1,0,"\n" );
    IO::NodeId src = m_io->peek( ); 
    if ( src == IO::AnyId ) {
        return false;
    }

    m_dbg.verbose(CALL_INFO,1,0,"data coming from src %d\n", src );
    
    IORequest* req = new IORequest; 
    req->nodeId = src;
    req->protoType = -1;
    req->callback  = new IO_Functor(this, &Hades::recvWireHdrDone, req);

    std::vector<IoVec> vec; 

    vec.resize(1);
    vec[0].ptr = &req->protoType;
    vec[0].len = sizeof(req->protoType);

    m_io->recvv( req->nodeId, vec, req->callback );
    return true;
}

IO::Entry* Hades::recvWireHdrDone( IO::Entry* e )
{
    IORequest* req = static_cast<IORequest*>(e);

    m_dbg.verbose(CALL_INFO,1,0,"%s\n",
                m_protocolM[req->protoType]->name().c_str());

    m_protocolM[ req->protoType ]->startRecvStream( req->nodeId );
    
    delete e;
    return NULL;
}
