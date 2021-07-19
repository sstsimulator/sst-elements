// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "nic.h"

using namespace SST;
using namespace SST::Firefly;

void Nic::Shmem::handleEvent( NicShmemCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d `%s` targetNode=%d targetCore=%d\n",
            id, event->getTypeStr().c_str(), event->getNode(), event->getPid() );

    switch (event->type) {

    // operations that will take place only from the NIC
    case NicShmemCmdEvent::Init:
    case NicShmemCmdEvent::RegMem:
    case NicShmemCmdEvent::Cswap:
    case NicShmemCmdEvent::Swap:
    case NicShmemCmdEvent::Fence:
        handleNicEvent( event, id );
        break;

    case NicShmemCmdEvent::Fadd:
    case NicShmemCmdEvent::Add:
    case NicShmemCmdEvent::Put:
    case NicShmemCmdEvent::Putv:
    case NicShmemCmdEvent::Get:
    case NicShmemCmdEvent::Getv:

        if ( event->getNode() == m_nic.getNodeId() )  {
			handleHostEvent( event, id );
        } else {
            handleNicEvent( event, id );
        }
        break;

    // operations that take place only from the Host
    case NicShmemCmdEvent::Wait:
        hostWait( static_cast< NicShmemOpCmdEvent*>(event), id );
        break;
    }
}

void Nic::Shmem::handleHostEvent( NicShmemCmdEvent* event, int id )
{
	if ( m_hostBusy ) {
    	m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"busy core=%d %s\n",id,event->getTypeStr().c_str());
		m_hostCmdQ.push( std::make_pair(event,id) );
	} else {
   		m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"start busy core=%d %s\n",id,event->getTypeStr().c_str());
		m_hostBusy = true;
		m_nic.schedCallback(
			[=](){
    			m_dbg.verbosePrefix( prefix(),CALL_INFO_LAMBDA,"handleHostEvent",1,NIC_DBG_SHMEM,"ready core=%d\n",id);
				m_hostBusy = false;
				if ( ! m_hostCmdQ.empty() ) {
					handleHostEvent( m_hostCmdQ.front().first, m_hostCmdQ.front().second );
					m_hostCmdQ.pop();
				}
			},
			m_hostCmdLatency
		);

		handleHostEvent2( event, id );
	}
}

void Nic::Shmem::handleHostEvent2( NicShmemCmdEvent* event, int id )
{

	switch( event->type ) {
    case NicShmemCmdEvent::Fadd:
        hostFadd( static_cast<NicShmemFaddCmdEvent*>(event), id );
                //1600 );
		break;
    case NicShmemCmdEvent::Add:
    	hostAdd( static_cast<NicShmemAddCmdEvent*>(event), id );
                //40 );
		break;
    case NicShmemCmdEvent::Put:
        hostPut( static_cast<NicShmemPutCmdEvent*>(event), id );
		break;
    case NicShmemCmdEvent::Putv:
        hostPutv( static_cast<NicShmemPutvCmdEvent*>(event), id );
                //20 );
		break;
    case NicShmemCmdEvent::Get:
        hostGet( static_cast<NicShmemGetCmdEvent*>(event), id );
		break;
    case NicShmemCmdEvent::Getv:
        hostGetv( static_cast<NicShmemGetvCmdEvent*>(event), id );
		break;
	default:
		assert(0);
	}
}
void Nic::Shmem::handleNicEvent( NicShmemCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d `%s` targetNode=%d targetCore=%d\n",
            id, event->getTypeStr().c_str(), event->getNode(), event->getPid() );

    switch (event->type) {
      case NicShmemCmdEvent::Add:
      case NicShmemCmdEvent::Putv:
      case NicShmemCmdEvent::Put:

		incActivePuts(id);
        incPendingPuts(id);
		break;

      case NicShmemCmdEvent::Get:
        incPendingGets(id);
		break;

      case NicShmemCmdEvent::Getv:
      case NicShmemCmdEvent::Fadd:
      case NicShmemCmdEvent::Cswap:
      case NicShmemCmdEvent::Swap:
      case NicShmemCmdEvent::Init:
      case NicShmemCmdEvent::RegMem:
      case NicShmemCmdEvent::Wait:
      case NicShmemCmdEvent::Fence:
		break;

      default:
        assert(0);
    }
    SimTime_t start = m_nic.getCurrentSimTimeNano();
    std::vector<MemOp>* vec = new std::vector<MemOp>;
    vec->push_back( MemOp( -1, 16, MemOp::Op::HostBusWrite,
         [=]() {
            m_dbg.verbosePrefix( prefix(),CALL_INFO_LAMBDA,"handleNicEvent",1,NIC_DBG_SHMEM,"latency=%" PRIu64 "\n",
                            m_nic.getCurrentSimTimeNano() - start);
            handleNicEvent2( event, id );
        }
     ) );

    m_nic.calcHostMemDelay(id, vec, [=]() { });
}

