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

#include "sst_config.h"

#include <sstream>

#include <sst/core/component.h>
#include <sst/core/debug.h>
#include <sst/core/element.h>
#include <sst/core/link.h>
#include <sst/core/module.h>
#include <sst/core/params.h>
#include <sst/core/timeLord.h>

#include "testDriver.h"

using namespace Hermes;
using namespace Hermes::MP;
using namespace SST;
using namespace SST::Firefly;

TestDriver::TestDriver(ComponentId_t id, Params &params) :
    Component( id ),
    m_sharedTraceFile(false),
    m_functor( DerivedFunctor( this, &TestDriver::funcDone ) ),
    my_rank( AnySrc ),
    my_size(0)
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
    //m_dbg.output(CALL_INFO,"loading module `%s`\n",name.c_str());

    Params hermesParams = params.find_prefix_params("hermesParams." );

    m_hermes = dynamic_cast<MP::Interface*>(
                loadModuleWithComponent( name, this, hermesParams )); 

    if ( !m_hermes ) {
        _abort(TestDriver, "ERROR:  Unable to find Hermes '%s'\n",
                                        name.c_str());
    }

    m_selfLink = configureSelfLink("Self", "100 ns",
        new Event::Handler<TestDriver>(this,&TestDriver::handle_event));

    m_sharedTraceFile = params.find_integer("sharedTraceFile",0);
    m_traceFileName = params.find_string("traceFile");

    m_bufLen = params.find_integer( "bufLen" );
    assert( m_bufLen != (size_t)-1 ); 

    m_recvBuf.resize(m_bufLen);
    m_sendBuf.resize(m_bufLen);
    
    m_root = params.find_integer( "root", 0 );
    for ( unsigned int i = 0; i < m_sendBuf.size(); i++ ) {
        m_sendBuf[i] = i;
    } 

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
}
    
TestDriver::~TestDriver()
{
}

void TestDriver::printStatus( Output& out )
{
    m_dbg.verbose(CALL_INFO,2,0, "\n" );
    m_hermes->printStatus( m_dbg );
}

void TestDriver::init( unsigned int phase )
{
    m_dbg.verbose(CALL_INFO,3,0,"\n");
    m_hermes->_componentInit( phase );
}

void TestDriver::setup() 
{ 
    m_hermes->_componentSetup( );

    std::ostringstream tmp;

    tmp << m_traceFileName.c_str();
    if ( ! m_sharedTraceFile ) {
        tmp << m_hermes->myWorldRank();
    }

    if ( m_hermes->myWorldRank() == 0 ) {
//        m_selfLink->send(10000,NULL);
        m_selfLink->send(1,NULL);
    } else {
        m_selfLink->send(1,NULL);
    }

    m_traceFile.open( tmp.str().c_str() );
    assert( ! m_traceFile.fail() );

    if ( m_hermes->myWorldRank() == 0 ) {
        m_dbg.verbose(CALL_INFO,1,0,"traceFile `%s`\n",tmp.str().c_str());
    }
#if 0
    if ( ! m_traceFile.is_open() ) {
        m_dbg.verbose(CALL_INFO,1,0,"traceFile `%s`\n",tmp.str().c_str());
        _abort(TestDriver, "ERROR:  Unable to open trace file '%s'\n",
                                        tmp.str().c_str() );
    }

#else
    std::string line;
    while  ( ! getline( m_traceFile, line ).eof() ) {
        m_fileBuffer.push_back(line);
    }

    m_traceFile.close();
#endif

    char buffer[100];
    snprintf(buffer,100,"@t:%d:TestDriver::@p():@l ",m_hermes->myWorldRank()); 
    m_dbg.setPrefix(buffer);

    m_collectiveOut = 0;
    m_collectiveIn = m_hermes->myWorldRank();
}

