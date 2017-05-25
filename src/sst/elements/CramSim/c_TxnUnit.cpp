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

/*
 * c_TxnUnit.cpp
 *
 *  Created on: May 18, 2016
 *      Author: tkarkha
 */

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//   http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//SST includes
#include "sst_config.h"

// std includes
#include <iostream>
#include <assert.h>

// local includes
#include "c_TxnUnit.hpp"
#include "c_TxnReqEvent.hpp"
#include "c_TxnResEvent.hpp"
#include "c_TokenChgEvent.hpp"
#include "c_CmdPtrPkgEvent.hpp"
#include "c_CmdResEvent.hpp"
#include "c_TransactionToCommands.hpp"
#include "c_AddressHasher.hpp"

using namespace SST;
using namespace SST::n_Bank;
using namespace std;

c_TxnUnit::c_TxnUnit(SST::ComponentId_t x_id, SST::Params& x_params) :
		Component(x_id) {
	// read params here
	bool l_found = false;

	// load internal params
	k_txnReqQEntries = (uint32_t)x_params.find<uint32_t>("numTxnUnitReqQEntries", 100,
			l_found);
	if (!l_found) {
		std::cout << "numTxnUnitReqQEntries param value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	k_txnResQEntries = (uint32_t)x_params.find<uint32_t>("numTxnUnitResQEntries", 100,
			l_found);
	if (!l_found) {
		std::cout << "numTxnUnitResQEntries param value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	k_numBytesPerTransaction = (uint32_t)x_params.find<uint32_t>("numBytesPerTransaction",
			32, l_found);
	if (!l_found) {
		std::cout << "numBytesPerTransaction value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	k_numChannelsPerDimm = (uint32_t)x_params.find<uint32_t>("numChannelsPerDimm", 1,
			l_found);
	if (!l_found) {
		std::cout << "numChannelsPerDimm value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	k_numRanksPerChannel = (uint32_t)x_params.find<uint32_t>("numRanksPerChannel", 2,
			l_found);
	if (!l_found) {
		std::cout << "numRanksPerChannel value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	k_numBankGroupsPerRank = (uint32_t)x_params.find<uint32_t>("numBankGroupsPerRank", 100,
			l_found);
	if (!l_found) {
		std::cout << "numBankGroupsPerRank value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	k_numBanksPerBankGroup = (uint32_t)x_params.find<uint32_t>("numBanksPerBankGroup", 100,
			l_found);
	if (!l_found) {
		std::cout << "numBanksPerBankGroup value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	k_numColsPerBank = (uint32_t)x_params.find<uint32_t>("numColsPerBank", 100, l_found);
	if (!l_found) {
		std::cout << "numColsPerBank value is missing... exiting" << std::endl;
		exit(-1);
	}

	k_numRowsPerBank = (uint32_t)x_params.find<uint32_t>("numRowsPerBank", 100, l_found);
	if (!l_found) {
		std::cout << "numRowsPerBank value is missing... exiting" << std::endl;
		exit(-1);
	}

	k_relCommandWidth = (uint32_t)x_params.find<uint32_t>("relCommandWidth", 1, l_found);
	if (!l_found) {
		std::cout << "relCommandWidth value is missing ... exiting"
				<< std::endl;
		exit(-1);
	}

	k_REFI = (uint32_t)x_params.find<uint32_t>("nREFI", 1, l_found);
	if (!l_found) {
		std::cout << "nREFI value is missing ... exiting" << std::endl;
		exit(-1);
	}

	// calculate total number of banks

	int l_numChannelsPerDimm = (uint32_t)x_params.find<uint32_t>("numChannelsPerDimm", 1,
			l_found);
	if (!l_found) {
		std::cout << "numChannelsPerDimm value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	int l_numRanksPerChannel = (uint32_t)x_params.find<uint32_t>("numRanksPerChannel", 100,
			l_found);
	if (!l_found) {
		std::cout << "numRanksPerChannel value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	int l_numBankGroupsPerRank = (uint32_t)x_params.find<uint32_t>("numBankGroupsPerRank",
			100, l_found);
	if (!l_found) {
		std::cout << "numBankGroupsPerRank value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	int l_numBanksPerBankGroup = (uint32_t)x_params.find<uint32_t>("numBanksPerBankGroup",
			100, l_found);
	if (!l_found) {
		std::cout << "numBanksPerBankGroup value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	m_numBanks = l_numRanksPerChannel * l_numBankGroupsPerRank
			* l_numBanksPerBankGroup;

	// refresh stuff
	// populate the m_refreshGroups with bankIds

	//per-rank refresh groups (all-banks refresh)
	uint l_groupId = 0;
	uint l_bankId = 0;
	for(uint l_chan = 0; l_chan < l_numChannelsPerDimm; l_chan++) {
	  for(uint l_rank = 0; l_rank < l_numRanksPerChannel; l_rank++) {
	    // every rank in a different refresh group
	    m_refreshGroups.push_back(std::vector<unsigned>());
	    for(uint l_bankGroup = 0; l_bankGroup < l_numBankGroupsPerRank; l_bankGroup++) {
	      for(uint l_bank = 0; l_bank < l_numBanksPerBankGroup; l_bank++) {
		m_refreshGroups[l_groupId].push_back(l_bankId);
		l_bankId++;
	      } // banks
	    } // bankgroups
	    l_groupId++;
	  } // ranks
	} // channels
	
	m_currentRefreshGroup=0;
	m_currentREFICount = (int)((double)k_REFI/m_refreshGroups.size());

	int l_tmp = 0;
	for(auto l_vec : m_refreshGroups) {
	  std::cout << "Refresh Group " << l_tmp << " : ";
	  for(auto l_bankId : l_vec) {
	    std::cout << l_bankId << " ";
	  } std::cout << std::endl;
	  l_tmp++;
	}
	  

	//load neighboring component's params
	k_txnGenResQEntries = (uint32_t)x_params.find<uint32_t>("numTxnGenResQEntries", 100,
			l_found);
	if (!l_found) {
		std::cout << "numTxnGenResQEntries param value is missing... exiting"
				<< std::endl;
		exit(-1);
	}
	m_txnGenResQTokens = k_txnGenResQEntries;

	k_cmdUnitReqQEntries = (uint32_t)x_params.find<uint32_t>("numCmdReqQEntries", 100,
			l_found);
	if (!l_found) {
		std::cout << "numCmdReqQEntries param value is missing... exiting"
				<< std::endl;
		exit(-1);
	}
	m_cmdUnitReqQTokens = k_cmdUnitReqQEntries;

	k_useReadA = (uint32_t)x_params.find<uint32_t>("boolUseReadA", 1, l_found);
	if (!l_found) {
		std::cout << "boolUseWriteA param value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	k_useWriteA = (uint32_t)x_params.find<uint32_t>("boolUseWriteA", 1, l_found);
	if (!l_found) {
		std::cout << "boolUseWriteA param value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	k_bankPolicy = (uint32_t)x_params.find<uint32_t>("bankPolicy", 0, l_found);
	if (!l_found) {
		std::cout << "bankPolicy value is missing... exiting" << std::endl;
		exit(-1);
	}

	if ((k_bankPolicy == 1) && (k_useReadA || k_useWriteA)) {
		std::cout << "Open bank/row and READA or WRITEA makes no sense"
				<< std::endl;
		exit(-1);
	}

	k_useRefresh = (uint32_t)x_params.find<uint32_t>("boolUseRefresh", 1, l_found);
	if (!l_found) {
		std::cout << "boolUseRefresh param value is missing... exiting"
				<< std::endl;
		exit(-1);
	}

	m_statsReqQ = new unsigned[k_txnReqQEntries + 1];
	m_statsResQ = new unsigned[k_txnResQEntries + 1];

	for (unsigned l_i = 0; l_i != k_txnReqQEntries + 1; ++l_i)
		m_statsReqQ[l_i] = 0;

	for (unsigned l_i = 0; l_i != k_txnResQEntries + 1; ++l_i)
		m_statsResQ[l_i] = 0;

	// tell the simulator not to end without us <-- why??[seokin]
	//registerAsPrimaryComponent();
	//primaryComponentDoNotEndSim();

	// configure links

	// TxnUnit <-> TxnGen Links

	// TxnUnit <- TxnGen (Req) (Txn)
	m_inTxnGenReqPtrLink = configureLink("inTxnGenReqPtr",
			new Event::Handler<c_TxnUnit>(this,
					&c_TxnUnit::handleInTxnGenReqPtrEvent));
	// TxnUnit -> TxnGen (Req) (Token)
	m_outTxnGenReqQTokenChgLink = configureLink("outTxnGenReqQTokenChg",
			new Event::Handler<c_TxnUnit>(this,
					&c_TxnUnit::handleOutTxnGenReqQTokenChgEvent));
	// TxnUnit -> TxnGen (Res) (Txn)
	m_outTxnGenResPtrLink = configureLink("outTxnGenResPtr",
			new Event::Handler<c_TxnUnit>(this,
					&c_TxnUnit::handleOutTxnGenResPtrEvent));
	// TxnUnit <- TxnGen (Res) (Token)
	m_inTxnGenResQTokenChgLink = configureLink("inTxnGenResQTokenChg",
			new Event::Handler<c_TxnUnit>(this,
					&c_TxnUnit::handleInTxnGenResQTokenChgEvent));

	// TxnUnit <-> CmdUnit Links

	// TxnUnit -> CmdUnit (Req) (Cmd)
	m_outCmdUnitReqPtrLink = configureLink("outCmdUnitReqPtrPkg",
			new Event::Handler<c_TxnUnit>(this,
					&c_TxnUnit::handleOutCmdUnitReqPtrEvent));
	// TxnUnit <- CmdUnit (Req) (Token)
	m_inCmdUnitReqQTokenChgLink = configureLink("inCmdUnitReqQTokenChg",
			new Event::Handler<c_TxnUnit>(this,
					&c_TxnUnit::handleInCmdUnitReqQTokenChgEvent));
	// TxnUnit <- CmdUnit (Res) (Txn)
	m_inCmdUnitResPtrLink = configureLink("inCmdUnitResPtr",
			new Event::Handler<c_TxnUnit>(this,
					&c_TxnUnit::handleInCmdUnitResPtrEvent));

	//set our clock
	registerClock("1GHz",
			new Clock::Handler<c_TxnUnit>(this, &c_TxnUnit::clockTic));

	m_processingRefreshCmds = false;

	// Statistics setup
	s_readTxnsRecvd = registerStatistic<uint64_t>("readTxnsRecvd");
	s_writeTxnsRecvd = registerStatistic<uint64_t>("writeTxnsRecvd");
	s_totalTxnsRecvd = registerStatistic<uint64_t>("totalTxnsRecvd");
	s_reqQueueSize = registerStatistic<uint64_t>("reqQueueSize");
	s_resQueueSize = registerStatistic<uint64_t>("resQueueSize");
}

c_TxnUnit::~c_TxnUnit() {

}

c_TxnUnit::c_TxnUnit() :
		Component(-1) {
	// for serialization only
}

void c_TxnUnit::print() const {
	std::cout << "***TxnUnit " << Component::getName() << std::endl;
	std::cout << "ReqQEntries=" << k_txnReqQEntries << ", " << "ResQEntries="
			<< k_txnResQEntries << std::endl;
	std::cout << "k_txnGenResQEntries=" << k_txnGenResQEntries
			<< ", k_cmdReqQEntries=" << k_cmdUnitReqQEntries << std::endl;
	std::cout << "TxnReqQ size=" << m_txnReqQ.size() << ", " << "TxnResQ size="
			<< m_txnResQ.size() << std::endl;
	std::cout << "m_cmdReqQTokens = " << m_cmdUnitReqQTokens << ", "
			<< "m_txnGenResQTokens = " << m_txnGenResQTokens << std::endl;
}

void c_TxnUnit::printQueues() {
	std::cout << "TxnReqQ: " << std::endl;
	for (auto& l_entry : m_txnReqQ) {
		l_entry->print();
		std::cout << std::endl;
	}

	std::cout << std::endl << "TxnResQ:" << std::endl;
	for (auto& l_entry : m_txnResQ) {
		l_entry->print();
		std::cout << std::endl;
	}
}

bool c_TxnUnit::clockTic(SST::Cycle_t) {

//	std::cout << std::endl << std::endl << "TxnUnit:: clock tic" << std::endl;
//	std::cout << "m_ResQTokens: " << m_txnGenResQTokens << std::endl;
//	printQueues();

	m_thisCycleReqQTknChg = 0;

	// store the current number of entries in the queue to later compute change
	m_thisCycleReqQTknChg = m_txnReqQ.size();

	sendResponse();

	if (k_useRefresh) {
		// if the refresh counter is still counting, send regular Request
		// else send REF to all banks
		if (m_currentREFICount > 0) {
//			std::cout << __PRETTY_FUNCTION__ << ": m_currentREFICount = "
//					<< m_currentREFICount << std::endl;

			--m_currentREFICount;
		} else {
			if (!m_processingRefreshCmds) {
//				std::cout << __PRETTY_FUNCTION__ << ": Creating REFs"
//						<< std::endl;

				createRefreshCmds();
				m_processingRefreshCmds = true;
			}
		}

		if (m_processingRefreshCmds && (m_refreshList.size() == 0)
				&& (m_cmdUnitReqQTokens == k_cmdUnitReqQEntries)) {
			// refresh was started and now we have all tokens from CmdUnit CmdReqQ and the refresh list container is empty so refresh must have finished
			// therefore now we can start another refresh cycle
		        m_currentREFICount = (int)((double)k_REFI/m_refreshGroups.size());
			m_processingRefreshCmds = false;
		}
	}

	sendRequest();

	m_thisCycleReqQTknChg -= m_txnReqQ.size();

	sendTokenChg();

	//FIXME: Delete. For debugging queue size issues
	m_statsReqQ[m_txnReqQ.size()]++;
	m_statsResQ[m_txnResQ.size()]++;

	s_reqQueueSize->addData(m_txnReqQ.size());
	s_resQueueSize->addData(m_txnResQ.size());

	return false;
}

void c_TxnUnit::sendResponse() {

	// sendResponse conditions:
	// - m_txnGenResQTokens > 0
	// - m_txnResQ.size() > 0
	// - m_txnResQ has an element which is response-ready

  //std::cout << "m_txnGenResQTokens " << std::dec << m_txnGenResQTokens << std::endl;
  //std::cout << "m_txnResQ " << std::dec << m_txnResQ.size() << std::endl;
  //printQueues();

	if ((m_txnGenResQTokens > 0) && (m_txnResQ.size() > 0)) {

		c_Transaction* l_txnRes = nullptr;
		for (std::vector<c_Transaction*>::iterator l_it = m_txnResQ.begin();
				l_it != m_txnResQ.end(); ++l_it) {
			if ((*l_it)->isResponseReady()) {
				l_txnRes = *l_it;
				m_txnResQ.erase(l_it);
				break;
			}
		}

		if (l_txnRes != nullptr) {

		        //std::cout << "@" << std::dec
			//		<< Simulation::getSimulation()->getCurrentSimCycle()
			//		<< ": " << __PRETTY_FUNCTION__ << std::endl;
			//l_txnRes->print();
			//std::cout << std::endl;

			c_TxnResEvent* l_txnResEvPtr = new c_TxnResEvent();
			l_txnResEvPtr->m_payload = l_txnRes;
			m_outTxnGenResPtrLink->send(l_txnResEvPtr);
			--m_txnGenResQTokens;

		}

	}
} // sendResponse

void c_TxnUnit::createRefreshCmds() {
//	std::cout << "@" << std::dec
//			<< Simulation::getSimulation()->getCurrentSimCycle() << ": "
//			<< __PRETTY_FUNCTION__ << std::endl;

	c_TransactionToCommands* l_converter =
			c_TransactionToCommands::getInstance();

	//cout << "Refreshing group " << m_currentRefreshGroup << endl;
	m_refreshList = l_converter->getRefreshCommands(m_refreshGroups[m_currentRefreshGroup]);
	m_currentRefreshGroup++;
	if(m_currentRefreshGroup >= m_refreshGroups.size()) {
	  m_currentRefreshGroup = 0;
	}
}

void c_TxnUnit::sendRequest() {

	// first check if any refreshes are pending being sent
	if (m_refreshList.size() > 0) {
	  //		std::cout << "@" << std::dec
	  //		  << Simulation::getSimulation()->getCurrentSimCycle() << ": "
	  //		  << __PRETTY_FUNCTION__ << std::endl;
	  //	printf("%u REF commands left to send\n", m_refreshList.size());
		std::vector<c_BankCommand*> l_cmdPkg;
		while (m_cmdUnitReqQTokens > 0 && m_refreshList.size() > 0) {
			l_cmdPkg.push_back(m_refreshList.front());
			m_refreshList.pop();

			m_cmdUnitReqQTokens--;
		}

		if (l_cmdPkg.size() > 0) {
		  //printf("Sending %lu REF commands\n", l_cmdPkg.size());
			c_CmdPtrPkgEvent* l_cmdPtrPkgEventPtr = new c_CmdPtrPkgEvent();
			l_cmdPtrPkgEventPtr->m_payload = l_cmdPkg;
			m_outCmdUnitReqPtrLink->send(l_cmdPtrPkgEventPtr);
		}

		return;
	}

	if ((m_txnReqQ.size() > 0) && !m_processingRefreshCmds) {
	        //std::cout << "@" << std::dec
		//	  << Simulation::getSimulation()->getCurrentSimCycle() << ": "
		//	  << __PRETTY_FUNCTION__ << std::endl;
		c_Transaction* l_reqTxn = m_txnReqQ.front();
		//l_reqTxn->print();
		//std::cout << std::endl;

		c_TransactionToCommands* l_converter =
				c_TransactionToCommands::getInstance();
		std::vector<c_BankCommand*> l_cmdPkg = l_converter->getCommands(
				l_reqTxn, k_relCommandWidth, k_useReadA, k_useWriteA);

		if ((l_cmdPkg.size() < m_cmdUnitReqQTokens)
				&& ((k_txnResQEntries - m_txnResQ.size()) > 0)) {
			c_CmdPtrPkgEvent* l_cmdPtrPkgEventPtr = new c_CmdPtrPkgEvent();
			l_cmdPtrPkgEventPtr->m_payload = l_cmdPkg;
			m_outCmdUnitReqPtrLink->send(l_cmdPtrPkgEventPtr);
			m_cmdUnitReqQTokens -= l_cmdPkg.size();

			//Add the related txn to m_ResQ and pop it out of m_ReqQ
			m_txnResQ.push_back(l_reqTxn);
			m_txnReqQ.erase(m_txnReqQ.begin());

			//std::cout << "Txn unit side pkg size " << l_cmdPkg.size() << std::endl;
			//for (auto &l_entry : l_cmdPkg) {
			//  std::cout<<"Txn (*l_entry) = " << std::hex << l_entry << std::endl;
			//  l_entry->print();
			//  std::cout << std::endl;
			//}
			
			//std::cout << "@" << std::dec
			//	  << Simulation::getSimulation()->getCurrentSimCycle()
			//	  << ": " << __PRETTY_FUNCTION__ << ": Request sent"
			//	  << std::endl;
		} else {
			for (int l_i = 0; l_i != l_cmdPkg.size(); ++l_i)
				delete l_cmdPkg[l_i];

		}
	} else {
		// std::cout << "@" << std::dec
		// 		<< Simulation::getSimulation()->getCurrentSimCycle() << ": "
		// 		<< __PRETTY_FUNCTION__ << ": No Requests to send out"
		// 		<< std::endl;
	}

}

void c_TxnUnit::sendTokenChg() {
	// only send tokens when space has opened up in queues
	// there are no negative tokens. token subtraction must be performed
	// in the source component immediately after sending an event
	if (m_thisCycleReqQTknChg > 0) {
		// send req q token chg
		// std::cout << "@" << std::dec
		// 		<< Simulation::getSimulation()->getCurrentSimCycle() << ": "
		// 		<< __PRETTY_FUNCTION__ << std::endl;
		// std::cout << "TxnUnit:: Sending req q tokens" << std::endl;
		c_TokenChgEvent* l_txnReqQTokenChgEvent = new c_TokenChgEvent();
		l_txnReqQTokenChgEvent->m_payload = m_thisCycleReqQTknChg;
		m_outTxnGenReqQTokenChgLink->send(l_txnReqQTokenChgEvent);
	}
}

// ----EVENT HANDLERS----
//// TxnUnit <-> TxnGen Handlers
void c_TxnUnit::handleInTxnGenReqPtrEvent(SST::Event *ev) {
	// make sure the internal req q has at least one empty entry
	// to accept a new txn ptr
	assert(1 <= (k_txnReqQEntries - m_txnReqQ.size()));

	c_TxnReqEvent* l_txnReqEventPtr = dynamic_cast<c_TxnReqEvent*>(ev);
	if (l_txnReqEventPtr) {
	        //std::cout << "@" << std::dec
		//	  << Simulation::getSimulation()->getCurrentSimCycle() << ": "
		//	  << __PRETTY_FUNCTION__ << l_txnReqEventPtr->m_payload << " ";
		//l_txnReqEventPtr->m_payload->print();
		//std::cout << std::endl;

		if(l_txnReqEventPtr->m_payload->getTransactionMnemonic() == e_TransactionType::READ) {
		  s_readTxnsRecvd->addData(1);
		}
		if(l_txnReqEventPtr->m_payload->getTransactionMnemonic() == e_TransactionType::WRITE) {
		  s_writeTxnsRecvd->addData(1);
		}
		s_totalTxnsRecvd->addData(1);
		
		m_txnReqQ.push_back(l_txnReqEventPtr->m_payload);
		delete l_txnReqEventPtr;
	} else {
		std::cout << __PRETTY_FUNCTION__ << "ERROR:: Bad event type!"
				<< std::endl;
	}

}

void c_TxnUnit::handleOutTxnGenReqQTokenChgEvent(SST::Event *ev) {
	// nothing to do here
	std::cout << __PRETTY_FUNCTION__ << " ERROR: Should not be here"
			<< std::endl;
}

void c_TxnUnit::handleOutTxnGenResPtrEvent(SST::Event *ev) {
	// nothing to do here
	std::cout << __PRETTY_FUNCTION__ << " ERROR: Should not be here"
			<< std::endl;
}

void c_TxnUnit::handleInTxnGenResQTokenChgEvent(SST::Event *ev) {
	c_TokenChgEvent* l_txnGenResQTokenChgEventPtr =
			dynamic_cast<c_TokenChgEvent*>(ev);
	if (l_txnGenResQTokenChgEventPtr) {
		// std::cout << std::endl << "@" << std::dec
		// 		<< Simulation::getSimulation()->getCurrentSimCycle() << ": "
		// 		<< __PRETTY_FUNCTION__ << std::endl;

		m_txnGenResQTokens += l_txnGenResQTokenChgEventPtr->m_payload;

		assert(m_txnGenResQTokens >= 0);
		assert(m_txnGenResQTokens <= k_txnGenResQEntries);

		// std::cout << "m_ResQTokens: " << m_txnGenResQTokens << std::endl;
		delete l_txnGenResQTokenChgEventPtr;
	} else {
		std::cout << __PRETTY_FUNCTION__ << "ERROR:: Bad event type!"
				<< std::endl;
	}
}

//// TxnUnit <-> CmdUnit Handlers
void c_TxnUnit::handleOutCmdUnitReqPtrEvent(SST::Event *ev) {
	// nothing to do here
	std::cout << __PRETTY_FUNCTION__ << " ERROR: Should not be here"
			<< std::endl;
}

void c_TxnUnit::handleInCmdUnitReqQTokenChgEvent(SST::Event *ev) {
	c_TokenChgEvent* l_cmdUnitReqQTokenChgEventPtr =
			dynamic_cast<c_TokenChgEvent*>(ev);
	if (l_cmdUnitReqQTokenChgEventPtr) {
		// std::cout << std::endl << "@" << std::dec
		// 		<< Simulation::getSimulation()->getCurrentSimCycle() << ": "
		// 		<< __PRETTY_FUNCTION__ << std::endl;

		m_cmdUnitReqQTokens += l_cmdUnitReqQTokenChgEventPtr->m_payload;

		assert(m_cmdUnitReqQTokens >= 0);
		assert(m_cmdUnitReqQTokens <= k_cmdUnitReqQEntries);

		//FIXME: Critical: This pointer is left dangling
		delete l_cmdUnitReqQTokenChgEventPtr;
	} else {
		std::cout << __PRETTY_FUNCTION__ << "ERROR:: Bad event type!"
				<< std::endl;
	}
}

void c_TxnUnit::handleInCmdUnitResPtrEvent(SST::Event *ev) {

  c_CmdResEvent* l_cmdResEventPtr = dynamic_cast<c_CmdResEvent*>(ev);
  if (l_cmdResEventPtr) {
    ulong l_resSeqNum = l_cmdResEventPtr->m_payload->getSeqNum();
    // need to find which txn matches the command seq number in the txnResQ
    c_Transaction* l_txnRes = nullptr;
    for(auto l_txIter : m_txnResQ) {
      if(l_txIter->matchesCmdSeqNum(l_resSeqNum)) {
	l_txnRes = l_txIter;
      }
    }

    if(l_txnRes == nullptr) {
      std::cout << "Error! Couldn't find transaction to match cmdSeqnum " << l_resSeqNum << std::endl;
      exit(-1);
    }

    const unsigned l_cmdsLeft = l_txnRes->getWaitingCommands() - 1;
    l_txnRes->setWaitingCommands(l_cmdsLeft);
    if (l_cmdsLeft == 0)
      l_txnRes->setResponseReady();

    //std::cout << std::endl << "@" << std::dec
    //      << Simulation::getSimulation()->getCurrentSimCycle() << ": "
    //	      << __PRETTY_FUNCTION__ << std::endl;
    //l_txnRes->print();
    //std::cout << std::endl;

    delete l_cmdResEventPtr->m_payload;
    //delete l_cmdResEventPtr;

  } else {
    std::cout << __PRETTY_FUNCTION__ << "ERROR:: Bad event type!"
	      << std::endl;
  }
}
