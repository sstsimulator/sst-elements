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

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <stdlib.h>

#include <sst/core/stringize.h>

//local includes
#include "c_DramSimTraceReader.hpp"
#include "c_TxnReqEvent.hpp"
#include "c_TxnResEvent.hpp"
#include "c_TokenChgEvent.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_DramSimTraceReader::c_DramSimTraceReader(ComponentId_t x_id, Params& x_params) :
		Component(x_id) {

	/*---- LOAD PARAMS ----*/

	//used for reading params
	bool l_found = false;

	// internal params
	m_seqNum = 0;
	m_reqReadCount = 0;
	m_reqWriteCount = 0;
	m_resReadCount = 0;
	m_resWriteCount = 0;

	//internal queues' sizes
	k_txnGenReqQEntries = (uint32_t)x_params.find<uint32_t>("numTxnGenReqQEntries", 100,
			l_found);
	if (!l_found) {
		std::cout << "TxnGen:: numTxnGenReqQEntries value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	k_txnGenResQEntries = (uint32_t)x_params.find<uint32_t>("numTxnGenResQEntries", 100,
			l_found);
	if (!l_found) {
		std::cout << "TxnGen:: numTxnGenResQEntries value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	//transaction unit queue entries
	k_txnUnitReqQEntries = (uint32_t)x_params.find<uint32_t>("numTxnUnitReqQEntries", 100,
			l_found);
	if (!l_found) {
		std::cout
				<< "TxnGen:: numTxnUnitReqQEntries value is missing... exiting"
				<< std::endl;
		exit(-1);
	}
	m_txnUnitReqQTokens = k_txnUnitReqQEntries;

	// trace file param
	m_traceFileName = x_params.find<std::string>("traceFile", "nil", l_found);
	if (!l_found) {
		std::cout << "TxnGen:: traceFile name is missing... exiting"
				<< std::endl;
		exit(-1);
	}
	m_traceFileStream = new std::ifstream(m_traceFileName, std::ifstream::in);
	if(!(*m_traceFileStream)) {
	  std::cerr << "Unable to open trace file " << m_traceFileName << " Aborting!" << std::endl;
	  exit(-1);
	}
	

	m_statsReqQ = new unsigned[k_txnGenReqQEntries + 1];
	m_statsResQ = new unsigned[k_txnGenResQEntries + 1];

	for (unsigned l_i = 0; l_i != k_txnGenReqQEntries + 1; ++l_i)
		m_statsReqQ[l_i] = 0;

	for (unsigned l_i = 0; l_i != k_txnGenResQEntries + 1; ++l_i)
		m_statsResQ[l_i] = 0;

	// tell the simulator not to end without us
	registerAsPrimaryComponent();
	primaryComponentDoNotEndSim();

	/*---- CONFIGURE LINKS ----*/

	// request-related links
	//// send to txn unit
	m_outTxnGenReqPtrLink = configureLink("outTxnGenReqPtr",
			new Event::Handler<c_DramSimTraceReader>(this,
					&c_DramSimTraceReader::handleOutTxnGenReqPtrEvent));
	//// accept token chg from txn unit
	m_inTxnUnitReqQTokenChgLink = configureLink("inTxnUnitReqQTokenChg",
			new Event::Handler<c_DramSimTraceReader>(this,
					&c_DramSimTraceReader::handleInTxnUnitReqQTokenChgEvent));

	// response-related links
	//// accept from txn unit
	m_inTxnUnitResPtrLink = configureLink("inTxnUnitResPtr",
			new Event::Handler<c_DramSimTraceReader>(this,
					&c_DramSimTraceReader::handleInTxnUnitResPtrEvent));
	//// send token chg to txn unit
	m_outTxnGenResQTokenChgLink = configureLink("outTxnGenResQTokenChg",
			new Event::Handler<c_DramSimTraceReader>(this,
					&c_DramSimTraceReader::handleOutTxnGenResQTokenChgEvent));

	//set our clock
	registerClock("1GHz",
			new Clock::Handler<c_DramSimTraceReader>(this,
					&c_DramSimTraceReader::clockTic));

	// Statistics
	s_readTxnsCompleted = registerStatistic<uint64_t>("readTxnsCompleted");
	s_writeTxnsCompleted = registerStatistic<uint64_t>("writeTxnsCompleted");
}

c_DramSimTraceReader::~c_DramSimTraceReader() {
  delete m_traceFileStream;
}

c_DramSimTraceReader::c_DramSimTraceReader() :
		Component(-1) {
	// for serialization only
}

bool c_DramSimTraceReader::clockTic(Cycle_t) {
	//std::cout << std::endl << std::endl << "TxnGen::clock tic" << std::endl;

	m_thisCycleResQTknChg = 0;

	// store the current number of entries in the queue, later compute the change
	m_thisCycleResQTknChg = m_txnResQ.size();

	//FIXME: Delete. For debugging queue size issues
	m_statsReqQ[m_txnReqQ.size()]++;
	m_statsResQ[m_txnResQ.size()]++;

	createTxn();
	sendRequest();
	readResponse();

	m_thisCycleResQTknChg -= m_txnResQ.size();
	sendTokenChg();

	return false;

}

c_Transaction* c_DramSimTraceReader::getNextTransaction(std::string x_txnType,
		ulong x_addr, unsigned x_dataWidth) {
	c_Transaction* l_txn = new c_Transaction(m_seqNum,
	       m_stringToTxnTypeMap.at(x_txnType), x_addr, x_dataWidth);
	m_seqNum++;
	return l_txn;
}

void c_DramSimTraceReader::createTxn() {
	// check if txn can fit inside Req q
	if (m_txnReqQ.size() < k_txnGenReqQEntries) {
		std::string l_line;
		if (std::getline(*m_traceFileStream, l_line)) {
			char_delimiter sep(" ");
			Tokenizer<> l_tok(l_line, sep);
			unsigned l_numTokens = std::distance(l_tok.begin(), l_tok.end());
			unsigned l_tokNum = 0;
			unsigned l_txnInterval = 0;
			std::string l_txnType;
			ulong    l_txnAddress = 0;
			unsigned l_txnDataWidth = 0;

			for (Tokenizer<>::iterator l_iter =
					l_tok.begin(); l_iter != l_tok.end(); ++l_iter) {
				switch (l_tokNum) {
				case 0:
					l_txnAddress = (ulong) strtoul((*l_iter).c_str(), NULL,
							0);
					break;
				case 1:
					if ((*l_iter).find("WR") != std::string::npos)
						l_txnType = "WRITE";
					else
						l_txnType = "READ";
					break;
				case 2:
					l_txnInterval = std::atoi((*l_iter).c_str());
					break;
				default:
					std::cout
							<< "TxnGen: Should not be in this stage of switch statment"
							<< std::endl;
					exit(-1);
					break;
				}
				++l_tokNum;
			}
			std::transform(l_txnType.begin(), l_txnType.end(),
					l_txnType.begin(), ::toupper);

			c_Transaction* l_txn = getNextTransaction(l_txnType, l_txnAddress,
					1);
			std::pair<c_Transaction*, unsigned> l_entry = std::make_pair(l_txn,
					l_txnInterval);
			m_txnReqQ.push(l_entry);

			// std::cout << std::endl << std::endl
			// 		<< "TxnGen::createTxn() created txn seqNum = " << m_seqNum
			// 		<< " addr=" << l_txnAddress << std::endl;
		} else {
			//  std::cout << "TxnGen:: Ran out of txn's to read" << std::endl;
		}
	} else {
		// std::cout << std::endl << std::endl
		// 		<< "TxnGen::createTxn() Did not create txn seqNum = "
		// 		<< m_seqNum << " txnReqQ is full" << std::endl;
	}
}

void c_DramSimTraceReader::handleInTxnUnitReqQTokenChgEvent(SST::Event *ev) {
	c_TokenChgEvent* l_txnUnitReqQTknChgEventPtr =
			dynamic_cast<c_TokenChgEvent*>(ev);

	if (l_txnUnitReqQTknChgEventPtr) {
//		 std::cout << "TxnGen::handleInTxnUnitReqQTokenChgEvent(): @"
//		 		<< std::dec
//		 		<< Simulation::getSimulation()->getCurrentSimCycle() << " "
//		 		<< __PRETTY_FUNCTION__
//		 		<< " "
//		 		<< l_txnUnitReqQTknChgEventPtr
//		 		<< " "
//		 		<< l_txnUnitReqQTknChgEventPtr->m_payload
//		 		<< std::endl;

		m_txnUnitReqQTokens += l_txnUnitReqQTknChgEventPtr->m_payload;

		//FIXME: Critical: This pointer is left dangling
		delete l_txnUnitReqQTknChgEventPtr;

		assert(m_txnUnitReqQTokens >= 0);
		assert(m_txnUnitReqQTokens <= k_txnUnitReqQEntries);

	} else {
		std::cout << std::endl << std::endl << "TxnGen:: "
				<< __PRETTY_FUNCTION__ << " ERROR:: Bad event type!"
				<< std::endl;
	}
}

void c_DramSimTraceReader::handleInTxnUnitResPtrEvent(SST::Event* ev) {
	// make sure the txn res q has at least one empty entry
	// to accept a new txn ptr
	assert(1 <= (k_txnGenResQEntries - m_txnResQ.size()));

	c_TxnResEvent* l_txnResEventPtr = dynamic_cast<c_TxnResEvent*>(ev);
	if (l_txnResEventPtr) {

//		 std::cout << "TxnGen::handleInTxnUnitResPtrEvent(): @" << std::dec
//		 		<< Simulation::getSimulation()->getCurrentSimCycle() << " "
//		 		<< __PRETTY_FUNCTION__ << " Txn received: "<< std::endl;
//		 l_txnResEventPtr->m_payload->print();
//		 std::cout << std::endl;

		if (l_txnResEventPtr->m_payload->getTransactionMnemonic()
		    == e_TransactionType::READ) {
		  s_readTxnsCompleted->addData(1);
		  m_resReadCount++;
		} else {
		  s_writeTxnsCompleted->addData(1);
		  m_resWriteCount++;
		}

		m_txnResQ.push(l_txnResEventPtr->m_payload);

		//FIXME: Critical: This pointer is left dangling
		delete l_txnResEventPtr;
	} else {
		std::cout << std::endl << std::endl << "TxnGen:: "
				<< __PRETTY_FUNCTION__ << " ERROR:: Bad Event Type!"
				<< std::endl;
	}
}

// dummy event functions
void c_DramSimTraceReader::handleOutTxnGenReqPtrEvent(SST::Event* ev) {
	// nothing to do here
	std::cout << __PRETTY_FUNCTION__ << " ERROR: Should not be here"
			<< std::endl;
}

void c_DramSimTraceReader::handleOutTxnGenResQTokenChgEvent(SST::Event* ev) {
	// nothing to do here
	std::cout << __PRETTY_FUNCTION__ << " ERROR: Should not be here"
			<< std::endl;
}

void c_DramSimTraceReader::sendTokenChg() {
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

void c_DramSimTraceReader::sendRequest() {

	if (m_txnUnitReqQTokens > 0) {
		if (m_txnReqQ.size() > 0) {

			// confirm that interval timer has run out before contiuing
			if (m_txnReqQ.front().second
					>= Simulation::getSimulation()->getCurrentSimCycle()) {
				// std::cout << __PRETTY_FUNCTION__ << ": Interval timer for front txn is not done"
				// << std::endl;
				// m_txnReqQ.front().first->print();
				// std::cout << " - Interval:" << m_txnReqQ.front().second << std::endl;
				return;
			}

			//std::cout << "TxnGen::sendRequest(): sending Txn=" << m_txnReqQ.front().first << " ";
			//m_txnReqQ.front().first->print();
			//std::cout << std::endl;

			if (m_txnReqQ.front().first->getTransactionMnemonic()
					== e_TransactionType::READ)
				m_reqReadCount++;
			else
				m_reqWriteCount++;

			c_TxnReqEvent* l_txnReqEvPtr = new c_TxnReqEvent();
			l_txnReqEvPtr->m_payload = m_txnReqQ.front().first;
			m_txnReqQ.pop();
			m_outTxnGenReqPtrLink->send(l_txnReqEvPtr);
			--m_txnUnitReqQTokens;
		}
	}
}

void c_DramSimTraceReader::readResponse() {
	if (m_txnResQ.size() > 0) {
		c_Transaction* l_txn = m_txnResQ.front();
		m_txnResQ.pop();
		// std::cout << "TxnGen::readResponse() Transaction printed: Addr-"
		// 		<< l_txn->getAddress() << std::endl;
	} else {
		// std::cout << "TxnGen::readResponse(): No transactions in res q to read"
		// 		<< std::endl;
	}
}

// Element Libarary / Serialization stuff