void TestDriver::handle_event( Event* ev )
{
#if 0 
    getline( m_traceFile, m_funcName );

    if ( m_funcName.empty() ) {
        primaryComponentOKToEndSim();
        return;
    }
#else
    do {
        if ( m_fileBuffer.empty() ) {
            primaryComponentOKToEndSim();
            return;
        } 
        m_funcName = m_fileBuffer.front();
        m_fileBuffer.pop_front();
    } while ( m_funcName[0] == '#' );

#endif

    m_dbg.verbose(CALL_INFO,2,0, "calling `%s`\n" , m_funcName.c_str());

    if ( m_funcName.compare( "init" ) == 0 ) {
        m_hermes->init( &m_functor );
    } else if ( m_funcName.compare( "size" ) == 0 ) {
        m_hermes->size( GroupWorld, &my_size, &m_functor );
    } else if ( m_funcName.compare( "rank" ) == 0 ) {
        m_hermes->rank( GroupWorld, &my_rank, &m_functor );
    } else if ( m_funcName.compare( "recv" ) == 0 ) {
        m_hermes->recv( &m_recvBuf[0], m_recvBuf.size(), CHAR, 
                                    (my_rank + 1) % 2, 
                                    AnyTag, 
                                    GroupWorld, 
                                    &my_resp[RecvReq], 
                                    &m_functor);

    } else if ( m_funcName.compare( "irecv" ) == 0 ) {
        m_hermes->irecv( &m_recvBuf[0], m_recvBuf.size(), CHAR, 
                                    (my_rank + 1) % 2, 
                                    AnyTag, 
                                    GroupWorld, 
                                    &my_req[RecvReq], 
                                    &m_functor);

    } else if ( m_funcName.compare( "send" ) == 0 ) {
        m_hermes->send( &m_sendBuf[0], m_sendBuf.size(), CHAR,
                                (my_rank + 1 ) % 2,
                                0xdead, 
                                GroupWorld, 
                                &m_functor);

    } else if ( m_funcName.compare( "isend" ) == 0 ) {
        m_hermes->isend( &m_sendBuf[0], m_sendBuf.size(), CHAR,
                                (my_rank + 1 ) % 2,
                                0xdead, 
                                GroupWorld, 
                                &my_req[SendReq],
                                &m_functor);

    } else if ( m_funcName.compare( "barrier" ) == 0 ) {
        m_hermes->barrier( GroupWorld, &m_functor );
    } else if ( m_funcName.compare( "allgather" ) == 0 ) {
        allgatherEnter();
    } else if ( m_funcName.compare( "allgatherv" ) == 0 ) {
        allgathervEnter();
    } else if ( m_funcName.compare( "gather" ) == 0 ) {
        gatherEnter();
    } else if ( m_funcName.compare( "gatherv" ) == 0 ) {
        gathervEnter();
    } else if ( m_funcName.compare( "alltoall" ) == 0 ) {
        alltoallEnter();
    } else if ( m_funcName.compare( "alltoallv" ) == 0 ) {
        alltoallvEnter();
    } else if ( m_funcName.compare( "reduce" ) == 0 ) {
        m_hermes->reduce( &m_collectiveIn, &m_collectiveOut, 1, INT,
                                SUM, m_root, GroupWorld, &m_functor );
    } else if ( m_funcName.compare( "allreduce" ) == 0 ) {
        m_hermes->allreduce( &m_collectiveIn, &m_collectiveOut, 1, INT,
                                SUM, GroupWorld, &m_functor );
    } else if ( m_funcName.compare( "waitSend" ) == 0 ) {
        m_hermes->wait( my_req[SendReq], &my_resp[SendReq], &m_functor );
    } else if ( m_funcName.compare( "waitRecv" ) == 0 ) {
        m_hermes->wait( my_req[RecvReq], &my_resp[RecvReq], &m_functor );
    } else if ( m_funcName.compare( "waitall" ) == 0 ) {
        my_respPtr[0] = &my_resp[0];
        my_respPtr[1] = &my_resp[1];
        m_hermes->waitall( 2, &my_req[0], &my_respPtr[0], &m_functor );
    } else if ( m_funcName.compare( "waitanySend" ) == 0 ) {
        m_hermes->waitany( 1, &my_req[0], &my_index, &my_resp[0], &m_functor );
    } else if ( m_funcName.compare( "waitanyRecv" ) == 0 ) {
        m_hermes->waitany( 1, &my_req[1], &my_index, &my_resp[0], &m_functor );
    } else if ( m_funcName.compare( "fini" ) == 0 ) {

        m_hermes->fini( &m_functor );
    }
}