void Nic::Shmem::handleNicEvent2( NicShmemCmdEvent* event, int id )
{
	if ( m_engineBusy ) {
    	m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"busy core=%d %s\n",id,event->getTypeStr().c_str());
		m_cmdQ.push( std::make_pair(event,id) );
	} else {
   		m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"start busy core=%d %s\n",id,event->getTypeStr().c_str());
		m_engineBusy = true;
		m_nic.schedCallback(

			[=](){
    			m_dbg.verbosePrefix( prefix(),CALL_INFO_LAMBDA,"handleNicEvent2",1,NIC_DBG_SHMEM,"ready core=%d\n",id);
				m_engineBusy = false;
				if ( ! m_cmdQ.empty() ) {
					handleNicEvent2( m_cmdQ.front().first, m_cmdQ.front().second );
					m_cmdQ.pop();
				}
			},
			m_nicCmdLatency
		);

		handleNicEvent3( event, id );
	}
}

void Nic::Shmem::handleNicEvent3( NicShmemCmdEvent* event, int id )
{

    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d %s\n",id,event->getTypeStr().c_str());
    switch (event->type) {

    case NicShmemCmdEvent::Init:
        init( static_cast< NicShmemInitCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Fence:
        fence( static_cast< NicShmemFenceCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::RegMem:
        regMem( static_cast< NicShmemRegMemCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Put:
        put( static_cast<NicShmemPutCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Putv:
        putv( static_cast<NicShmemPutvCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Get:
        get( static_cast<NicShmemGetCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Getv:
        getv( static_cast<NicShmemGetvCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Add:
        add( static_cast< NicShmemAddCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Fadd:
        fadd( static_cast< NicShmemFaddCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Cswap:
        if ( event->getNode() == m_nic.getNodeId() )  {
            sameNodeCswap( static_cast< NicShmemCswapCmdEvent*>(event), id );
        } else {
            cswap( static_cast< NicShmemCswapCmdEvent*>(event), id );
        }
        break;
    case NicShmemCmdEvent::Swap:
        if ( event->getNode() == m_nic.getNodeId() )  {
            sameNodeSwap( static_cast< NicShmemSwapCmdEvent*>(event), id );
        } else {
            swap( static_cast< NicShmemSwapCmdEvent*>(event), id );
        }
        break;

    default:
        assert(0);
    }
}

void Nic::Shmem::init( NicShmemInitCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d simVAddr=%" PRIx64 "\n",
            id, event->addr );

	size_t dataSize = Hermes::Value::getLength( Hermes::Value::Long );
	Hermes::Value local1( Hermes::Value::Long, getBacking( id, event->addr, dataSize) );
	Hermes::Value local2( Hermes::Value::Long, getBacking( id, event->addr + dataSize, dataSize) );

	m_pendingPuts[id] = std::make_pair( event->addr, local1 );
	m_pendingGets[id] = std::make_pair( event->addr + dataSize, local2 );

    m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns(), event->callback );

    delete event;
}

void Nic::Shmem::fence( NicShmemFenceCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d\n", id );

	if ( m_activePuts[id].count ) {

		m_activePuts[id].event = event;
	} else {
    	m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns(), event->callback );
    	delete event;
	}
}

void Nic::Shmem::regMem( NicShmemRegMemCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d simVAddr=%" PRIx64 " backing=%p len=%lu\n",
            id, event->addr.getSimVAddr(), event->addr.getBacking(), event->len );

    m_regMem[id].push_back( RegionEntry( event->addr, event->realAddr, event->len) );

    m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns(), event->callback );

    delete event;
}


void Nic::Shmem::put( NicShmemPutCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d targetNode=%d farAddr=%" PRIx64" len=%lu\n",
                            id, event->getNode(), event->getFarAddr(), event->getLength() );

    std::stringstream tmp;
    tmp << m_pendingPuts[id].second;
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"pendingRemoteOps=%s\n",tmp.str().c_str());

    int vn =m_nic.m_shmemPutSmallVN;
    if ( event->getLength() > m_nic.m_shmemPutThresholdLength ) {
        vn = m_nic.m_shmemPutLargeVN;
    }

    NicShmemRespEvent::Callback callback = static_cast< NicShmemPutCmdEvent*>(event)->getCallback();
    ShmemPutSendEntry* entry = new ShmemPutbSendEntry( id, event, getBacking( id, event->getMyAddr(), event->getLength() ), vn,
					[=]() {
                        m_dbg.verbosePrefix( prefix(),CALL_INFO_LAMBDA,"put",1,NIC_DBG_SHMEM,"finished\n");
        			   	m_nic.getVirtNic(id)->notifyShmem( 0, callback );
						decActivePuts(id);
					}
    );

    m_nic.qSendEntry( entry );

}

void Nic::Shmem::putv( NicShmemPutvCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d targetCore=%d far=%" PRIx64" len=%lu\n",
			id, event->getVnic(), event->getFarAddr(), event->getLength() );

    std::stringstream tmp;
    tmp << m_pendingPuts[id].second;
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"pendingRemoteOps=%s\n",tmp.str().c_str());

    int vn = m_nic.m_shmemPutSmallVN;
    if ( event->getLength() > m_nic.m_shmemPutThresholdLength ) {
        vn = m_nic.m_shmemPutLargeVN;
    }

    ShmemPutSendEntry* entry = new ShmemPutvSendEntry( id, event, vn,
		[=]() {
        	m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns() );
			decActivePuts(id);
		}
    );

    m_nic.qSendEntry( entry );
}


