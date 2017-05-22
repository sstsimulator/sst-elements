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

//SST includes
#include "sst_config.h"

#include <assert.h>
#include <iostream>


//local includes
#include "c_TxnDriver.hpp"
#include "c_TxnGenSeq.hpp"
#include "c_TxnReqEvent.hpp"
#include "c_TxnResEvent.hpp"
#include "c_TokenChgEvent.hpp"

using namespace SST;
using namespace SST::n_TxnDriver;
using namespace SST::n_Bank;

c_TxnDriver::c_TxnDriver(ComponentId_t x_id, Params& x_params) :
	Component(x_id) {

	/*---- LOAD PARAMS ----*/

	//used for reading params
	bool l_found = false;

	//internal queue sizes
	k_txnDrvBufferQEntries = (uint32_t)x_params.find<uint32_t>("numTxnDrvBufferQEntries",
			100, l_found);
	if (!l_found) {
		std::cout
				<< "TxnDriver:: numTxnDrvBufferQEntries value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	//transaction gen queue entries
	k_txnGenResQEntries = (uint32_t)x_params.find<uint32_t>("numTxnGenResQEntries", 100,
			l_found);
	if (!l_found) {
		std::cout
				<< "TxnDriver:: numTxnGenResQEntries value is missing... exiting"
				<< std::endl;
		exit(-1);
	}
	m_txnGenResQTokens = k_txnGenResQEntries;

	// tell the simulator not to end without us
	registerAsPrimaryComponent();
	primaryComponentDoNotEndSim();

	/*---- CONFIGURE LINKS ----*/

	// request-related links
	//// send token chg to txn gen
	m_outTxnDrvReqQTokenChgLink = configureLink(
			"outTxnDrvReqQTokenChg",
			new Event::Handler<c_TxnDriver>(this,
					&c_TxnDriver::handleOutTxnDrvReqQTokenChgEvent));
	//// accept from txn gen
	m_inTxnGenReqPtrLink = configureLink(
			"inTxnGenReqPtr",
			new Event::Handler<c_TxnDriver>(this,
					&c_TxnDriver::handleInTxnGenReqPtrEvent));

	// response-related links
	//// accept token chg from txn gen
	m_inTxnGenResQTokenChgLink = configureLink(
			"inTxnGenResQTokenChg",
			new Event::Handler<c_TxnDriver>(this,
					&c_TxnDriver::handleInTxnGenResQTokenChgEvent));
	//// send txn to txn gen
	m_outTxnDrvResPtrLink = configureLink(
			"outTxnDrvResPtr",
			new Event::Handler<c_TxnDriver>(this,
					&c_TxnDriver::handleOutTxnDrvResPtrEvent));

	//set our clock
	registerClock("1GHz",
			new Clock::Handler<c_TxnDriver>(this, &c_TxnDriver::clockTic));
}

c_TxnDriver::~c_TxnDriver() {
}

c_TxnDriver::c_TxnDriver() :
	Component(-1) {
	// for serialization only
}

bool c_TxnDriver::clockTic(Cycle_t) {
	std::cout << std::endl << std::endl << "TxnDriver:: clock tic" << std::endl;

	m_thisCycleQTknChg = 0;

	// store the current number of entries in the queue, later compute the change
	m_thisCycleQTknChg = m_txnQ.size();

	sendResponse();

	m_thisCycleQTknChg -= m_txnQ.size();
	sendTokenChg();

	return false;
}

void c_TxnDriver::handleInTxnGenResQTokenChgEvent(SST::Event *ev) {
	c_TokenChgEvent* l_txnGenResQTknChgEventPtr =
			dynamic_cast<c_TokenChgEvent*> (ev);

	if (l_txnGenResQTknChgEventPtr) {
		std::cout << std::endl << std::endl
				<< "TxnDriver::handleInTxnGenResQTokenChgEvent() Received new txn gen res q token chg event"
				<< std::endl;
		std::cout << "TxnDriver::handleInTxnGenResQTokenChgEvent(): @"
				<< std::dec
				<< Simulation::getSimulation()->getCurrentSimCycle() << " "
				<< __PRETTY_FUNCTION__ << std::endl;
		std::cout
				<< "TxnDriver::handleInTxnGenResQTokenChgEvent(): TxnGen Res Token Chg = "
				<< (l_txnGenResQTknChgEventPtr->m_payload);

		m_txnGenResQTokens += l_txnGenResQTknChgEventPtr->m_payload;

		assert(m_txnGenResQTokens >= 0);
		assert(m_txnGenResQTokens <= k_txnGenResQEntries);

		delete l_txnGenResQTknChgEventPtr;
	} else {
		std::cout << std::endl << std::endl << "TxnDriver:: "
				<< __PRETTY_FUNCTION__ << " ERROR:: Bad event type!"
				<< std::endl;
	}
}

void c_TxnDriver::handleInTxnGenReqPtrEvent(SST::Event* ev) {
	// make sure the txn buffer q has at least one empty entry
	// to accept a new txn ptr

	c_TxnReqEvent* l_txnReqEventPtr = dynamic_cast<c_TxnReqEvent*> (ev);
	if (l_txnReqEventPtr) {

		//print
		std::cout << std::endl << std::endl
				<< "TxnDriver::handleInTxnGenReqPtrEvent(): Received new transaction req event"
				<< std::endl;
		std::cout << "TxnDriver::handleInTxnGenReqPtrEvent(): @" << std::dec
				<< Simulation::getSimulation()->getCurrentSimCycle() << " "
				<< __PRETTY_FUNCTION__ << std::endl;
		std::cout << "TxnDriver::handleInTxnGenReqPtrEvent(): Txn received: ";
		l_txnReqEventPtr->m_payload->print();
		std::cout << std::endl;

		m_txnQ.push(l_txnReqEventPtr->m_payload);

		delete l_txnReqEventPtr;
	} else {
		std::cout << std::endl << std::endl << "TxnDriver:: "
				<< __PRETTY_FUNCTION__ << " ERROR:: Bad Event Type!"
				<< std::endl;
	}
}

// dummy event functions
void c_TxnDriver::handleOutTxnDrvResPtrEvent(SST::Event* ev) {
	// nothing to do here
	std::cout << __PRETTY_FUNCTION__ << " ERROR: Should not be here"
			<< std::endl;
}

void c_TxnDriver::handleOutTxnDrvReqQTokenChgEvent(SST::Event* ev) {
	// nothing to do here
	std::cout << __PRETTY_FUNCTION__ << " ERROR: Should not be here"
			<< std::endl;
}

void c_TxnDriver::sendResponse() {

	if (m_txnGenResQTokens > 0) {
		if (m_txnQ.size() > 0) {

			std::cout << "TxnDriver::sendResponse(): sending txn:";
			m_txnQ.front()->print();
			std::cout << std::endl;
			c_TxnResEvent* l_txnResEventPtr = new c_TxnResEvent();
			l_txnResEventPtr->m_payload = m_txnQ.front();
			m_txnQ.pop();
			m_outTxnDrvResPtrLink->send(l_txnResEventPtr);
			--m_txnGenResQTokens;
		}
	}
}

void c_TxnDriver::sendTokenChg() {
	// only send tokens when space has opened up in queues
	// there are no negative tokens. token subtraction must be performed
	// in the source component immediately after sending an event
	if (m_thisCycleQTknChg > 0) {

		//send req q token chg
		std::cout << "TxnDriver::sendTokenChg(): sending tokens: " << m_thisCycleQTknChg
				<< std::endl;
		c_TokenChgEvent* l_txnReqQTokenChgEvPtr = new c_TokenChgEvent();
		l_txnReqQTokenChgEvPtr->m_payload = m_thisCycleQTknChg;
		m_outTxnDrvReqQTokenChgLink->send(l_txnReqQTokenChgEvPtr);
	}
}

// Element Libarary / Serialization stuff
