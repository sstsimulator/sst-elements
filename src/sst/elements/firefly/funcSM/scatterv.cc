// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
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

#include "funcSM/scatterv.h"
#include "info.h"

using namespace SST::Firefly;

void ScattervFuncSM::handleStartEvent( SST::Event *e, Retval& retval )
{
    assert( NULL == m_event );
    m_event = static_cast< ScattervStartEvent* >(e);

    ++m_seq;

	int sendSize;

	if ( m_event->root >= m_info->getGroup(m_event->group)->getSize() ) {
		m_dbg.fatal( CALL_INFO, -1, "Scatterv root is %d, group size is %d\n", m_event->root,  m_info->getGroup(m_event->group)->getSize() );
	}

    if ( m_event->sendCntPtr ) {
       sendSize = ((int*)m_event->sendCntPtr)[m_event->root] * m_info->sizeofDataType( m_event->sendType );
    } else {
       sendSize = m_event->sendCnt * m_info->sizeofDataType( m_event->sendType );
    }

	int recvSize = m_event->recvCnt * m_info->sizeofDataType( m_event->recvType );

    if ( m_info->getGroup(m_event->group)->getMyRank() == 0 && sendSize != recvSize ) {
		m_dbg.fatal( CALL_INFO, -1, "send data size [%d] must equal recv data size [%d]  \n", sendSize, recvSize );
	}

    m_tree = new MaryTree( 4, m_info->getGroup(m_event->group)->getMyRank(),
                m_info->getGroup(m_event->group)->getSize(), m_event->root );

    m_dbg.debug(CALL_INFO,1,0,"group %d, root %d, size %d, rank %d\n",
                m_event->group, m_event->root, m_tree->size(),
                m_tree->myRank());

    m_dbg.debug(CALL_INFO,1,0,"parent %d \n",m_tree->parent());
    for ( unsigned int i = 0; i < m_tree->numChildren(); i++ ) {
        m_dbg.debug(CALL_INFO,1,0,"child[%d]=%d\n",i,m_tree->calcChild(i));
    }

	m_callback = new Callback;
	if ( -1 != m_tree->parent() ) {
		*m_callback = std::bind( &ScattervFuncSM::recvSize, this  );
	} else {
		std::vector<int>* buf = new std::vector<int>( m_tree->numDes() + 1);

		for ( int i =0; i < m_info->getGroup(m_event->group)->getSize() * 2; i++ ) {
			m_dbg.debug(CALL_INFO,1,0,"%ld \n",((long*)m_event->sendBuf.getBacking())[i]);
		}

		SendInfo* info = new SendInfo( buf, m_tree->numChildren() );

		for ( int i = 0; i < m_tree->numDes() + 1; i++ ) {

			if ( m_event->sendCntPtr ) {
				(*buf)[i] = m_event->sendCntPtr[i] * m_info->sizeofDataType( m_event->sendType );
			} else {
				(*buf)[i] = m_event->sendCnt * m_info->sizeofDataType( m_event->sendType );
			}
			m_dbg.debug(CALL_INFO,1,0,"node %d has %d bytes of data\n", i, (*buf)[i]);
		}

		*m_callback = std::bind( &ScattervFuncSM::sendSize, this, info, (RecvInfo*)  NULL );
	}
	if ( 0 != m_event->root && m_event->root == m_info->getGroup(m_event->group)->getMyRank() ) {

		// root is not 0, we need to swap new root data with rank 0 

		char* backing = (char*) m_event->sendBuf.getBacking();

		if ( backing) {
			void* tmp = malloc( sendSize );

			// save rank 0 data
			memcpy( tmp, backing, sendSize );

			size_t offset;
			if ( m_event->sendDisplsPtr ) {
				offset = m_event->sendDisplsPtr[m_event->root];
			} else {
				offset = m_event->root * m_event->sendCnt * m_info->sizeofDataType( m_event->sendType );
			}

			// calc ptr of new root rank's data
			void *ptr = backing + offset;

			// copy new root rank data to rank 0 
			memcpy( backing, ptr ,sendSize );

			// copy rank 0 data to new root rank
			memcpy( ptr, tmp, sendSize );
			free( tmp );
		}
	}

    handleEnterEvent( retval );
}