bool TestDriver::funcDone( int retval )
{
    m_selfLink->send(1,NULL);

    if ( m_funcName.compare( "size" ) == 0 ) {
        m_dbg.verbose(CALL_INFO,1,0,"`%s` size=%d\n" , m_funcName.c_str(),
             my_size);
    } else if ( m_funcName.compare( "rank" ) == 0 ) {
        m_dbg.verbose(CALL_INFO,1,0,"`%s` rank=%d\n" , m_funcName.c_str(),
             my_rank);
    } else if ( m_funcName.compare( "waitRecv" ) == 0 ) {
        waitRecvReturn();
    } else if ( m_funcName.compare( "waitall" ) == 0 ) {
        waitallReturn();
    } else if ( m_funcName.compare( "waitanyRecv" ) == 0 ) {
        waitanyRecvReturn();
    } else if ( m_funcName.compare( "recv" ) == 0 ) {
        recvReturn();
    } else if ( m_funcName.compare( "allgather" ) == 0 ) {
        allgatherReturn();
    } else if ( m_funcName.compare( "allgatherv" ) == 0 ) {
        allgathervReturn();
    } else if ( m_funcName.compare( "gather" ) == 0 ) {
        gatherReturn();
    } else if ( m_funcName.compare( "gatherv" ) == 0 ) {
        gathervReturn();
    } else if ( m_funcName.compare( "alltoall" ) == 0 ) {
        alltoallReturn();
    } else if ( m_funcName.compare( "alltoallv" ) == 0 ) {
        alltoallvReturn();
    } else if ( m_funcName.compare( "allreduce" ) == 0 ) {
        m_dbg.verbose(CALL_INFO,1,0,"allreduce %s\n", 
          m_collectiveOut == ((my_size*(my_size+1))/2) - my_size ? "passed": "failed" );
    } else if ( m_funcName.compare( "reduce" ) == 0 ) {
        if ( m_root == my_rank ) {
            m_dbg.verbose(CALL_INFO,1,0,"reduce %s\n", 
              m_collectiveOut== ((my_size*(my_size+1))/2) - my_size ? "passed": "failed" );
        }
    } else {
        m_dbg.verbose(CALL_INFO,1,0,"`%s` retval=%d\n" ,
                                m_funcName.c_str(), retval);
    }
    return false;
}

void TestDriver::recvReturn( )
{
    m_dbg.verbose(CALL_INFO,1,0,"src=%d tag=%#x count=%d status=%d\n",
                my_resp[RecvReq].src, my_resp[RecvReq].tag, 
                my_resp[RecvReq].count, my_resp[RecvReq].status );
    if ( my_resp[RecvReq].count != m_recvBuf.size() ) {
        printf("ERROR %d != %lu\n", my_resp[RecvReq].count, m_recvBuf.size());
        return;
    }
    for ( unsigned int i = 0; i < m_recvBuf.size(); i++ ) {
        if ( m_recvBuf[i] != (i&0xff) ) {
            printf("ERROR %d != %d\n",i,m_recvBuf[i]);
        }
    }
}

void TestDriver::waitRecvReturn( )
{
    m_dbg.verbose(CALL_INFO,1,0,"src=%d tag=%#x count=%d status=%d\n",
                my_resp[RecvReq].src, my_resp[RecvReq].tag,
                my_resp[RecvReq].count, my_resp[RecvReq].status );
    if ( my_resp[RecvReq].count != m_recvBuf.size() ) {
        printf("ERROR %d != %lu\n", my_resp[RecvReq].count, m_recvBuf.size());
        return;
    }
    for ( unsigned int i = 0; i < m_recvBuf.size(); i++ ) {
        if ( m_recvBuf[i] != (i&0xff) ) {
            printf("ERROR %d != %d\n",i,m_recvBuf[i]);
        }
    }
}

