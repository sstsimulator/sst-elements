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
#include "c_CmdDriver.hpp"
#include "c_TxnUnit.hpp"
#include "c_CmdPtrPkgEvent.hpp"
#include "c_CmdResEvent.hpp"
#include "c_TokenChgEvent.hpp"

using namespace SST;
using namespace SST::n_CmdDriver;
using namespace SST::n_Bank;

c_CmdDriver::c_CmdDriver(ComponentId_t x_id, Params& x_params) :
	Component(x_id) {

	/*---- LOAD PARAMS ----*/

	//used for reading params
	bool l_found = false;

	//internal queue sizes
	k_cmdDrvBufferQEntries = (uint32_t)x_params.find<uint32_t>("numCmdReqQEntries", 100,
			l_found);
	if (!l_found) {
		std::cout
				<< "CmdDriver:: numCmdReqQEntries value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	// tell the simulator not to end without us
	registerAsPrimaryComponent();
	primaryComponentDoNotEndSim();

	/*---- CONFIGURE LINKS ----*/

	// request-related links
	//// send token chg to txn unit
	m_outCmdDrvReqQTokenChgLink = configureLink(
			"outCmdDrvReqQTokenChg",
			new Event::Handler<c_CmdDriver>(this,
					&c_CmdDriver::handleOutCmdDrvReqQTokenChgEvent));
	//// accept cmd from txn unit
	m_inTxnUnitReqPtrLink = configureLink(
			"inTxnUnitReqPtr",
			new Event::Handler<c_CmdDriver>(this,
					&c_CmdDriver::handleInTxnUnitReqPtrPkgEvent));

	// response-related links
	//// send to txn unit
	m_outCmdDrvResPtrLink = configureLink(
			"outCmdDrvResPtr",
			new Event::Handler<c_CmdDriver>(this,
					&c_CmdDriver::handleOutCmdDrvResPtrEvent));

	//set our clock
	registerClock("1GHz",
			new Clock::Handler<c_CmdDriver>(this, &c_CmdDriver::clockTic));
}

c_CmdDriver::~c_CmdDriver() {
}

c_CmdDriver::c_CmdDriver() :
	Component(-1) {
	// for serialization only
}

bool c_CmdDriver::clockTic(Cycle_t) {
	std::cout << std::endl << std::endl << "CmdDriver:: clock tic" << std::endl;

	m_thisCycleQTknChg = 0;

	// store the current number of entries in the queue, later compute the change
	m_thisCycleQTknChg = m_cmdQ.size();

	sendResponse();

	m_thisCycleQTknChg -= m_cmdQ.size();
	sendTokenChg();

	return false;
}

void c_CmdDriver::handleInTxnUnitReqPtrPkgEvent(SST::Event* ev) {

	// make sure the txn buffer q has at least one empty entry
	// to accept a new txn ptr
	c_CmdPtrPkgEvent* l_cmdReqEventPtr = dynamic_cast<c_CmdPtrPkgEvent*> (ev);
	if (l_cmdReqEventPtr) {

		//print
		std::cout << std::endl << std::endl
				<< "CmdDriver::handleInTxnUnitReqPtrPkgEvent(): Received new cmd req event"
				<< std::endl;
		std::cout << "CmdDriver::handleInTxnUnitReqPtrPkgEvent(): @"
				<< std::dec
				<< Simulation::getSimulation()->getCurrentSimCycle() << " "
				<< __PRETTY_FUNCTION__ << std::endl;
		std::cout
				<< "CmdDriver::handleInTxnUnitReqPtrPkgEvent(): Cmds received: ";

		// make sure the received cmd pkg is not empty
		std::vector<c_BankCommand*> l_cmdBuffer = l_cmdReqEventPtr->m_payload;
		while (l_cmdBuffer.size() != 0) {
			c_BankCommand* l_entry = l_cmdBuffer.back();
			m_cmdQ.push(l_entry);
			l_cmdBuffer.pop_back();

			std::cout << "Cmd " << l_entry->getCommandString() << " Addr:"
					<< l_entry->getAddress() << std::endl;

		}

		delete l_cmdReqEventPtr;
	} else {
		std::cout << std::endl << std::endl << "CmdDriver:: "
				<< __PRETTY_FUNCTION__ << " ERROR:: Bad Event Type!"
				<< std::endl;
	}
}

// dummy event functions
void c_CmdDriver::handleOutCmdDrvResPtrEvent(SST::Event* ev) {
	// nothing to do here
	std::cout << __PRETTY_FUNCTION__ << " ERROR: Should not be here"
			<< std::endl;
}

void c_CmdDriver::handleOutCmdDrvReqQTokenChgEvent(SST::Event* ev) {
	// nothing to do here
	std::cout << __PRETTY_FUNCTION__ << " ERROR: Should not be here"
			<< std::endl;
}

void c_CmdDriver::sendResponse() {
	// make sure that the internal buffer is not empty
	if (m_cmdQ.size() > 0) {

		std::cout << "CmdDriver::sendResponse(): sending cmd:";
		m_cmdQ.front()->print();
		std::cout << std::endl;
		c_CmdResEvent* l_txnResEventPtr = new c_CmdResEvent();
		l_txnResEventPtr->m_payload = m_cmdQ.front();
		m_cmdQ.pop();
		m_outCmdDrvResPtrLink->send(l_txnResEventPtr);

		std::cout << __PRETTY_FUNCTION__ << " : sent response"
					<< std::endl;
	}
}

void c_CmdDriver::sendTokenChg() {
	// only send tokens when space has opened up in queues and
	// there are no negative tokens. token subtraction must be performed
	// in the source component immediately after sending an event
	if (m_thisCycleQTknChg > 0) {

		//send req q token chg
		std::cout << "CmdDriver::sendTokenChg(): sending tokens: "
				<< m_thisCycleQTknChg << std::endl;
		c_TokenChgEvent* l_txnReqQTokenChgEvPtr = new c_TokenChgEvent();
		l_txnReqQTokenChgEvPtr->m_payload = m_thisCycleQTknChg;
		m_outCmdDrvReqQTokenChgLink->send(l_txnReqQTokenChgEvPtr);
	}
}

// Element Libarary / Serialization stuff
