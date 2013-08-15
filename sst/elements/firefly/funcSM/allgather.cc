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
            int verboseLevel, Output::output_location_t loc,
            Info* info, SST::Link*& progressLink, 
            ProtocolAPI* ctrlMsg, IO::Interface* io ) :
    FunctionSMInterface(verboseLevel,loc,info),
    m_dataReadyFunctor( IO_Functor(this,&AllgatherFuncSM::dataReady) ),
    m_toProgressLink( progressLink ),
    m_ctrlMsg( static_cast<CtrlMsg*>(ctrlMsg) ),
    m_event( NULL ),
    m_io( io )
{
    m_dbg.setPrefix("@t:AllgatherFuncSM::@p():@l ");
}

void AllgatherFuncSM::handleEnterEvent( SST::Event *e) 
{
    if ( m_setPrefix ) {
        char buffer[100];
        snprintf(buffer,100,"@t:%d:%d:AllgatherFuncSM::@p():@l ",
                    m_info->nodeId(), m_info->worldRank());
        m_dbg.setPrefix(buffer);

        m_setPrefix = false;
    }

    assert( NULL == m_event );
    m_event = static_cast< GatherEnterEvent* >(e);

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
        ioVec.resize( m_numChunks[i] );
        for ( unsigned int j = 0; j < ioVec.size(); j++ ) {
            int chunk = ( recvStartChunk + j ) % m_size;
            ioVec[j].ptr = chunkPtr( chunk ); 
            ioVec[j].len = chunkSize( chunk ); 
            m_dbg.verbose(CALL_INFO,1,0,"ioVec[%d] ptr=%p len=%lu\n",
                                    j,ioVec[j].ptr,ioVec[j].len); 
        }

        m_ctrlMsg->recvv( ioVec, src, AllgatherTag + i + 1, 
                            m_event->group, &m_recvReqV[i] );

        if ( i == 0 ) {
            m_ctrlMsg->recv( NULL, 0, src, AllgatherTag,
                                        m_event->group, &m_recvReq );
        }

        offset *= 2;
    
    }

    memcpy( chunkPtr(m_rank), m_event->sendbuf, chunkSize(m_rank) );
    m_currentStage = 0;

    m_dbg.verbose(CALL_INFO,1,0,"send ready\n");
    m_ctrlMsg->send( NULL, 0, m_dest[0], AllgatherTag,
                                        m_event->group, &m_sendReq );

    m_toProgressLink->send(0, NULL );

    m_state = WaitSendStart;
}

void AllgatherFuncSM::handleProgressEvent( SST::Event *e )
{
    switch( m_state ) {
    case WaitSendStart:
        if ( ! m_ctrlMsg->test( &m_sendReq ) ) {
            m_dbg.verbose(CALL_INFO,1,0,"call setDataReadyFunc\n");
            m_io->setDataReadyFunc( &m_dataReadyFunctor );
            break;
        }
        m_dbg.verbose(CALL_INFO,1,0,"switch to WaitRecvStart\n");
        m_state = WaitRecvStart;
        m_pending = false;

    case WaitRecvStart:
        if ( ! m_ctrlMsg->test( &m_recvReq ) ) {
            m_dbg.verbose(CALL_INFO,1,0,"call setDataReadyFunc\n");
            m_io->setDataReadyFunc( &m_dataReadyFunctor );
            break;
        }
        m_dbg.verbose(CALL_INFO,1,0,"switch to WaitSendData\n");
        m_state = WaitSendData;
        m_pending = false;

    case WaitSendData:
        if ( ! m_pending ) {
            std::vector<CtrlMsg::IoVec> ioVec;
            ioVec.resize( m_numChunks[m_currentStage] );

            for ( unsigned int j = 0; j < ioVec.size(); j++ ) {
                int chunk = (m_sendStartChunk[m_currentStage] + j) % m_size;
                ioVec[j].ptr = chunkPtr( chunk );
                ioVec[j].len = chunkSize( chunk ); 
            }
            m_dbg.verbose(CALL_INFO,1,0,"send data\n");
            m_ctrlMsg->sendv( ioVec, m_dest[m_currentStage], 
                AllgatherTag + m_currentStage + 1, m_event->group, &m_sendReq );
            m_pending = true;
            m_toProgressLink->send(0, NULL );
            break;
        } else {
            if ( ! m_ctrlMsg->test( & m_sendReq ) ) {
                m_dbg.verbose(CALL_INFO,1,0,"call setDataReadyFunc\n");
                m_io->setDataReadyFunc( &m_dataReadyFunctor );
                break;
            } 
        }
        m_dbg.verbose(CALL_INFO,1,0,"switch to WaitRecvData\n");
        m_state = WaitRecvData;
        m_pending = false;

    case WaitRecvData:
        if ( m_ctrlMsg->test( &m_recvReqV[m_currentStage] ) ) {
            m_dbg.verbose(CALL_INFO,1,0,"stage %d complete\n", m_currentStage);
            ++m_currentStage;
            if ( m_currentStage < m_dest.size() ) {
                m_toProgressLink->send(0, NULL );
                m_state = WaitSendData;
                m_pending = false;
                break;
            }  
        } else {
            m_dbg.verbose(CALL_INFO,1,0,"call setDataReadyFunc\n");
            m_io->setDataReadyFunc( &m_dataReadyFunctor );
            break;
        } 
        m_dbg.verbose(CALL_INFO,1,0,"leave\n");
        exit( static_cast<SMEnterEvent*>(m_event), 0 );
        delete m_event;
        m_event = NULL;
        m_pending = false;
    }
}

void AllgatherFuncSM::dataReady( IO::NodeId src )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    assert( m_event );
    m_io->setDataReadyFunc( NULL );
    m_toProgressLink->send(0, NULL );
}