void TestDriver::waitanyRecvReturn( )
{
    m_dbg.verbose(CALL_INFO,1,0,"src=%d tag=%#x count=%d status=%d\n",
                my_resp[my_index].src, my_resp[my_index].tag, 
                my_resp[my_index].count, my_resp[my_index].status );
    if ( my_resp[my_index].count != m_recvBuf.size() ) {
        printf("ERROR %d != %lu\n", my_resp[my_index].count, m_recvBuf.size());
        return;
    }
    for ( unsigned int i = 0; i < m_recvBuf.size(); i++ ) {
        if ( m_recvBuf[i] != (i&0xff) ) {
            printf("ERROR %d != %d\n",i,m_recvBuf[i]);
        }
    }
}

void TestDriver::waitallReturn( )
{
    m_dbg.verbose(CALL_INFO,1,0,"src=%d tag=%#x count=%d status=%d\n",
                my_resp[RecvReq].src, my_resp[RecvReq].tag, 
                my_resp[RecvReq].count, my_resp[RecvReq].status );

    m_dbg.verbose(CALL_INFO,1,0,"status=%d\n", my_resp[SendReq].status );

    if ( my_resp[RecvReq].count != m_recvBuf.size() ) {
        printf("ERROR %d != %lu\n", my_resp[RecvReq].count, m_recvBuf.size());
        return;
    }
    for ( unsigned int i = 0; i < m_recvBuf.size(); i++ ) {
        if ( m_recvBuf[i] != (i&0xff) ) {
            printf("ERROR %d != %d\n",i,m_recvBuf[i]);
        }
    }
}

void TestDriver::alltoallEnter( )
{
    if ( my_rank == m_root ) {
        m_dbg.verbose(CALL_INFO,1,0,"my_rank=%d\n", my_rank);
    }

    m_intSendBuf.resize( m_bufLen * my_size );
    m_intRecvBuf.resize( m_bufLen * my_size );

    for ( int i = 0; i < my_size; i++ ) {
        for ( unsigned int j = 0; j < m_bufLen; j++ ) {
            m_intSendBuf[ i * m_bufLen + j ] = (my_rank << 16) | (j  + 1); 
            m_intRecvBuf[ i * m_bufLen + j ] = 0;
        }
    } 

    m_hermes->alltoall( &m_intSendBuf[0],  m_intSendBuf.size()/my_size, INT,
                    &m_intRecvBuf[0],  m_intRecvBuf.size()/my_size, INT,
                    GroupWorld, &m_functor );
}

void TestDriver::alltoallReturn( )
{
    bool passed = true;

    for ( int i = 0; i < my_size; i++ ) {
        for ( unsigned int j = 0; j < m_bufLen; j++ ) {
            int pos = i * m_bufLen + j;
            if ( (unsigned ) m_intRecvBuf[ pos ] != 
                                    ( ( i << 16 ) | (j + 1)) ) 
            {
                m_dbg.verbose(CALL_INFO,1,0,"%d, failed, want %#010x %#010x\n",
                                i, (i << 16 ) | (j + 1), 
                                m_intRecvBuf[pos]);
                passed = false;
            } 
        }
    }
    if ( passed ) {
        m_dbg.verbose(CALL_INFO,1,0,"alltoall[v] passed\n");
    }
}

