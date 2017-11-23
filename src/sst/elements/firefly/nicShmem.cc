// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"node=%d core=%d type=%d\n", event->getNode(), id, event->type ); 

    if( event->getNode() == m_nic.getNodeId() || -2 == event->getNode() ) {
		handleHostEvent( event, id );
	} else {
		handleNicEvent( event, id );
	}
}

void Nic::Shmem::handleNicEvent( NicShmemCmdEvent* event, int id )
{
	if( m_freeCmdSlots == 0 ) {
		m_pendingCmds.push_back( std::make_pair(event,id ) );
		return;
	}

    switch (event->type) {
      case NicShmemCmdEvent::Add:
		--m_freeCmdSlots;	
        m_nic.getVirtNic(id)->notifyShmem( 0, static_cast< NicShmemAddCmdEvent*>(event)->getCallback() );
		break;

      case NicShmemCmdEvent::Putv:
		--m_freeCmdSlots;	
        m_nic.getVirtNic(id)->notifyShmem( 0, static_cast< NicShmemPutvCmdEvent*>(event)->getCallback() );
		break;

      case NicShmemCmdEvent::Put:
      case NicShmemCmdEvent::Init:
      case NicShmemCmdEvent::Get:
      case NicShmemCmdEvent::Getv:
      case NicShmemCmdEvent::RegMem:
      case NicShmemCmdEvent::Wait:
      case NicShmemCmdEvent::Fadd:
      case NicShmemCmdEvent::Cswap:
      case NicShmemCmdEvent::Swap:
		break;

      default:
        assert(0);
    }
	m_nic.schedEvent(  new SelfEvent( event, id ), getHost2NicDelay_ns() );
}

void Nic::Shmem::handleEvent2( NicShmemCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"core=%d\n",id ); 
    switch (event->type) {

    case NicShmemCmdEvent::Add:
        add( static_cast< NicShmemAddCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Put:
        put( static_cast<NicShmemPutCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Putv:
        putv( static_cast<NicShmemPutvCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Init:
        init( static_cast< NicShmemInitCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Get:
        get( static_cast<NicShmemGetCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Getv:
        getv( static_cast<NicShmemGetvCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::RegMem:
        regMem( static_cast< NicShmemRegMemCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Fadd:
        fadd( static_cast< NicShmemFaddCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Cswap:
        cswap( static_cast< NicShmemCswapCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Swap:
        swap( static_cast< NicShmemSwapCmdEvent*>(event), id );
        break;

    default:
        assert(0);
    }
}

void Nic::Shmem::handleHostEvent( NicShmemCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"core=%d\n",id ); 
    switch (event->type) {

    case NicShmemCmdEvent::Wait:
        hostWait( static_cast< NicShmemOpCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Add:
        hostAdd( static_cast< NicShmemAddCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Put:
        hostPut( static_cast<NicShmemPutCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Putv:
        hostPutv( static_cast<NicShmemPutvCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Get:
        hostGet( static_cast<NicShmemGetCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Getv:
        hostGetv( static_cast<NicShmemGetvCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Fadd:
        hostFadd( static_cast< NicShmemFaddCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Cswap:
        hostCswap( static_cast< NicShmemCswapCmdEvent*>(event), id );
        break;
    case NicShmemCmdEvent::Swap:
        hostSwap( static_cast< NicShmemSwapCmdEvent*>(event), id );
        break;

    default:
        assert(0);
    }
}

void Nic::Shmem::init( NicShmemInitCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"core=%d simVAddr=%" PRIx64 "\n", 
            id, event->addr );

	Hermes::Value local( Hermes::Value::Long, getBacking( id, event->addr, Hermes::Value::getLength( Hermes::Value::Long ) ) );
	m_pendingRemoteOps[id] = std::make_pair( event->addr, local );

    m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns(), event->callback );

    delete event;
}
void Nic::Shmem::regMem( NicShmemRegMemCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"core=%d simVAddr=%" PRIx64 " backing=%p len=%lu\n", 
            id, event->addr.getSimVAddr(), event->addr.getBacking(), event->len );

    m_regMem[id].push_back( std::make_pair(event->addr, event->len) );

    m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns(), event->callback );
    delete event;
}


void Nic::Shmem::put( NicShmemPutCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"core=%d far=%" PRIx64" len=%lu\n", id, event->getFarAddr(), event->getLength() );

    m_pendingRemoteOps[id].second += m_one;

    std::stringstream tmp;
    tmp << m_pendingRemoteOps[id].second;
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"Nic::Shmem:: pendingRemoteOps=%s\n",tmp.str().c_str());

    ShmemPutSendEntry* entry = new ShmemPutbSendEntry( id, event, getBacking( id, event->getMyAddr(), event->getLength() ),
					[=]() {
                        m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"Nic::Shmem::put complete\n");
        				m_nic.getVirtNic(id)->notifyShmem( 0, static_cast< NicShmemPutCmdEvent*>(event)->getCallback() );
					}
    );

    m_nic.m_sendMachine[0]->run( entry );
}

void Nic::Shmem::putv( NicShmemPutvCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"core=%d farCore=%d far=%" PRIx64" len=%lu\n", 
			id, event->getVnic(), event->getFarAddr(), event->getLength() );

    m_pendingRemoteOps[id].second += m_one;

    std::stringstream tmp;
    tmp << m_pendingRemoteOps[id].second;
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"Nic::Shmem:: pendingRemoteOps=%s\n",tmp.str().c_str());

    ShmemPutSendEntry* entry = new ShmemPutvSendEntry( id, event, 
					[=]() {
						incFreeCmdSlots();
					} 
    );

    m_nic.m_sendMachine[0]->run( entry );
}


void Nic::Shmem::getv( NicShmemGetvCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"core+%d far=%" PRIx64" len=%lu\n", id, event->getFarAddr(), event->getLength() );

    ShmemGetvSendEntry* entry = new ShmemGetvSendEntry( id, event, 
            [=](Hermes::Value& value) {
                m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"Nic::Shmem::getv complete\n");
                m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns(), event->callback, value );
            } 
    ); 

    m_nic.m_sendMachine[0]->run( entry );
}

void Nic::Shmem::get( NicShmemGetCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"core=%d far=%" PRIx64" len=%lu\n", id, event->getFarAddr(), event->getLength() );

    ShmemGetbSendEntry* entry = new ShmemGetbSendEntry( id, event, 
            [=]() {
                m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"Nic::Shmem::getv complete\n");
                m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns(), event->getCallback() );
            } 
    );  

	m_nic.m_sendMachine[0]->run( entry );
}