void Nic::Shmem::getv( NicShmemGetvCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d far=%" PRIx64" len=%lu\n", id, event->getFarAddr(), event->getLength() );

    NicShmemValueRespEvent::Callback callback = event->callback;
    ShmemGetvSendEntry* entry = new ShmemGetvSendEntry( id, event, m_nic.m_shmemGetReqVN,
            [=](Hermes::Value& value) {
                m_dbg.verbosePrefix( prefix(),CALL_INFO_LAMBDA,"getv",1,NIC_DBG_SHMEM,"finished\n");
                m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns(), callback, value );
            }
    );

    entry->setRespKey(m_nic.genRespKey(entry));

    m_nic.qSendEntry( entry );
}

void Nic::Shmem::get( NicShmemGetCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d far=%" PRIx64" len=%lu\n", id, event->getFarAddr(), event->getLength() );

    std::stringstream tmp;
    tmp << m_pendingGets[id].second;
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"pendingRemoteOps=%s\n",tmp.str().c_str());

    NicShmemRespEvent::Callback callback = event->getCallback();
    ShmemGetbSendEntry* entry = new ShmemGetbSendEntry( id, event, m_nic.m_shmemGetReqVN,
            [=]() {
                m_dbg.verbosePrefix( prefix(),CALL_INFO_LAMBDA,"get",1,NIC_DBG_SHMEM,"finished\n");
                m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns(), callback );
				decPendingGets( id );
            }
    );

    entry->setRespKey(m_nic.genRespKey(entry));

	m_nic.qSendEntry( entry );
}


void Nic::Shmem::add( NicShmemAddCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d targetNode=%d targetCore=%d far=%" PRIx64" len=%lu event=%p\n", id,
			event->getNode(), event->getVnic(), event->getFarAddr(), event->getLength(), event );

    std::stringstream tmp;
    tmp << m_pendingPuts[id].second;
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"pendingRemoteOps=%s\n",tmp.str().c_str());

    int vn = m_nic.m_shmemPutSmallVN;
    if ( event->getLength() > m_nic.m_shmemPutThresholdLength ) {
        vn = m_nic.m_shmemPutLargeVN;
    }

    ShmemAddSendEntry* entry = new ShmemAddSendEntry( id, event, vn,
			[=]() {
           		m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns() );
				decActivePuts(id);
			}
    );

    m_nic.qSendEntry( entry );

}

void Nic::Shmem::fadd( NicShmemFaddCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d far=%" PRIx64" len=%lu\n", id, event->getFarAddr(), event->getLength() );

    NicShmemValueRespEvent::Callback callback = event->callback;
    ShmemFaddSendEntry* entry = new ShmemFaddSendEntry( id, event, m_nic.m_shmemGetReqVN,
            [=](Hermes::Value& value) {
                m_dbg.verbosePrefix( prefix(),CALL_INFO_LAMBDA,"fadd",1,NIC_DBG_SHMEM,"finished\n");
                m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns(), callback, value );
            }
    );

    entry->setRespKey(m_nic.genRespKey(entry));

    m_nic.qSendEntry( entry );
}

