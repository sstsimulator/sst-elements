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
#include "sst/core/serialization.h"

#include "funcSM/allgather.h"

using namespace SST::Firefly;

inline long mod( long a, long b )
{
    return (a % b + b) % b;
}

AllgatherFuncSM::AllgatherFuncSM( 
        int verboseLevel, Output::output_location_t loc, Info* info,
        ProtocolAPI* ctrlMsg, SST::Link* selfLink ) :
    FunctionSMInterface(verboseLevel,loc,info),
    m_selfLink( selfLink ),
    m_ctrlMsg( static_cast<CtrlMsg*>(ctrlMsg) ),
    m_event( NULL ),
    m_seq( 0 )
{
    m_dbg.setPrefix("@t:AllgatherFuncSM::@p():@l ");
}

void AllgatherFuncSM::handleStartEvent( SST::Event *e) 
{
    if ( m_setPrefix ) {
        char buffer[100];
        snprintf(buffer,100,"@t:%d:%d:AllgatherFuncSM::@p():@l ",
                    m_info->nodeId(), m_info->worldRank());
        m_dbg.setPrefix(buffer);

        m_setPrefix = false;
    }
    ++m_seq;

    assert( NULL == m_event );
    m_event = static_cast< GatherStartEvent* >(e);

    m_rank = m_info->getGroup(m_event->group)->getMyRank();
    m_size = m_info->getGroup(m_event->group)->size();
    int offset = 1;

    int numStages = ceil( log2(m_size) );
    m_dbg.verbose(CALL_INFO,1,0,"numStages=%d rank=%d size=%d\n",
                                        numStages,m_rank,m_size);
    
    m_recvReqV.resize( numStages );
    m_numChunks.resize( numStages );
    m_sendStartChunk.resize( numStages );
    m_dest.resize( numStages );

    for ( int i = 0; i < numStages; i++ ) {
        int src = ( m_rank - offset );
        src = src < 0 ? m_size + src : src;

        m_dest[i] = mod( m_rank + offset, m_size );

        m_dbg.verbose(CALL_INFO,1,0,"setup stage %d, src %d, dest %d\n",
                                                        i, src, m_dest[i] ); 

        m_numChunks[i] = offset;
        int recvStartChunk;
        if ( i < numStages - 1 ) {  
            recvStartChunk = 
                        mod( (m_rank + 1) + ( offset * (m_size-2)), m_size );
            m_sendStartChunk[i] = 
                        mod( ( m_rank + 1) - ( offset + m_size), m_size );
        } else {
            m_numChunks[i]  = m_size - offset;
            recvStartChunk = mod( m_rank + 1, m_size);
            m_sendStartChunk[i] = mod( ( m_rank + 1) + offset, m_size );
        }
        m_dbg.verbose(CALL_INFO,1,0," 2^stage %d, sendStart %d, recvStart %d "
                    "numChunks %d\n", offset, m_sendStartChunk[i], 
                    recvStartChunk, m_numChunks[i] );

        std::vector<CtrlMsg::IoVec> ioVec;

        initIoVec(ioVec,recvStartChunk,m_numChunks[i]);

        m_ctrlMsg->recvv( ioVec, src, genTag() + i + 1, 
                            m_event->group, &m_recvReqV[i] );

        if ( i == 0 ) {
            m_ctrlMsg->recv( NULL, 0, src, genTag(),
                                        m_event->group, &m_recvReq );
        }

        offset *= 2;
    }

    memcpy( chunkPtr(m_rank), m_event->sendbuf, chunkSize(m_rank) );
    m_currentStage = 0;

    m_dbg.verbose(CALL_INFO,1,0,"send ready\n");
    m_ctrlMsg->send( NULL, 0, m_dest[0], genTag(),
                                        m_event->group, &m_sendReq );

    m_ctrlMsg->enter();

    m_state = WaitSendStart;

    m_delay = 0;
}