void Nic::Shmem::add( NicShmemAddCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"core=%d farCore=%d far=%" PRIx64" len=%lu\n", id, event->getVnic(), event->getFarAddr(), event->getLength() );

    m_pendingRemoteOps[id].second += m_one;

    std::stringstream tmp;
    tmp << m_pendingRemoteOps[id].second;
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"Nic::Shmem:: pendingRemoteOps=%s\n",tmp.str().c_str());

    ShmemAddSendEntry* entry = new ShmemAddSendEntry( id, event,
					[=]() {
						incFreeCmdSlots();
					} 
    ); 

    m_nic.m_sendMachine[0]->run( entry );
}

void Nic::Shmem::fadd( NicShmemFaddCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"core=%d far=%" PRIx64" len=%lu\n", id, event->getFarAddr(), event->getLength() );

    ShmemFaddSendEntry* entry = new ShmemFaddSendEntry( id, event, 
            [=](Hermes::Value& value) {
                m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"Nic::Shmem::fadd complete\n");
                m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns(), event->callback, value );
            } 
    ); 

    m_nic.m_sendMachine[0]->run( entry );
}

void Nic::Shmem::cswap( NicShmemCswapCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"core+%d far=%" PRIx64" len=%lu\n", id, event->getFarAddr(), event->getLength() );

    ShmemCswapSendEntry* entry = new ShmemCswapSendEntry( id, event, 
            [=](Hermes::Value& value) {
                m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"Nic::Shmem::cswap complete\n");
                m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns(), event->callback, value );
            } 
    ); 

    m_nic.m_sendMachine[0]->run( entry );
}