void Nic::Shmem::cswap( NicShmemCswapCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core+%d far=%" PRIx64" len=%lu\n", id, event->getFarAddr(), event->getLength() );

    NicShmemValueRespEvent::Callback callback = event->callback;
    ShmemCswapSendEntry* entry = new ShmemCswapSendEntry( id, event, m_nic.m_shmemGetReqVN,
            [=](Hermes::Value& value) {
                m_dbg.verbosePrefix( prefix(),CALL_INFO_LAMBDA,"cswap",1,NIC_DBG_SHMEM,"finished\n");
                m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns(), callback, value );
            }
    );

    entry->setRespKey(m_nic.genRespKey(entry));

    m_nic.qSendEntry( entry );
}

void Nic::Shmem::swap( NicShmemSwapCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d far=%" PRIx64" len=%lu\n", id, event->getFarAddr(), event->getLength() );

    NicShmemValueRespEvent::Callback callback = event->callback;
    ShmemSwapSendEntry* entry = new ShmemSwapSendEntry( id, event, m_nic.m_shmemGetReqVN,
            [=](Hermes::Value& value) {
                m_dbg.verbosePrefix( prefix(),CALL_INFO_LAMBDA,"swap",1,NIC_DBG_SHMEM,"finished\n");
                m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns(), callback, value );
            }
    );

    entry->setRespKey(m_nic.genRespKey(entry));

    m_nic.qSendEntry( entry );
}

void Nic::Shmem::hostWait( NicShmemOpCmdEvent* event, int id )
{
    std::stringstream tmp;
    tmp << event->value;
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d addr=%" PRIx64 " value=%s %s\n", id, event->addr,
			tmp.str().c_str(), WaitOpName(event->op).c_str() );

	Hermes::Vaddr addr = event->addr;
	Callback callback = event->callback;
    Op* op = new WaitOp( event, getBacking( id, event->addr, event->value.getLength() ),
            [=]() {
                m_dbg.verbosePrefix( prefix(),CALL_INFO_LAMBDA,"hostWait",1,NIC_DBG_SHMEM,"core=%d addr=%" PRIx64 " finished\n",id,addr);
                m_nic.getVirtNic(id)->notifyShmem( 0, callback );
            }
        );

    if ( ! op->checkOp( m_dbg, id ) ) {
        m_pendingOps[id].push_back( op);
    } else {
        m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d wait satisfied\n",id);
		m_nic.schedCallback( op->callback() );
        delete op;
    }
}

void Nic::Shmem::hostPut( NicShmemPutCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d\n",id);
	std::vector<MemOp>* vec = new std::vector<MemOp>;

    if ( event->getOp() == Hermes::Shmem::ReduOp::MOVE ) {

   		unsigned char* dest = (unsigned char*) getBacking( event->getVnic(), event->getFarAddr(), event->getLength() );
   		unsigned char* src = (unsigned char*) getBacking( id, event->getMyAddr(), event->getLength() );

		if ( dest && src ) {
    		memcpy( dest, src, event->getLength() );
		}

    	vec->push_back( MemOp( event->getFarAddr(), event->getMyAddr(), event->getLength(), MemOp::Op::HostCopy ));

	} else {
		doReduction( event->getOp(), event->getVnic(), event->getFarAddr(), id, event->getMyAddr(), event->getLength(), event->getDataType(), *vec );
	}

	checkWaitOps( event->getVnic(), event->getFarAddr(), event->getLength() );

    SimTime_t start = m_nic.getCurrentSimTimeNano();

	m_nic.calcHostMemDelay(id, vec,
		[=]() {
            m_dbg.verbosePrefix( prefix(),CALL_INFO_LAMBDA,"hostPut",1,NIC_DBG_SHMEM,"core=%d finished latency=%" PRIu64 "\n",id,
                m_nic.getCurrentSimTimeNano()-start);
    		m_nic.getVirtNic(id)->notifyShmem( 0, event->getCallback() );
    		delete event;
		}
	);
}