void TestDriver::alltoallvEnter( )
{
    if ( my_rank == m_root ) {
        m_dbg.verbose(CALL_INFO,1,0,"my_rank=%d\n", my_rank);
    }

    m_intSendBuf.resize( m_bufLen * my_size );
    m_intRecvBuf.resize( m_bufLen * my_size );

    m_sendcnts.resize(my_size);
    m_senddispls.resize(my_size);
    m_recvcnts.resize(my_size);
    m_recvdispls.resize(my_size);

    for ( int i = 0; i < my_size; i++ ) {
        for ( unsigned int j = 0; j < m_bufLen; j++ ) {
            m_intSendBuf[ i * m_bufLen + j ] = (my_rank << 16) | (j  + 1); 
            m_intRecvBuf[ i * m_bufLen + j ] = 0;
        }
    } 

    for ( int i = 0; i < my_size; i++ ) {
        m_recvcnts[i] = m_bufLen;
        m_recvdispls[i] = i * m_bufLen * m_hermes->sizeofDataType(INT); 
        m_sendcnts[i] = m_bufLen;
        m_senddispls[i] = i * m_bufLen * m_hermes->sizeofDataType(INT); 
    }

    m_hermes->alltoallv( &m_intSendBuf[0], &m_sendcnts[0], &m_senddispls[0],INT,
                    &m_intRecvBuf[0], &m_recvcnts[0], &m_recvdispls[0], INT,
                    GroupWorld, &m_functor );
}

void TestDriver::alltoallvReturn( )
{
    TestDriver::alltoallReturn( );
}

// GATHER
void TestDriver::gatherEnter( )
{
    if ( my_rank == m_root ) {
        m_dbg.verbose(CALL_INFO,1,0,"my_rank=%d\n", my_rank);
    }
    assert( my_size != 0 ); 

    m_gatherRecvBuf.resize(m_bufLen * my_size);
    m_gatherSendBuf.resize(m_bufLen);

    for ( unsigned int i = 0; i < m_gatherSendBuf.size(); i++ ) {
        m_gatherSendBuf[ i ] = my_rank; 
    }

    m_hermes->gather( &m_gatherSendBuf[0],  m_gatherSendBuf.size(), INT,
                    &m_gatherRecvBuf[0],  m_gatherRecvBuf.size()/my_size, INT,
                    m_root, GroupWorld, &m_functor );
}

void TestDriver::gatherReturn( )
{
    if ( my_rank == m_root ) {
        bool passed = true; 
        for ( int i = 0; i < my_size; i++ ) { 
            for ( unsigned int j = 0; j < m_gatherRecvBuf.size()/my_size; j++ ) { 
                if ( (int) m_gatherRecvBuf[i * m_gatherRecvBuf.size()/my_size] != i ) {
                    m_dbg.verbose(CALL_INFO,1,0,"failed, want %d  %d\n",i, 
                                m_gatherRecvBuf[i * m_gatherRecvBuf.size()/my_size]);
                    passed = false;
                }
            }
        }
        if ( passed ) {
            m_dbg.verbose(CALL_INFO,1,0,"gather passed\n");
        }
    }
}

// GATHERV 
void TestDriver::gathervEnter( )
{
    if ( my_rank == m_root ) {
        m_dbg.verbose(CALL_INFO,1,0,"my_rank=%d\n", my_rank);
    }
    assert( my_size != 0 ); 

    m_gathervSendBuf.resize( m_bufLen );

    for ( unsigned int i = 0; i < m_gathervSendBuf.size(); i++ ) {
        m_gathervSendBuf[ i ] = my_rank;
    }

    if ( my_rank == m_root ) {
        m_recvcnts.resize( my_size );
        m_recvdispls.resize( my_size );

        m_gathervRecvBuf.resize( my_size * m_gathervSendBuf.size() );

        for ( int i = 0; i < my_size; i++ ) {
            m_recvcnts[i] =  m_gathervSendBuf.size();
            m_recvdispls[i] = 
                i* m_gathervSendBuf.size() * m_hermes->sizeofDataType(INT); 
        }
    }

    m_hermes->gatherv( &m_gathervSendBuf[0],  m_gathervSendBuf.size(), INT,
                        &m_gathervRecvBuf[0],  &m_recvcnts[0],
                        &m_recvdispls[0], INT,
                        m_root, GroupWorld, &m_functor );
}

