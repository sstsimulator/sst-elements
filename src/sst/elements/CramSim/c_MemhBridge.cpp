// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

//SST includes
#include "sst_config.h"

#include <assert.h>
#include <iostream>


//local includes
#include "c_MemhBridge.hpp"
#include "c_TxnReqEvent.hpp"
#include "c_TxnResEvent.hpp"
#include "c_TokenChgEvent.hpp"
#include "memReqEvent.hpp"

using namespace SST;
using namespace SST::n_Bank;
using namespace CramSim;

c_MemhBridge::c_MemhBridge(ComponentId_t x_id, Params& x_params) :
	Component(x_id) {

	/*---- LOAD PARAMS ----*/

	//used for reading params
	bool l_found = false;

	// internal params
	m_prevAddress = 0;
	m_seqNum = 0;
	m_resReadCount = 0;
	m_resWriteCount = 0;

	//internal queues' sizes
	k_txnGenReqQEntries = (uint32_t)x_params.find<uint32_t>("numTxnGenReqQEntries", 100,
			l_found);

	if (!l_found) {
		std::cout
				<< "TxnGen:: numTxnGenReqQEntries value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	k_txnGenResQEntries = (uint32_t)x_params.find<uint32_t>("numTxnGenResQEntries", 100,
			l_found);
	if (!l_found) {
		std::cout
				<< "TxnGen:: numTxnGenResQEntries value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	//transaction unit queue entries
	k_txnUnitReqQEntries = (uint32_t)x_params.find<uint32_t>("numTxnUnitReqQEntries", 100,
			l_found);
	if (!l_found) {
		std::cout << "TxnGen:: numTxnUnitReqQEntries value is missing... exiting"
				<< std::endl;
		exit(-1);
	}
	m_txnUnitReqQTokens = k_txnUnitReqQEntries;


	// tell the simulator not to end without us
	//registerAsPrimaryComponent();
	//primaryComponentDoNotEndSim();

	/*---- CONFIGURE LINKS ----*/

	// request-related links
	//// send to txn unit
	m_outTxnGenReqPtrLink = configureLink(
			"outTxnGenReqPtr",
			new Event::Handler<c_MemhBridge>(this,
					&c_MemhBridge::handleOutTxnGenReqPtrEvent));
	//// accept token chg from txn unit
	m_inTxnUnitReqQTokenChgLink = configureLink(
			"inTxnUnitReqQTokenChg",
			new Event::Handler<c_MemhBridge>(this,
					&c_MemhBridge::handleInTxnUnitReqQTokenChgEvent));

	// response-related links
	//// accept from txn unit
	m_inTxnUnitResPtrLink = configureLink(
			"inTxnUnitResPtr",
			new Event::Handler<c_MemhBridge>(this,
					&c_MemhBridge::handleInTxnUnitResPtrEvent));
	//// send token chg to txn unit
	m_outTxnGenResQTokenChgLink = configureLink(
			"outTxnGenResQTokenChg",
			new Event::Handler<c_MemhBridge>(this,
					&c_MemhBridge::handleOutTxnGenResQTokenChgEvent));

	// CPU links
	m_linkCPU = configureLink( "linkCPU");

	//set our clock
	registerClock("1GHz",
			new Clock::Handler<c_MemhBridge>(this, &c_MemhBridge::clockTic));
}

c_MemhBridge::~c_MemhBridge() {
}

c_MemhBridge::c_MemhBridge() :
	Component(-1) {
	// for serialization only
}


void c_MemhBridge::createTxn() {
	if (m_txnReqQ.size() < k_txnGenReqQEntries) {

		SST::Event* e = 0;
		if((e = m_linkCPU->recv())) {
			MemReqEvent *event = dynamic_cast<MemReqEvent *>(e);
			dbg.output(CALL_INFO, "req for %llx)\n", event->getAddr());

			//TODO: Default value for dataWidth?
			c_Transaction *mTxn;
			ulong addr = event->getAddr();

			if (event->getIsWrite())
				mTxn = new c_Transaction(event->getReqId(), e_TransactionType::WRITE, addr, 1);
			else
				mTxn = new c_Transaction(event->getReqId(), e_TransactionType::READ, addr, 1);

			m_txnReqQ.push(mTxn);

			m_prevAddress = addr;

	//		std::cout << std::endl << std::endl
	 //		<< "TxnGen::createTxn() created txn seqNum = " << event->getReqId()
	 	//	<< " addr=" << std::hex << addr << std::endl;

		}


	} else {
	//	 std::cout << std::endl << std::endl
	//	 		<< "TxnGen::createTxn() Did not create txn seqNum = "
	//	 		<< event->getReqId() << " txnReqQ is full" << std::endl;
	}
}

bool c_MemhBridge::clockTic(Cycle_t) {
	// std::cout << std::endl << std::endl << "TxnGen::clock tic" << std::endl;

	m_thisCycleResQTknChg = 0;

	// store the current number of entries in the queue, later compute the change
	m_thisCycleResQTknChg = m_txnResQ.size();


	createTxn();
	sendRequest();
	readResponse();

	m_thisCycleResQTknChg -= m_txnResQ.size();
	sendTokenChg();

	return false;

}

void c_MemhBridge::handleInTxnUnitReqQTokenChgEvent(SST::Event *ev) {
	c_TokenChgEvent* l_txnUnitReqQTknChgEventPtr =
			dynamic_cast<c_TokenChgEvent*> (ev);

	if (l_txnUnitReqQTknChgEventPtr) {
		// std::cout << "TxnGen::handleInTxnUnitReqQTokenChgEvent(): @"
		// 		<< std::dec
		// 		<< Simulation::getSimulation()->getCurrentSimCycle() << " "
		// 		<< __PRETTY_FUNCTION__ << std::endl;

		m_txnUnitReqQTokens += l_txnUnitReqQTknChgEventPtr->m_payload;

		//FIXME: Critical: This pointer is left dangling
		//delete l_txnUnitReqQTknChgEventPtr;

		assert(m_txnUnitReqQTokens >= 0);
		assert(m_txnUnitReqQTokens <= k_txnUnitReqQEntries);


	} else {
		std::cout << std::endl << std::endl << "TxnGen:: "
				<< __PRETTY_FUNCTION__ << " ERROR:: Bad event type!"
				<< std::endl;
	}
}

void c_MemhBridge::handleInTxnUnitResPtrEvent(SST::Event* ev) {
	// make sure the txn res q has at least one empty entry
	// to accept a new txn ptr
	assert(1 <= (k_txnGenResQEntries - m_txnResQ.size()));

	c_TxnResEvent* l_txnResEventPtr = dynamic_cast<c_TxnResEvent*> (ev);
	if (l_txnResEventPtr) {
	//	 std::cout << "TxnGen::handleInTxnUnitResPtrEvent(): @" << std::dec
	//	 		<< Simulation::getSimulation()->getCurrentSimCycle() << " "
	//	 		<< __PRETTY_FUNCTION__ << " Txn received: "<< std::endl;
	//	 l_txnResEventPtr->m_payload->print();
	//	 std::cout << std::endl;

		if (l_txnResEventPtr->m_payload->getTransactionMnemonic()
				== e_TransactionType::READ)
			m_resReadCount++;
		else
			m_resWriteCount++;


		m_txnResQ.push(l_txnResEventPtr->m_payload);

		//FIXME: Critical: This pointer is left dangling
		//delete l_txnResEventPtr;
	} else {
		std::cout << std::endl << std::endl << "TxnGen:: "
				<< __PRETTY_FUNCTION__ << " ERROR:: Bad Event Type!"
				<< std::endl;
	}
}

// dummy event functions
void c_MemhBridge::handleOutTxnGenReqPtrEvent(SST::Event* ev) {
	// nothing to do here
	std::cout << __PRETTY_FUNCTION__ << " ERROR: Should not be here"
			<< std::endl;
}

void c_MemhBridge::handleOutTxnGenResQTokenChgEvent(SST::Event* ev) {
	// nothing to do here
	std::cout << __PRETTY_FUNCTION__ << " ERROR: Should not be here"
			<< std::endl;
}

void c_MemhBridge::sendTokenChg() {
	// only send tokens when space has opened up in queues
	// there are no negative tokens. token subtraction must be performed
	// in the source component immediately after sending an event
	if (m_thisCycleResQTknChg > 0) {

		//send res q token chg
		// std::cout << "TxnGen::sendTokenChg(): sending tokens: "
		// 		<< m_thisCycleResQTknChg << std::endl;
		c_TokenChgEvent* l_txnResQTokenChgEvPtr = new c_TokenChgEvent();
		l_txnResQTokenChgEvPtr->m_payload = m_thisCycleResQTknChg;
		m_outTxnGenResQTokenChgLink->send(l_txnResQTokenChgEvPtr);
	}
}

void c_MemhBridge::sendRequest() {

	if (m_txnUnitReqQTokens > 0) {
		if (m_txnReqQ.size() > 0) {

			// std::cout << "TxnGen::sendRequest(): sending Txn=";
			// m_txnReqQ.front()->print();
			// std::cout << std::endl;
			c_TxnReqEvent* l_txnReqEvPtr = new c_TxnReqEvent();
			l_txnReqEvPtr->m_payload = m_txnReqQ.front();
			m_txnReqQ.pop();
			m_outTxnGenReqPtrLink->send(l_txnReqEvPtr);
			--m_txnUnitReqQTokens;

		}
	}
}

void c_MemhBridge::readResponse() {
	if (m_txnResQ.size() > 0) {
		c_Transaction* l_txn = m_txnResQ.front();

		MemRespEvent *event = new MemRespEvent(l_txn->getSeqNum(), l_txn->getAddress(), 0);

		//std::cout << "TxnGen::readResponse() Transaction printed: Addr : "
		//		  << std::hex << l_txn->getAddress() << std::endl;

		m_linkCPU->send( event );
		m_txnResQ.pop();

	} else {
		// std::cout << "TxnGen::readResponse(): No transactions in res q to read"
		// 		<< std::endl;
	}
}

// Element Libarary / Serialization stuff