void Nic::Shmem::hostPutv( NicShmemPutvCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d\n",id);
    Hermes::Value local( event->getDataType(),
                getBacking( event->getVnic(), event->getFarAddr(), event->getLength() ) );

	std::vector<MemOp>* vec = new std::vector<MemOp>;

    if ( local.getPtr() ) {
        local = event->getValue();
    }

    checkWaitOps( event->getVnic(), event->getFarAddr(), local.getLength() );

   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::HostStore ));

    SimTime_t start = m_nic.getCurrentSimTimeNano();

    // this will add pressure to the memory model
    m_nic.calcHostMemDelay(id, vec, [=](){} );
   	m_nic.getVirtNic(id)->notifyShmem( 0 );
   	delete event;
}

void Nic::Shmem::hostGetv( NicShmemGetvCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d\n",id);
    Hermes::Value local( event->getDataType(),
                getBacking( event->getVnic(), event->getFarAddr(), event->getLength() ) );
	std::vector<MemOp>* vec = new std::vector<MemOp>;

   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::HostLoad ));

	m_nic.calcHostMemDelay(id, vec,
		[=]() {
			Hermes::Value _local = local;
            m_dbg.verbosePrefix( prefix(),CALL_INFO_LAMBDA,"hostGetv",1,NIC_DBG_SHMEM,"core=%d finished\n",id);
    		m_nic.getVirtNic(id)->notifyShmem( 0, event->getCallback(), _local );
    		delete event;
		}
	);

}


void Nic::Shmem::hostGet( NicShmemGetCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d\n",id);
	std::vector<MemOp>* vec =new  std::vector<MemOp>;
    assert( event->getOp() == Hermes::Shmem::ReduOp::MOVE );

    void* src = getBacking( event->getVnic(), event->getFarAddr(), event->getLength() );
    void* dest = getBacking( id, event->getMyAddr(), event->getLength() );
	if ( dest && src ) {
    	memcpy( dest, src, event->getLength() );
	}

   	vec->push_back( MemOp( event->getFarAddr(), event->getMyAddr(), event->getLength(), MemOp::Op::HostCopy ));

	m_nic.calcHostMemDelay(id, vec,
		[=]() {
            m_dbg.verbosePrefix( prefix(),CALL_INFO_LAMBDA,"hostGet",1,NIC_DBG_SHMEM,"core=%d finished\n",id);
    		m_nic.getVirtNic(id)->notifyShmem( 0, event->getCallback() );
    		delete event;
		}
	);

}

void Nic::Shmem::hostAdd( NicShmemAddCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d\n",id);
    Hermes::Value local( event->getDataType(),
                getBacking( event->getVnic(), event->getFarAddr(), event->getLength() ) );

	std::vector<MemOp>* vec = new std::vector<MemOp>;

    if ( local.getPtr() ) {
        local += event->getValue();
	}

    checkWaitOps( event->getVnic(), event->getFarAddr(), local.getLength() );

   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::HostLoad ));
   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::HostStore ));

    SimTime_t start = m_nic.getCurrentSimTimeNano();

    // this will add pressure to the memory model
    m_nic.calcHostMemDelay( id, vec, [=](){} );
    m_nic.getVirtNic(id)->notifyShmem( 0 );
    delete event;
}

void Nic::Shmem::hostFadd( NicShmemFaddCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d\n",id);
    Hermes::Value local( event->getDataType(),
                getBacking( event->getVnic(), event->getFarAddr(), event->getLength() ) );
	std::vector<MemOp>* vec = new std::vector<MemOp>;

   	Hermes::Value save = Hermes::Value( event->getDataType() );

	if ( local.getPtr() ) {

    	save = local;
    	local += event->getValue();
	}

    checkWaitOps( event->getVnic(), event->getFarAddr(), local.getLength() );

   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::HostLoad ));
   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::HostStore ));

	m_nic.calcHostMemDelay( id, vec,
		[=]() {
			Hermes::Value _save = save;
            m_dbg.verbosePrefix( prefix(),CALL_INFO_LAMBDA,"hostFadd",1,NIC_DBG_SHMEM,"core=%d finished\n",id);
    		m_nic.getVirtNic(id)->notifyShmem( 0, event->getCallback(), _save );
    		delete event;

		}
	);
}