bool ScattervFuncSM::recvSize( )
{
	m_dbg.debug(CALL_INFO,1,0,"numDes %d parent %d\n",m_tree->numDes(), m_tree->parent() );

	std::vector<int>* buf = new std::vector<int>( m_tree->numDes() + 1 );

	int tag = genTag(SizeMsg);
	m_dbg.debug(CALL_INFO,1,0,"recv from parent=%d tag=%x\n",m_tree->parent(), tag );
    proto()->recv( buf->data(), buf->size() * sizeof(int), m_tree->parent(), tag, m_event->group );

	m_callback = new Callback;
	*m_callback = std::bind( &ScattervFuncSM::recvSizeDone, this, buf );
	return false;
}

bool ScattervFuncSM::recvSizeDone( std::vector<int>* buf )
{
	m_dbg.debug(CALL_INFO,1,0,"received %d bytes\n", (*buf)[0] );

	int bufLen = 0;
	for ( int i = 1; i < m_tree->numDes() + 1; i++ ) {
		m_dbg.debug(CALL_INFO,1,0,"descendant %d part %d \n",m_tree->myRank()+i,(*buf)[i]);
		bufLen += (*buf)[i];
	}

	RecvInfo* rInfo = new RecvInfo;

	rInfo->ioVec.push_back( IoVec( m_event->recvBuf, (*buf)[0]) );

	m_callback = new Callback;
	if ( m_tree->numChildren() ) {
		void *backing = NULL;
		if ( m_event->recvBuf.getBacking() ) {
			backing = malloc(bufLen);
		}
		rInfo->ioVec.push_back( IoVec( MemAddr( 1, backing ), bufLen ) );
		SendInfo* sInfo = new SendInfo( buf, m_tree->numChildren() );
		*m_callback = std::bind( &ScattervFuncSM::sendSize, this, sInfo, rInfo );
		rInfo->bufPos = (char*)backing;
	} else {
		*m_callback = std::bind( &ScattervFuncSM::dataWait, this, (SendInfo*)NULL, rInfo );
	}

	int tag = genTag(DataMsg);

	m_dbg.debug(CALL_INFO,1,0,"call irecvv() from parent=%d tag=%x numVec=%zu\n",m_tree->parent(), tag,rInfo->ioVec.size());
	proto()->irecvv( rInfo->ioVec, m_tree->parent(), tag, m_event->group, &rInfo->req );

	return false;
}

bool ScattervFuncSM::sendSize( SendInfo* sendInfo, RecvInfo* info )
{
	int* ptr = &sendInfo->sizeBuf->data()[sendInfo->bufPos];

	int treeSize = m_tree->calcChildTreeSize(sendInfo->count);
	m_dbg.debug(CALL_INFO,1,0,"child %d tree size %d\n",sendInfo->count, treeSize );

	size_t length = sizeof(int) * treeSize;

	m_dbg.debug(CALL_INFO,1,0,"bufPos=%d length=%zu\n",sendInfo->bufPos,length);

	sendInfo->bufPos += treeSize;

	int tag = genTag(SizeMsg);
	m_dbg.debug(CALL_INFO,1,0,"isend to %d tag %x\n", m_tree->calcChild( sendInfo->count ), tag);
    int vn = 0;
    if ( length <= m_smallCollectiveSize ) {
        vn = m_smallCollectiveVN;
    }
    proto()->isend( ptr, length, m_tree->calcChild( sendInfo->count ), tag, m_event->group, &sendInfo->reqs[sendInfo->count], vn );

	++sendInfo->count;

	m_callback = new Callback;
	if ( sendInfo->count == sendInfo->reqs.size() ) {
		*m_callback = std::bind( &ScattervFuncSM::sendSizeWait, this, sendInfo, info );
	} else {
		*m_callback = std::bind( &ScattervFuncSM::sendSize, this, sendInfo, info );
	}
	return false;
}

bool ScattervFuncSM::sendSizeWait( SendInfo* sInfo, RecvInfo* rInfo )
{
	m_dbg.debug(CALL_INFO,1,0,"\n");
	proto()->waitAll( sInfo->reqs );

	m_callback = new Callback;

	if ( rInfo ) {
		*m_callback = std::bind( &ScattervFuncSM::dataWait, this, sInfo, rInfo );
	} else {
		sInfo->count = 0;
		*m_callback = std::bind( &ScattervFuncSM::dataSend, this, sInfo, rInfo );
	}
	return false;
}

