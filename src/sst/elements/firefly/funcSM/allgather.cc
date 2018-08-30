// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>

#include <cmath>
#include <string.h>


#include "funcSM/allgather.h"

using namespace SST::Firefly;

const char* AllgatherFuncSM::m_enumName[] = {
    FOREACH_ENUM(GENERATE_STRING)
};

AllgatherFuncSM::AllgatherFuncSM( SST::Params& params ) :
    FunctionSMInterface( params ),
    m_event( NULL ),
    m_seq( 0 )
{ }

void AllgatherFuncSM::handleStartEvent( SST::Event *e, Retval& retval ) 
{
    ++m_seq;

    assert( NULL == m_event );
    m_event = static_cast< GatherStartEvent* >(e);

    m_rank = m_info->getGroup(m_event->group)->getMyRank();
    m_size = m_info->getGroup(m_event->group)->getSize();

    int numStages = ceil( log2(m_size) );
    m_dbg.debug(CALL_INFO,1,0,"numStages=%d rank=%d size=%d\n",
                                        numStages, m_rank, m_size );
    
    m_recvReqV.resize( numStages );
    m_numChunks.resize( numStages );
    m_sendStartChunk.resize( numStages );
    m_dest.resize( numStages );

    m_state = Setup;
    m_currentStage = 0;

    m_setupState.init();
    handleEnterEvent( retval );
}

bool AllgatherFuncSM::setup( Retval& retval )
{
	Hermes::MemAddr addr;
    std::vector<IoVec> ioVec;
    int recvStartChunk;
    int src; 

    switch ( m_setupState.state ) {

      case SetupState::PostStartMsgRecv:

        src = m_info->getGroup(m_event->group)->getMapping( calcSrc( m_setupState.offset ) );

        m_dbg.debug(CALL_INFO,1,0,"post recv for ready msg from %d\n", src);
		addr.setSimVAddr( 1 );
		addr.setBacking( NULL );
        proto()->irecv( addr, 0, src, genTag(), &m_recvReq );
        m_setupState.state = SetupState::PostStageRecv;
        return false;

      case SetupState::PostStageRecv:

        src = m_info->getGroup(m_event->group)->getMapping( calcSrc( m_setupState.offset ) );
        m_dest[m_setupState.stage] = mod(m_rank + m_setupState.offset, m_size);

        m_dbg.debug(CALL_INFO,1,0,"setup stage %d, src %d, dest %d\n",
                   m_setupState.stage, src, 
					m_info->getGroup(m_event->group)->getMapping( m_dest[m_setupState.stage] ) ); 

        if ( m_setupState.stage < m_dest.size() - 1 ) {  
            m_numChunks[m_setupState.stage] = m_setupState.offset;
            recvStartChunk = 
               mod( (m_rank + 1) + (m_setupState.offset * (m_size-2)), m_size );
            m_sendStartChunk[m_setupState.stage] = 
               mod( (m_rank + 1) - (m_setupState.offset + m_size), m_size );
        } else {
            m_numChunks[m_setupState.stage]  = m_size - m_setupState.offset;
            recvStartChunk = mod( m_rank + 1, m_size);
            m_sendStartChunk[m_setupState.stage] = 
               mod( ( m_rank + 1) + m_setupState.offset, m_size );
        }
        m_dbg.debug(CALL_INFO,1,0," 2^stage %d, sendStart %d, recvStart %d "
                    "numChunks %d\n", m_setupState.offset, 
                    m_sendStartChunk[ m_setupState.stage], 
                    recvStartChunk, m_numChunks[m_setupState.stage] );

        initIoVec( ioVec, recvStartChunk, m_numChunks[m_setupState.stage] );

        proto()->irecvv( ioVec, src, genTag() + m_setupState.stage + 1, 
                            &m_recvReqV[m_setupState.stage] );

        m_setupState.offset *= 2;
        ++m_setupState.stage;

        if ( m_setupState.stage == m_dest.size()  ) {
            m_setupState.state = SetupState::SendStartMsg;
        }
        return false;

      case SetupState::SendStartMsg:

        memcpy( chunkPtr(m_rank), m_event->sendbuf.getBacking(), chunkSize(m_rank) );

        m_dbg.debug(CALL_INFO,1,0,"send ready message to %d\n",
						m_info->getGroup(m_event->group)->getMapping(m_dest[0]));
		
		addr.setSimVAddr( 1 );
		addr.setBacking( NULL );
        proto()->send( addr, 0, m_info->getGroup(m_event->group)->getMapping( m_dest[0] ), genTag() );
    }
    return true;
}

