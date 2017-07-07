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
 * c_Dimm.cpp
 *
 *  Created on: June 27, 2016
 *      Author: mcohen
 */

// Copyright 2015 IBM Corporation
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

// C++ includes
#include <algorithm>
#include <string>
#include <assert.h>

// CramSim includes
#include "c_Dimm.hpp"
#include "c_AddressHasher.hpp"
#include "c_Transaction.hpp"
#include "c_Bank.hpp"
#include "c_CmdReqEvent.hpp"
#include "c_CmdResEvent.hpp"
#include "c_BankCommand.hpp"
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/sst_types.h>

using namespace SST;
using namespace SST::n_Bank;

c_Dimm::c_Dimm(SST::ComponentId_t x_id, SST::Params& x_params) :
		Component(x_id) {

	// tell the simulator not to end without us <-- why?
	//registerAsPrimaryComponent();
	//primaryComponentDoNotEndSim();

	// configure links

	// DIMM <-> CmdUnit Links
	//// DIMM <-> CmdUnit (Req) (Cmd)
	m_inCmdUnitReqPtrLink = configureLink("inCmdUnitReqPtr",
			new Event::Handler<c_Dimm>(this,
					&c_Dimm::handleInCmdUnitReqPtrEvent));
	//// DIMM <-> CmdUnit (Res) (Cmd)
	m_outCmdUnitResPtrLink = configureLink("outCmdUnitResPtr",
			new Event::Handler<c_Dimm>(this,
					&c_Dimm::handleOutCmdUnitResPtrEvent));

	// read params here
	bool l_found = false;

	k_numRanksPerChannel = (uint32_t)x_params.find<uint32_t>("numRanksPerChannel", 100,
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

	m_numBanks = k_numRanksPerChannel * k_numBankGroupsPerRank
			* k_numBanksPerBankGroup;

	Statistic<uint64_t> *l_totalRowHits = registerStatistic<uint64_t>("totalRowHits");
	for (int l_i = 0; l_i != m_numBanks; ++l_i) {
		c_Bank* l_entry = new c_Bank(x_params);
		m_banks.push_back(l_entry);

		std::string l_statName;
		unsigned l_bankNum = l_entry->getBankNum();
		c_BankStatistics *l_bankStats = new c_BankStatistics();
		
		l_statName = "bankACTsRecvd_" + std::to_string(l_bankNum);
		l_bankStats->s_bankACTsRecvd = registerStatistic<uint64_t>(l_statName);
		l_statName = "bankREADsRecvd_" + std::to_string(l_bankNum);
		l_bankStats->s_bankREADsRecvd = registerStatistic<uint64_t>(l_statName);
		l_statName = "bankWRITEsRecvd_" + std::to_string(l_bankNum);
		l_bankStats->s_bankWRITEsRecvd = registerStatistic<uint64_t>(l_statName);
		l_statName = "bankPREsRecvd_" + std::to_string(l_bankNum);
		l_bankStats->s_bankPREsRecvd = registerStatistic<uint64_t>(l_statName);

		l_statName = "bankRowHits_" + std::to_string(l_bankNum);
		l_bankStats->s_bankRowHits = registerStatistic<uint64_t>(l_statName);
		
		l_bankStats->s_totalRowHits = l_totalRowHits;
		
		l_entry->acceptStatistics(l_bankStats);
		m_bankStatsVec.push_back(l_bankStats);
	}

	m_thisCycleReceivedCmds = 0;

	//set our clock
	registerClock("1GHz", new Clock::Handler<c_Dimm>(this, &c_Dimm::clockTic));

	// Statistics setup
	s_actCmdsRecvd     = registerStatistic<uint64_t>("actCmdsRecvd");
	s_readCmdsRecvd    = registerStatistic<uint64_t>("readCmdsRecvd");
	s_readACmdsRecvd   = registerStatistic<uint64_t>("readACmdsRecvd");
	s_writeCmdsRecvd   = registerStatistic<uint64_t>("writeCmdsRecvd");
	s_writeACmdsRecvd  = registerStatistic<uint64_t>("writeACmdsRecvd");
	s_preCmdsRecvd     = registerStatistic<uint64_t>("preCmdsRecvd");
	s_refCmdsRecvd     = registerStatistic<uint64_t>("refCmdsRecvd");
}

c_Dimm::~c_Dimm() {

}

c_Dimm::c_Dimm() :
		Component(-1) {
	// for serialization only
}

void c_Dimm::printQueues() {
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	std::cout << "m_cmdResQ.size() = " << m_cmdResQ.size() << std::endl;
	for (auto& l_cmdPtr : m_cmdResQ)
		(l_cmdPtr)->print();
}

bool c_Dimm::clockTic(SST::Cycle_t) {
	// std::cout << std::endl << std::endl << "DIMM:: clock tic" << std::endl;
//	std::cout << std::endl << "@" << std::dec
//			<< Simulation::getSimulation()->getCurrentSimCycle() << ": "
//			<< __PRETTY_FUNCTION__ << std::endl;

	for (int l_i = 0; l_i != m_banks.size(); ++l_i) {
//		std::cout << "Bank" << std::dec << l_i << " clockTic from DIMM"
//				<< std::endl;

		c_BankCommand* l_resPtr = m_banks.at(l_i)->clockTic();
		if (nullptr != l_resPtr) {
			m_cmdResQ.push_back(l_resPtr);
		}
	}

//	printQueues();

	sendResponse();

	m_thisCycleReceivedCmds = 0;
	return false;
}

void c_Dimm::handleInCmdUnitReqPtrEvent(SST::Event *ev) {

	c_CmdReqEvent* l_cmdReqEventPtr = dynamic_cast<c_CmdReqEvent*>(ev);
	if (l_cmdReqEventPtr) {
		// each cycle, the DIMM should only receive one req
		m_thisCycleReceivedCmds++;
		// FIXME: confirm removal of this assert
		// tejask removed it because now we are allowing multiple
		// requests in the DIMM in the same cycle
		// assert(m_thisCycleReceivedCmds <= 1);

		c_BankCommand* l_cmdReq = l_cmdReqEventPtr->m_payload;

//		std::cout << std::endl << "@" << std::dec
//				<< Simulation::getSimulation()->getCurrentSimCycle() << ": "
//				<< __PRETTY_FUNCTION__ << " received command " << std::endl;
//		l_cmdReq->print();
//		std::cout << std::endl;

		switch(l_cmdReq->getCommandMnemonic()) {
		case e_BankCommandType::ACT:
		  s_actCmdsRecvd->addData(1);
		  break;
		case e_BankCommandType::READ:
		  s_readCmdsRecvd->addData(1);
		  break;
		case e_BankCommandType::READA:
		  s_readACmdsRecvd->addData(1);
		  break;
		case e_BankCommandType::WRITE:
		  s_writeCmdsRecvd->addData(1);
		  break;
		case e_BankCommandType::WRITEA:
		  s_writeACmdsRecvd->addData(1);
		  break;
		case e_BankCommandType::PRE:
		  s_preCmdsRecvd->addData(1);
		  break;
		case e_BankCommandType::REF:
		  s_refCmdsRecvd->addData(1);
		  break;
		}

		sendToBank(l_cmdReq);

		delete l_cmdReqEventPtr;
	} else {
		std::cout << __PRETTY_FUNCTION__ << "ERROR:: Bad event type!"
				<< std::endl;
	}
}

void c_Dimm::sendToBank(c_BankCommand* x_bankCommandPtr) {

  unsigned l_bankNum=0;
  if(x_bankCommandPtr->getCommandMnemonic() == e_BankCommandType::REF) {
    l_bankNum = x_bankCommandPtr->getBankId();
  } else {
    l_bankNum = x_bankCommandPtr->getHashedAddress()->getBankId();
  }
  m_banks.at(l_bankNum)->handleCommand(x_bankCommandPtr);
  
}

void c_Dimm::sendResponse() {

	// check if ResQ has cmds
	if (m_cmdResQ.size() > 0) {

	  //std::cout << std::endl << "@" << std::dec
	  //	    << Simulation::getSimulation()->getCurrentSimCycle() << ": "
	  //	    << __PRETTY_FUNCTION__ << std::endl;
	  //m_cmdResQ.front()->print();
	  //std::cout << std::endl;

	  c_CmdResEvent* l_cmdResEventPtr = new c_CmdResEvent();
	  l_cmdResEventPtr->m_payload = m_cmdResQ.front();
	  m_cmdResQ.erase(
			  std::remove(m_cmdResQ.begin(), m_cmdResQ.end(),
				      m_cmdResQ.front()), m_cmdResQ.end());
	  m_outCmdUnitResPtrLink->send(l_cmdResEventPtr);
	}
}

void c_Dimm::handleOutCmdUnitResPtrEvent(SST::Event *ev) {
	// nothing to do here
	std::cout << __PRETTY_FUNCTION__ << " ERROR: Should not be here"
			<< std::endl;
}