bool ScattervFuncSM::dataWait( SendInfo* sInfo , RecvInfo* rInfo )
{
	m_dbg.debug(CALL_INFO,1,0,"\n");

	proto()->wait( &rInfo->req );

	m_callback = new Callback;
	if ( m_tree->numChildren() ) {
		sInfo->count = 0;
		*m_callback = std::bind( &ScattervFuncSM::dataSend, this, sInfo, rInfo );
	} else {
		*m_callback = std::bind( &ScattervFuncSM::exit, this, sInfo, rInfo );
	}
	return false;
}

bool ScattervFuncSM::dataSend( SendInfo* sInfo , RecvInfo* rInfo )
{
	std::vector<IoVec> ioVec;

	if ( -1 == m_tree->parent() ) {
		int x = m_tree->calcChildTreeSize(sInfo->count);
		int child = m_tree->calcChild(sInfo->count);

		m_dbg.debug(CALL_INFO,1,0,"childNum=%d childNode=%d childTreeSize=%d\n",sInfo->count, child, x );

		for ( int i = 0; i < x ; i++ ) {

			char* backing = (char*) m_event->sendBuf.getBacking();
			int pos = (m_tree->getOrig(child) - m_tree->getOrig( m_tree->myRank()) ) + i;
			int length = (*sInfo->sizeBuf)[pos];
			size_t offset;

			if ( m_event->sendDisplsPtr ) {
				offset = m_event->sendDisplsPtr[pos];
			} else {
				offset = pos * m_event->sendCnt * m_info->sizeofDataType( m_event->sendType );
			}

			uint64_t simAddr = m_event->sendBuf.getSimVAddr() + offset;

			if ( backing ) {
				backing += offset;
			}

			m_dbg.debug(CALL_INFO,1,0,"des %d length=%d offset=%zu simAddr=%" PRIx64 " backing=%p\n",pos,length,offset,simAddr,backing);

			ioVec.push_back( IoVec( MemAddr(simAddr,backing), length ) );
		}

	} else {
		int x = m_tree->calcChildTreeSize(sInfo->count);
		int child = m_tree->calcChild(sInfo->count);
		m_dbg.debug(CALL_INFO,1,0,"childNum=%d childNode=%d childTreeSize=%d\n",sInfo->count, child, x );
		int length = 0;

		for ( int i = 0; i < x; i++ ) {
			length += ((int*)sInfo->sizeBuf->data())[ ( m_tree->getOrig(child) - m_tree->getOrig(m_tree->myRank())) + i];
		}
		MemAddr addr = rInfo->ioVec[1].addr;
		void* backing = NULL;
		if ( addr.getBacking() ) {
			backing = rInfo->bufPos;
			rInfo->bufPos += length;
		}

		m_dbg.debug(CALL_INFO,1,0,"backing=%p length=%d\n",backing,length);
		ioVec.push_back( IoVec( MemAddr( 1, backing ), length) );
	}

	int tag = genTag(DataMsg);
	m_dbg.debug(CALL_INFO,1,0,"isendv to child=%d tag=%x\n",m_tree->calcChild( sInfo->count ), tag);
    int vn = 0;
    size_t length = 0;
    for ( int i = 0; i < ioVec.size(); i++ ) {
        length += ioVec[i].len;
    }
    if ( length <= m_smallCollectiveSize ) {
        vn = m_smallCollectiveVN;
    }
	proto()->isendv( ioVec, m_tree->calcChild( sInfo->count ), tag, m_event->group, &sInfo->reqs[sInfo->count], vn );

	++sInfo->count;

	m_callback = new Callback;
	if ( sInfo->count == sInfo->reqs.size() ) {
		*m_callback = std::bind( &ScattervFuncSM::dataSendWait, this, sInfo, rInfo );
	} else {
		*m_callback = std::bind( &ScattervFuncSM::dataSend, this, sInfo, rInfo );
	}
	return false;
}
bool ScattervFuncSM::dataSendWait( SendInfo* sInfo , RecvInfo* rInfo )
{
	m_dbg.debug(CALL_INFO,1,0,"\n");
	proto()->waitAll( sInfo->reqs );

	m_callback = new Callback;
	*m_callback = std::bind( &ScattervFuncSM::exit, this, sInfo, rInfo );

	return false;
}

bool ScattervFuncSM::exit( SendInfo* sInfo , RecvInfo* rInfo )
{
	m_dbg.debug(CALL_INFO,1,0,"\n");
	if ( sInfo ) {
		delete sInfo;
	}
	if ( rInfo ) {
		delete rInfo;
	}
	return true;
}
