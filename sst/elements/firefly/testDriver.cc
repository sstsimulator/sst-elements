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

#include "sst/core/element.h"
#include "sst/core/component.h"
#include "sst/core/module.h"
#include "sst/core/timeLord.h"

#include "testDriver.h"

using namespace Hermes;
using namespace SST;
using namespace SST::Firefly;

TestDriver::TestDriver(ComponentId_t id, Params_t &params) :
    Component( id ),
    m_functor( DerivedFunctor( this, &TestDriver::funcDone ) )
{
    // this has to come first 
    registerTimeBase( "100 ns", true);

    std::string name = params.find_string("hermesModule");
    if ( name == "" ) {
        _abort(TestDriver, "ERROR:  What Hermes module? '%s'\n", name.c_str());
    } 
   
    m_dbg.init("@t:TestDriver::@p():@l " + getName() + ": ", 
            params.find_integer("verboseLevel",0),0,
            (Output::output_location_t)params.find_integer("debug", 0));
    m_dbg.output(CALL_INFO,"loading module `%s`\n",name.c_str());

    std::ostringstream ownerName;
    ownerName << this;
    Params_t hermesParams = params.find_prefix_params("hermesParams." ); 
    hermesParams.insert( 
        std::pair<std::string,std::string>("owner", ownerName.str()));

    m_hermes = dynamic_cast<MessageInterface*>(loadModule(name,hermesParams));
    if ( !m_hermes ) {
        _abort(TestDriver, "ERROR:  Unable to find Hermes '%s'\n",
                                        name.c_str());
    }

    m_selfLink = configureSelfLink("Self", "100 ns",
        new Event::Handler<TestDriver>(this,&TestDriver::handle_event));

    m_traceFileName = params.find_string("traceFile");

    int bufLen = params.find_integer( "bufLen" );
    assert( bufLen != -1 ); 

    m_dbg.output(CALL_INFO,"bufLen=%d\n",bufLen);
    m_recvBuf.resize(bufLen);
    m_sendBuf.resize(bufLen);
    
    m_root = 2;
    for ( unsigned int i = 0; i < m_sendBuf.size(); i++ ) {
        m_sendBuf[i] = i;
    } 
}
    
TestDriver::~TestDriver()
{
}

void TestDriver::init( unsigned int phase )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    m_hermes->_componentInit( phase );
}

void TestDriver::setup() 
{ 
    m_selfLink->send(1,NULL);
    m_hermes->_componentSetup( );

    std::ostringstream tmp;

    tmp << m_traceFileName.c_str() << m_hermes->myWorldRank();

    m_traceFile.open( tmp.str().c_str() );
    m_dbg.verbose(CALL_INFO,1,0,"traceFile `%s`\n",tmp.str().c_str());
    if ( ! m_traceFile.is_open() ) {
        _abort(TestDriver, "ERROR:  Unable to open trace file '%s'\n",
                                        tmp.str().c_str() );
    }

    char buffer[100];
    snprintf(buffer,100,"@t:%d:TestDriver::@p():@l ",m_hermes->myWorldRank()); 
    m_dbg.setPrefix(buffer);

    m_collectiveOut = 0x12345678;
    m_collectiveIn =  m_hermes->myWorldRank() << ( m_hermes->myWorldRank() *8); 
}

void TestDriver::handle_event( Event* ev )
{
    getline( m_traceFile, m_funcName );

    m_dbg.verbose(CALL_INFO,1,0, "function `%s`\n" , m_funcName.c_str());

    m_funcName = m_funcName.c_str();
    if ( ! m_funcName.empty() ) {
        printf("%d: %s\n",my_rank, m_funcName.c_str());
    }
    if ( m_funcName.compare( "init" ) == 0 ) {
        m_hermes->init( &m_functor );
    } else if ( m_funcName.compare( "size" ) == 0 ) {
        m_hermes->size( GroupWorld, &my_size, &m_functor );
    } else if ( m_funcName.compare( "rank" ) == 0 ) {
        m_hermes->rank( GroupWorld, &my_rank, &m_functor );
    } else if ( m_funcName.compare( "recv" ) == 0 ) {

        m_dbg.verbose(CALL_INFO,1,0,"my_size=%d my_rank=%d\n",my_size, my_rank);
        m_irecv = false;
        m_hermes->recv( &m_recvBuf[0], m_recvBuf.size(), CHAR, 
                                    (my_rank + 1) % 2, 
                                    AnyTag, 
                                    GroupWorld, 
                                    &my_resp, 
                                    &m_functor);

    } else if ( m_funcName.compare( "irecv" ) == 0 ) {

        m_dbg.verbose(CALL_INFO,1,0,"my_size=%d my_rank=%d\n",my_size, my_rank);
        m_irecv = true;
        m_hermes->irecv( &m_recvBuf[0], m_recvBuf.size(), CHAR, 
                                    (my_rank + 1) % 2, 
                                    AnyTag, 
                                    GroupWorld, 
                                    &my_req, 
                                    &m_functor);

    } else if ( m_funcName.compare( "send" ) == 0 ) {

        m_dbg.verbose(CALL_INFO,1,0,"my_size=%d my_rank=%d\n",my_size, my_rank);
        m_hermes->send( &m_sendBuf[0], m_sendBuf.size(), CHAR,
                                (my_rank + 1 ) % 2,
                                0xdead, 
                                GroupWorld, 
                                &m_functor);

    } else if ( m_funcName.compare( "barrier" ) == 0 ) {
        m_dbg.verbose(CALL_INFO,1,0,"my_size=%d my_rank=%d\n",my_size, my_rank);
        m_hermes->barrier( GroupWorld, &m_functor );
    } else if ( m_funcName.compare( "reduce" ) == 0 ) {
        m_dbg.verbose(CALL_INFO,1,0,"my_size=%d my_rank=%d\n",my_size, my_rank);
        m_hermes->reduce( &m_collectiveIn, &m_collectiveOut, 1, INT,
                                MIN, m_root, GroupWorld, &m_functor );
    } else if ( m_funcName.compare( "allreduce" ) == 0 ) {
        m_dbg.verbose(CALL_INFO,1,0,"my_size=%d my_rank=%d\n",my_size, my_rank);
        m_hermes->allreduce( &m_collectiveIn, &m_collectiveOut, 1, INT,
                                MAX, GroupWorld, &m_functor );
    } else if ( m_funcName.compare( "wait" ) == 0 ) {
        m_hermes->wait( &my_req, &my_resp, &m_functor );
    } else if ( m_funcName.compare( "fini" ) == 0 ) {
        printf("%d: collective result %#x\n",my_rank, m_collectiveOut);
        if ( m_irecv ) {
        printf("%d: src=%d tag=%#x len=%lu\n",my_rank, my_req.src,
                                            my_req.tag,m_recvBuf.size());
        } else {
        printf("%d: src=%d tag=%#x len=%lu\n",my_rank, my_resp.src,
                                            my_resp.tag,m_recvBuf.size());
        }

        for ( unsigned int i = 0; i < m_recvBuf.size(); i++ ) {
            if ( m_recvBuf[i] != (i&0xff) ) {
                printf("ERROR %d != %d\n",i,m_recvBuf[i]);
            }
        }
        m_hermes->fini( &m_functor );
    }
}

void TestDriver::funcDone( int retval )
{
    m_dbg.verbose(CALL_INFO,1,0,"`%s` retval=%d\n" , m_funcName.c_str(), retval);
    m_selfLink->send(1,NULL);
}