void Nic::Shmem::sameNodeCswap( NicShmemCswapCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d\n",id);
    Hermes::Value local( event->getDataType(),
                getBacking( event->getVnic(), event->getFarAddr(), event->getLength() ) );
	std::vector<MemOp>* vec = new std::vector<MemOp>;

    Hermes::Value save = Hermes::Value( event->getDataType() );

	assert( local.getPtr() );

    save = local;

   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::BusLoad ));

    if ( local == event->getCond() ) {
        local = event->getValue();
        checkWaitOps( event->getVnic(), event->getFarAddr(), local.getLength() );
   		vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::BusStore ));
    }

	m_nic.calcNicMemDelay( m_nic.allocNicRecvUnit(id), id, vec,
		[=]() {
			Hermes::Value _save = save;
            m_dbg.verbosePrefix( prefix(),CALL_INFO_LAMBDA,"sameNodeCswap",1,NIC_DBG_SHMEM,"core=%d finished\n",id);
    		m_nic.getVirtNic(id)->notifyShmem( 0, event->getCallback(), _save );
    		delete event;
		}
	);

}

void Nic::Shmem::sameNodeSwap( NicShmemSwapCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d\n",id);
    Hermes::Value local( event->getDataType(),
                getBacking( event->getVnic(), event->getFarAddr(), event->getLength() ) );

    Hermes::Value save = Hermes::Value( event->getDataType() );
	std::vector<MemOp>* vec = new std::vector<MemOp>;

    if ( local.getPtr() ) {
        save = local;
        local = event->getValue();
    }

    checkWaitOps( event->getVnic(), event->getFarAddr(), local.getLength() );

   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::BusLoad ));
   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::BusStore ));

	m_nic.calcNicMemDelay(m_nic.allocNicRecvUnit(id), id, vec,
		[=](){
			Hermes::Value _save = save;
            m_dbg.verbosePrefix( prefix(),CALL_INFO_LAMBDA,"sameNodeSwap",1,NIC_DBG_SHMEM,"core=%d finished\n",id);
    		m_nic.getVirtNic(id)->notifyShmem( 0, event->getCallback(), _save );
    		delete event;
		}
	);

}


void Nic::Shmem::doReduction( Hermes::Shmem::ReduOp op, int destCore, Hermes::Vaddr destAddr,
			int srcCore, Hermes::Vaddr srcAddr, size_t length, Hermes::Value::Type type, std::vector<MemOp>& vec )
{
	unsigned char* destPtr = (unsigned char*) getBacking( destCore, destAddr,  length );
    unsigned char* srcPtr = (unsigned char*) getBacking( srcCore, srcAddr, length );

	int nelems = length / Hermes::Value::getLength(type);
	for ( int i = 0; i < nelems; i++ ) {

		Hermes::Value src = Hermes::Value( type, srcPtr );
		Hermes::Value dest = Hermes::Value( type, destPtr );

   		vec.push_back( MemOp( srcAddr, length, MemOp::Op::HostLoad ));
   		vec.push_back( MemOp( destAddr, length, MemOp::Op::HostStore ));

       switch ( op ) {
          case Hermes::Shmem::AND:
            dest &= src;
            break;
          case Hermes::Shmem::OR:
            dest |= src;
            break;
          case Hermes::Shmem::XOR:
            dest ^= src;
            break;
          case Hermes::Shmem::SUM:
            dest += src;
            break;
          case Hermes::Shmem::PROD:
            dest *= src;
            break;
          case Hermes::Shmem::MIN:
            if ( src < dest ) dest = src;
            break;
          case Hermes::Shmem::MAX:
            if ( src > dest ) dest = src;
            break;
          default:
            assert(0);
        }
		srcPtr += Hermes::Value::getLength(type);
		destPtr += Hermes::Value::getLength(type);
		srcAddr += Hermes::Value::getLength(type);
		destAddr += Hermes::Value::getLength(type);
	}
}

void Nic::Shmem::checkWaitOps( int core, Hermes::Vaddr addr, size_t length )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,3,NIC_DBG_SHMEM,"core=%d addr=%" PRIx64" len=%lu\n", core, addr, length );

    std::list<Op*>::iterator iter = m_pendingOps[core].begin();

    while ( iter != m_pendingOps[core].end() ) {

        m_dbg.verbosePrefix( prefix(),CALL_INFO,3,NIC_DBG_SHMEM,"check op\n" );
        Op* op = *iter;
        if ( op->inRange( addr, length ) && op->checkOp( m_dbg, core ) ) {

        	m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"op valid, notify\n");
			m_nic.schedCallback( op->callback(), m_nic2HostDelay_ns );
            delete op;
            iter = m_pendingOps[core].erase(iter);
        } else {
            ++iter;
        }
    }
}