void AllgatherFuncSM::handleEnterEvent( Retval& retval )
{
    std::vector<IoVec> ioVec;
    m_dbg.debug(CALL_INFO,1,0,"%s\n", stateName(m_state).c_str());

    switch( m_state ) {
    case Setup:
        if ( setup( retval ) ) {
            m_state = WaitForStartMsg;
        }
        return;

    case WaitForStartMsg:
        m_dbg.debug(CALL_INFO,1,0,"wait for Start message to arrive\n");
        proto()->wait( &m_recvReq );
        m_state = SendData;
        return;

    case SendData:
        initIoVec( ioVec, m_sendStartChunk[m_currentStage],
                            m_numChunks[m_currentStage] );

        m_dbg.debug(CALL_INFO,1,0,"send stage %d, dest %d\n",
                        m_currentStage,m_info->getGroup(m_event->group)->getMapping(m_dest[m_currentStage]) );
        proto()->sendv( ioVec, m_info->getGroup(m_event->group)->getMapping(m_dest[m_currentStage]), 
               genTag() + m_currentStage + 1 );
        m_state = WaitRecvData;
        return;

    case WaitRecvData:
        m_dbg.debug(CALL_INFO,1,0,"wait stage %d\n", m_currentStage);
        proto()->wait( &m_recvReqV[m_currentStage] );
        ++m_currentStage;
        if ( m_currentStage < m_dest.size() ) {
            m_state = SendData;
        } else {
            m_state = Exit;
        }
        return;

    case Exit:
        m_dbg.debug(CALL_INFO,1,0,"leave\n");
        retval.setExit( 0 );
        delete m_event;
        m_event = NULL;
    }
}

void AllgatherFuncSM::initIoVec( std::vector<IoVec>& ioVec,
                    int startChunk, int numChunks  )
{
    int currentChunk = startChunk;
    int nextChunk = ( currentChunk + 1 ) % m_size; 
    int countDown = numChunks;

    m_dbg.debug(CALL_INFO,2,0,"startChunk=%d numChunks=%d\n",
                                            startChunk,numChunks);

    while ( countDown ) {  
        --countDown;
        IoVec jjj;
		jjj.addr.setSimVAddr( 1 );
        jjj.addr.setBacking( chunkPtr( currentChunk ) ); 
        jjj.len = chunkSize( currentChunk );
        
        m_dbg.debug(CALL_INFO,2,0,"currentChunk=%d ptr=%p len=%lu\n",
                    currentChunk, jjj.addr.getBacking(), jjj.len);
    
        while ( countDown &&
                (unsigned char*) jjj.addr.getBacking() + jjj.len == chunkPtr( nextChunk ) )
        {
            jjj.len += chunkSize( nextChunk );
            m_dbg.debug(CALL_INFO,2,0,"len=%lu\n", jjj.len );
            currentChunk = nextChunk;
            nextChunk = ( currentChunk + 1 ) % m_size; 
            --countDown;
        }
        if ( 0 == countDown || 
                (unsigned char*) jjj.addr.getBacking() + jjj.len != chunkPtr( nextChunk ) )
        {

            ioVec.push_back( jjj );
            m_dbg.debug(CALL_INFO,2,0,"ioVec[%lu] ptr=%p len=%lu\n",
                                ioVec.size()-1,
                                ioVec[ioVec.size()-1].addr.getBacking(),
                                ioVec[ioVec.size()-1].len); 
        }
        currentChunk = nextChunk;
        nextChunk = ( currentChunk + 1 ) % m_size; 
    }
}
