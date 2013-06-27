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

#include <cxxabi.h>

#define DBGX( fmt, args... ) \
{\
    char* realname = abi::__cxa_demangle(typeid(*this).name(),0,0,NULL);\
    fprintf( stderr, "%s::%s():%d: "fmt, realname ? realname : "?????????", \
                        __func__, __LINE__, ##args);\
    if ( realname ) free(realname);\
}

using namespace Hermes;
using namespace SST;
using namespace SST::Firefly;

TestDriver::TestDriver(ComponentId_t id, Params_t &params) :
    Component( id ),
    m_functor( DerivedFunctor( this, &TestDriver::funcDone ) )
{
    // this has to come first 
    registerTimeBase( "100 ns", true);

    int rank = params.find_integer("rank");
    if ( rank < 0 ) {
        _abort(TestDriver, "ERROR:  What Hermes rank?\n");
    } 
    DBGX("I'm rank %d\n",rank);

    std::string name = params.find_string("hermesModule");
    if ( name == "" ) {
        _abort(TestDriver, "ERROR:  What Hermes module? '%s'\n", name.c_str());
    } 
    DBGX("loading module `%s`\n",name.c_str());

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
    
    std::ostringstream traceFile;
    traceFile << params.find_string("traceFile") << rank;

    DBGX("traceFile `%s`\n",traceFile.str().c_str());

    m_traceFile.open( traceFile.str().c_str() );
    if ( ! m_traceFile.is_open() ) {
        _abort(TestDriver, "ERROR:  Unable to open trace file '%s'\n",
                                        traceFile.str().c_str() );
    }

    m_selfLink = configureSelfLink("Self", "100 ns",
        new Event::Handler<TestDriver>(this,&TestDriver::handle_event));
}
    
TestDriver::~TestDriver()
{
}

void TestDriver::init( unsigned int phase )
{
    m_hermes->_componentInit( phase );
}

void TestDriver::setup() 
{ 
    m_selfLink->send(1,NULL);
}

void TestDriver::handle_event( Event* ev )
{
    std::string line;
    getline( m_traceFile, line );

    DBGX("`%s`\n",line.c_str());
    if ( line.compare( "init" ) == 0 ) {
        m_hermes->init( &m_functor );
    } else if ( line.compare( "size" ) == 0 ) {
        m_hermes->size( GroupWorld, &my_size, &m_functor );
    } else if ( line.compare( "rank" ) == 0 ) {
        m_hermes->rank( GroupWorld, &my_rank, &m_functor );
    } else if ( line.compare( "irecv" ) == 0 ) {

        printf("my_size=%d my_rank=%d\n",my_size, my_rank);
        m_hermes->irecv( NULL, 0, CHAR, 
                                    (my_rank + 1) % 2, 
                                    AnyTag, 
                                    GroupWorld, 
                                    &my_req, 
                                    &m_functor);

    } else if ( line.compare( "send" ) == 0 ) {

        m_hermes->send( NULL, 0, CHAR,
                                (my_rank + 1 ) % 2,
                                0xdead, 
                                GroupWorld, 
                                &m_functor);

    } else if ( line.compare( "barrier" ) == 0 ) {
        m_hermes->barrier( 0, &m_functor );
    } else if ( line.compare( "wait" ) == 0 ) {
        int flag;
        m_hermes->wait( &my_req, &my_resp, &m_functor );
    } else if ( line.compare( "fini" ) == 0 ) {
        m_hermes->fini( &m_functor );
    }

//    DBGX("\n");
}

void TestDriver::funcDone( int retval )
{
    DBGX("retval %d\n", retval );
    m_selfLink->send(1,NULL);
}