void AllgatherFuncSM::initIoVec( std::vector<CtrlMsg::IoVec>& ioVec,
                    int startChunk, int numChunks  )
{
    int currentChunk = startChunk;
    int nextChunk = ( currentChunk + 1 ) % m_size; 
    int countDown = numChunks;

    m_dbg.verbose(CALL_INFO,2,0,"startChunk=%d numChunks=%d\n",
                                            startChunk,numChunks);

    while ( countDown ) {  
        --countDown;
        CtrlMsg::IoVec jjj;
        jjj.ptr = chunkPtr( currentChunk ); 
        jjj.len = chunkSize( currentChunk );
        
        m_dbg.verbose(CALL_INFO,2,0,"currentChunk=%d ptr=%p len=%lu\n",
                    currentChunk, jjj.ptr,jjj.len);
    
        while ( countDown &&
                (unsigned char*) jjj.ptr + jjj.len == chunkPtr( nextChunk ) )
        {
            jjj.len += chunkSize( nextChunk );
            m_dbg.verbose(CALL_INFO,2,0,"len=%lu\n", jjj.len );
            currentChunk = nextChunk;
            nextChunk = ( currentChunk + 1 ) % m_size; 
            --countDown;
        }
        if ( 0 == countDown || 
                (unsigned char*) jjj.ptr + jjj.len != chunkPtr( nextChunk ) )
        {

            ioVec.push_back( jjj );
            m_dbg.verbose(CALL_INFO,2,0,"ioVec[%lu] ptr=%p len=%lu\n",
                                ioVec.size()-1,
                                ioVec[ioVec.size()-1].ptr,
                                ioVec[ioVec.size()-1].len); 
        }
        currentChunk = nextChunk;
        nextChunk = ( currentChunk + 1 ) % m_size; 
    }
}

void AllgatherFuncSM::handleEnterEvent( SST::Event *e )
{
    switch( m_state ) {
    case WaitSendStart:
        if ( ! m_delay ) {
            m_test = m_ctrlMsg->test( &m_sendReq, m_delay );
            if ( m_delay ) {
                m_dbg.verbose(CALL_INFO,1,0,"delay %d\n", m_delay );
                m_selfLink->send( m_delay, NULL );
                break;
            }
        } else {
            m_delay = 0;
        }
        if ( ! m_test ) {
            m_ctrlMsg->sleep();
            break;
        }
        m_dbg.verbose(CALL_INFO,1,0,"switch to WaitRecvStart\n");
        m_state = WaitRecvStart;
        m_pending = false;

    case WaitRecvStart:

        if ( ! m_delay ) {
            m_test = m_ctrlMsg->test( &m_recvReq, m_delay );
            if ( m_delay ) {
                m_dbg.verbose(CALL_INFO,1,0,"delay %d\n", m_delay );
                m_selfLink->send( m_delay, NULL );
                break;
            }
        } else {
            m_delay = 0;
        }
        if ( ! m_test ) {
            m_ctrlMsg->sleep();
            break;
        }
        m_dbg.verbose(CALL_INFO,1,0,"switch to WaitSendData\n");
        m_state = WaitSendData;
        m_pending = false;

    case WaitSendData:
        if ( ! m_pending ) {
            std::vector<CtrlMsg::IoVec> ioVec;

            initIoVec( ioVec, m_sendStartChunk[m_currentStage],
                            m_numChunks[m_currentStage] );

            m_dbg.verbose(CALL_INFO,1,0,"send data\n");
            m_ctrlMsg->sendv( ioVec, m_dest[m_currentStage], 
                genTag() + m_currentStage + 1, m_event->group, &m_sendReq );
            m_pending = true;
            m_ctrlMsg->enter();
            break;
        } else {
            if ( ! m_delay ) {
                m_test = m_ctrlMsg->test( & m_sendReq, m_delay );
                if ( m_delay ) {
                    m_dbg.verbose(CALL_INFO,1,0,"delay %d\n", m_delay );
                    m_selfLink->send( m_delay, NULL );
                    break;
                }
            } else {
                m_delay = 0;
            }
            if ( ! m_test ) {
                m_ctrlMsg->sleep();
                break;
            } 
        }
        m_dbg.verbose(CALL_INFO,1,0,"switch to WaitRecvData\n");
        m_state = WaitRecvData;
        m_pending = false;

    case WaitRecvData:
        if ( ! m_delay ) {
            m_test = m_ctrlMsg->test( &m_recvReqV[m_currentStage], m_delay );
            if ( m_delay ) {
                m_dbg.verbose(CALL_INFO,1,0,"delay %d\n", m_delay );
                m_selfLink->send( m_delay, NULL );
                break;
            }
        } else {
            m_delay = 0;
        }
        if ( m_test ) {
            m_dbg.verbose(CALL_INFO,1,0,"stage %d complete\n", m_currentStage);
            ++m_currentStage;
            if ( m_currentStage < m_dest.size() ) {
                m_state = WaitSendData;
                m_pending = false;
                handleEnterEvent( NULL );
                
                break;
            }  
        } else {
            m_ctrlMsg->sleep();
            break;
        } 
        m_dbg.verbose(CALL_INFO,1,0,"leave\n");
        exit( static_cast<SMStartEvent*>(m_event), 0 );
        delete m_event;
        m_event = NULL;
        m_pending = false;
    }
}