void Nic::Shmem::swap( NicShmemSwapCmdEvent* event, int id )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"core=%d far=%" PRIx64" len=%lu\n", id, event->getFarAddr(), event->getLength() );

    ShmemSwapSendEntry* entry = new ShmemSwapSendEntry( id, event, 
            [=](Hermes::Value& value) {
                m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"Nic::Shmem::swap complete\n");
                m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns(), event->callback, value );
            } 
    ); 

    m_nic.m_sendMachine[0]->run( entry );
}

void Nic::Shmem::hostWait( NicShmemOpCmdEvent* event, int id )
{
    std::stringstream tmp;
    tmp << event->value;
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"core=%d addr=%" PRIx64 " value=%s op=%d\n", id, event->addr, tmp.str().c_str(), event->op );

    Op* op = new WaitOp(  event, getBacking( id, event->addr, event->value.getLength() ),
            [=]() {
				//std::vector<MemOp> vec;
                m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"Nic::Shmem::wait complete\n");
                m_nic.getVirtNic(id)->notifyShmem( 0, event->callback );
                //m_nic.getVirtNic(id)->notifyShmem( m_nic.calcHostMemDelay(vec), event->callback );
            } 
        );

    if ( ! op->checkOp( ) ) { 
        m_pendingOps[id].push_back( op); 
    } else {
   		m_nic.getVirtNic(id)->notifyShmem( 0, op->callback() );
        delete op; 
    }
}

void Nic::Shmem::hostPut( NicShmemPutCmdEvent* event, int id )
{
	std::vector<MemOp>* vec = new std::vector<MemOp>;

    if ( event->getOp() == Hermes::Shmem::ReduOp::MOVE ) {

   		unsigned char* dest = (unsigned char*) getBacking( event->getVnic(), event->getFarAddr(), event->getLength() );
   		unsigned char* src = (unsigned char*) getBacking( id, event->getMyAddr(), event->getLength() );

    	memcpy( dest, src, event->getLength() );

    	vec->push_back( MemOp( event->getFarAddr(), event->getMyAddr(), event->getLength(), MemOp::Op::HostCopy ));

	} else {
		doReduction( event->getOp(), event->getVnic(), event->getFarAddr(), id, event->getMyAddr(), event->getLength(), event->getDataType(), *vec );
	}

	checkWaitOps( event->getVnic(), event->getFarAddr(), event->getLength() );

	m_nic.calcHostMemDelay(id, vec, 
		[=]() {
    		m_nic.getVirtNic(id)->notifyShmem( 0, event->getCallback() ); 
    		delete event;
		}
	);
}

void Nic::Shmem::hostPutv( NicShmemPutvCmdEvent* event, int id )
{
    Hermes::Value local( event->getDataType(), 
                getBacking( event->getVnic(), event->getFarAddr(), event->getLength() ) );

	std::vector<MemOp>* vec = new std::vector<MemOp>;

    local = event->getValue();

    checkWaitOps( event->getVnic(), event->getFarAddr(), local.getLength() );

   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::HostStore ));

	m_nic.calcHostMemDelay(id, vec, 
		[=]() {	
    		m_nic.getVirtNic(id)->notifyShmem( 0, event->getCallback() );
    		delete event;
		}
	);

}

void Nic::Shmem::hostGetv( NicShmemGetvCmdEvent* event, int id )
{
    Hermes::Value local( event->getDataType(), 
                getBacking( event->getVnic(), event->getFarAddr(), event->getLength() ) );
	std::vector<MemOp>* vec = new std::vector<MemOp>;

   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::HostLoad ));

	m_nic.calcHostMemDelay(id, vec, 
		[=]() {
			Hermes::Value _local = local;
    		m_nic.getVirtNic(id)->notifyShmem( 0, event->getCallback(), _local );
    		delete event;
		}
	);

}


void Nic::Shmem::hostGet( NicShmemGetCmdEvent* event, int id )
{
	std::vector<MemOp>* vec =new  std::vector<MemOp>;
    assert( event->getOp() == Hermes::Shmem::ReduOp::MOVE );

    void* src = getBacking( event->getVnic(), event->getFarAddr(), event->getLength() );
    void* dest = getBacking( id, event->getMyAddr(), event->getLength() );
    memcpy( dest, src, event->getLength() );

   	vec->push_back( MemOp( event->getFarAddr(), event->getMyAddr(), event->getLength(), MemOp::Op::HostCopy ));

	m_nic.calcHostMemDelay(id, vec,
		[=]() {
    		m_nic.getVirtNic(id)->notifyShmem( 0, event->getCallback() );
    		delete event;
		}
	);

}