void TestDriver::gathervReturn( )
{
    if ( my_rank == m_root ) {
        m_dbg.verbose(CALL_INFO,1,0,"\n");
        bool passed = true;
        for ( int i = 0; i < my_size; i++ ) { 
            for ( unsigned int j = 0; j < m_gathervRecvBuf.size()/my_size; j++ ) { 
                if ( (int) m_gathervRecvBuf[i * m_gathervRecvBuf.size()/my_size] != i ) {
                    m_dbg.verbose(CALL_INFO,1,0,"failed, want %d  %d\n",i, 
                                m_gathervRecvBuf[i * m_gathervRecvBuf.size()/my_size]);
                    passed = false;
                }
            }
        }
        if ( passed ) {
            m_dbg.verbose(CALL_INFO,1,0,"gatherv passed\n");
        }
    }
}

// ALLGATHER
void TestDriver::allgatherEnter( )
{
    m_dbg.verbose(CALL_INFO,1,0,"my_rank=%d\n", my_rank);
    assert( my_size != 0 ); 

    m_allgatherSendBuf.resize( m_bufLen );

    for ( unsigned int i = 0; i < m_allgatherSendBuf.size(); i++ ) {
        m_allgatherSendBuf[ i ] = my_rank;
    }

    m_allgatherRecvBuf.resize( my_size * m_allgatherSendBuf.size() );

    m_hermes->allgather( &m_allgatherSendBuf[0],m_allgatherSendBuf.size(), INT,
                &m_allgatherRecvBuf[0], m_allgatherRecvBuf.size()/my_size, INT,
                GroupWorld, &m_functor );
}

void TestDriver::allgatherReturn( )
{
    bool passed = true;
    for ( int i = 0; i < my_size; i++ ) { 
        for ( unsigned int j = 0; j < m_allgatherRecvBuf.size()/my_size; j++ ) { 
            if ( (int) m_allgatherRecvBuf[i * m_allgatherRecvBuf.size()/my_size] != i ) {
                m_dbg.verbose(CALL_INFO,1,0,"failed, want %d  %d\n",i, 
                            m_allgatherRecvBuf[i * m_allgatherRecvBuf.size()/my_size]);
                passed = false;
            }
        }
    }
    if ( passed ) {
        m_dbg.verbose(CALL_INFO,1,0,"allgather passed\n");
    }
}

// ALLGATHERV
void TestDriver::allgathervEnter( )
{
    m_dbg.verbose(CALL_INFO,1,0,"my_rank=%d\n", my_rank);
    assert( my_size != 0 ); 

    m_allgatherSendBuf.resize( m_bufLen );

    for ( unsigned int i = 0; i < m_allgatherSendBuf.size(); i++ ) {
        m_allgatherSendBuf[ i ] = my_rank;
    }

    m_recvcnts.resize( my_size );
    m_recvdispls.resize( my_size );

    m_allgatherRecvBuf.resize( my_size * m_allgatherSendBuf.size() );

    for ( int i = 0; i < my_size; i++ ) {
        m_recvcnts[i] =  m_allgatherSendBuf.size();
        m_recvdispls[i] = 
                i* m_allgatherSendBuf.size() * m_hermes->sizeofDataType(INT); 
    }

    m_hermes->allgatherv(&m_allgatherSendBuf[0], m_allgatherSendBuf.size(), INT,
                &m_allgatherRecvBuf[0],  &m_recvcnts[0], &m_recvdispls[0], INT,
                GroupWorld, &m_functor );
}

void TestDriver::allgathervReturn( )
{
    bool passed = true;
    for ( int i = 0; i < my_size; i++ ) { 
        for ( unsigned int j = 0; j < m_allgatherRecvBuf.size()/my_size; j++ ) { 
            if ( (int) m_allgatherRecvBuf[i * m_allgatherRecvBuf.size()/my_size] != i ) {
                m_dbg.verbose(CALL_INFO,1,0,"failed, want %d  %d\n",i, 
                            m_allgatherRecvBuf[i * m_allgatherRecvBuf.size()/my_size]);
                passed = false;
            }
        }
    }
    if ( passed ) {
        m_dbg.verbose(CALL_INFO,1,0,"allgatherv passed\n");
    }
}