void Nic::Shmem::hostAdd( NicShmemAddCmdEvent* event, int id )
{
    Hermes::Value local( event->getDataType(), 
                getBacking( event->getVnic(), event->getFarAddr(), event->getLength() ) );

	std::vector<MemOp>* vec = new std::vector<MemOp>;

    local += event->getValue();

    checkWaitOps( event->getVnic(), event->getFarAddr(), local.getLength() );

   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::HostLoad ));
   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::HostStore ));

	m_nic.calcHostMemDelay(id, vec, 
		[=]() {
    		m_nic.getVirtNic(id)->notifyShmem( 0, event->getCallback() );
    		delete event;
		}
	);

}

void Nic::Shmem::hostFadd( NicShmemFaddCmdEvent* event, int id )
{
    Hermes::Value local( event->getDataType(), 
                getBacking( event->getVnic(), event->getFarAddr(), event->getLength() ) );
	std::vector<MemOp>* vec = new std::vector<MemOp>;

    Hermes::Value save = Hermes::Value( event->getDataType() ); 

    save = local;

    local += event->getValue();

    checkWaitOps( event->getVnic(), event->getFarAddr(), local.getLength() );

   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::HostLoad ));
   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::HostStore ));

	m_nic.calcHostMemDelay(id, vec, 
		[=]() {
			Hermes::Value _save = save;
    		m_nic.getVirtNic(id)->notifyShmem( 0, event->getCallback(), _save );
    		delete event;
		}
	);

}


void Nic::Shmem::hostCswap( NicShmemCswapCmdEvent* event, int id )
{
    Hermes::Value local( event->getDataType(), 
                getBacking( event->getVnic(), event->getFarAddr(), event->getLength() ) );
	std::vector<MemOp>* vec = new std::vector<MemOp>;

    Hermes::Value save = Hermes::Value( event->getDataType() ); 

    save = local;

   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::HostLoad ));

    if ( local == event->getCond() ) {
        local = event->getValue();
        checkWaitOps( event->getVnic(), event->getFarAddr(), local.getLength() );
   		vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::HostStore ));
    }

	m_nic.calcHostMemDelay(id, vec, 
		[=]() {
			Hermes::Value _save = save;
    		m_nic.getVirtNic(id)->notifyShmem( 0, event->getCallback(), _save );
    		delete event;
		}
	);

}

void Nic::Shmem::hostSwap( NicShmemSwapCmdEvent* event, int id )
{
    Hermes::Value local( event->getDataType(), 
                getBacking( event->getVnic(), event->getFarAddr(), event->getLength() ) );

    Hermes::Value save = Hermes::Value( event->getDataType() ); 
	std::vector<MemOp>* vec = new std::vector<MemOp>;

    save = local;

    local = event->getValue();

    checkWaitOps( event->getVnic(), event->getFarAddr(), local.getLength() );

   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::HostLoad ));
   	vec->push_back( MemOp( event->getFarAddr(), event->getLength(), MemOp::Op::HostStore ));


	m_nic.calcHostMemDelay(id, vec, 
		[=](){
			Hermes::Value _save = save;
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

void Nic::Shmem::checkWaitOps( int core, Hermes::Vaddr addr, size_t length, bool nic )
{
    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"core=%d addr=%" PRIx64" len=%lu\n", core, addr, length );

    std::list<Op*>::iterator iter = m_pendingOps[core].begin();

    while ( iter != m_pendingOps[core].end() ) {

        m_dbg.verbosePrefix( prefix(),CALL_INFO,1,1,"check op\n" );
        Op* op = *iter;
        if ( op->inRange( addr, length ) && op->checkOp() ) {
			
			SimTime_t delay = 0;		
			if ( nic ) {
				// this is the nic-to-host delay when an operation in the nic triggers
				// a wait, what should the delay be?
 				delay = 50;
			}
    		m_nic.getVirtNic(core)->notifyShmem( delay, op->callback() );
            delete op; 
            iter = m_pendingOps[core].erase(iter);
        } else {
            ++iter;
        }
    }
}

