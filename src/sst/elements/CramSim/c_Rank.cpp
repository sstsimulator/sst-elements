// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// Copyright 2016 IBM Corporation

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

// global includes
#include <assert.h>

// local includes
#include "c_Rank.hpp"
#include "c_BankCommand.hpp"
#include "c_BankGroup.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_Rank::c_Rank(std::map<std::string, unsigned>* x_bankParams) {
	m_channelPtr = nullptr;
	m_bankParams = x_bankParams;
	m_allBankPtrs.clear();
}

c_Rank::~c_Rank() {
	for (unsigned l_i = 0; l_i != m_bankGroupPtrs.size(); ++l_i)
		if (m_bankGroupPtrs.at(l_i) != nullptr)
			delete m_bankGroupPtrs.at(l_i);
}

void c_Rank::acceptBankGroup(c_BankGroup* x_bankGroupPtr) {
	assert(nullptr != x_bankGroupPtr);
	m_bankGroupPtrs.push_back(x_bankGroupPtr);
}

void c_Rank::acceptChannel(c_Channel* x_channelPtr) {
	assert(nullptr != x_channelPtr);
	m_channelPtr = x_channelPtr;
}

unsigned c_Rank::getNumBanks() const {
	unsigned l_numBanks = 0;
	for (auto& l_bankGroupPtr : m_bankGroupPtrs)
		l_numBanks += l_bankGroupPtr->getNumBanks();

	return l_numBanks;
}

unsigned c_Rank::getNumBankGroups() const {
	return m_bankGroupPtrs.size();
}

c_Channel* c_Rank::getChannelPtr() const {
	return m_channelPtr;
}

std::vector<c_BankInfo*>& c_Rank::getBankPtrs() {

	if(m_allBankPtrs.empty()) {
		for (auto &l_bankGroupPtr : m_bankGroupPtrs) {
			std::vector<c_BankInfo *> l_entryPtrs = l_bankGroupPtr->getBankPtrs();
			m_allBankPtrs.insert(m_allBankPtrs.end(), l_entryPtrs.begin(), l_entryPtrs.end());
		}
	}

	return m_allBankPtrs;
}

void c_Rank::updateOtherBanksNextCommandCycles(c_BankGroup* x_initBankGroupPtr,
					       c_BankCommand* x_cmdPtr, SimTime_t x_cycle) {

  SimTime_t l_time = x_cycle;
  for (auto& l_bankGroupPtr : m_bankGroupPtrs) {

    // skip the bank given as param
    if (x_initBankGroupPtr == l_bankGroupPtr)
      continue;

    for (auto& l_bankPtr : l_bankGroupPtr->getBankPtrs()) {



      switch (x_cmdPtr->getCommandMnemonic()) {
      case e_BankCommandType::ACT: {
	SimTime_t l_nextCycle = std::max(
					 l_bankPtr->getNextCommandCycle(e_BankCommandType::ACT),
					 l_time + m_bankParams->at("nRRD_S"));
	l_bankPtr->setNextCommandCycle(e_BankCommandType::ACT,
				       l_nextCycle);
	break;
      }
      case e_BankCommandType::READ:
      case e_BankCommandType::READA: {
	// timing for the next READ or READA command
	SimTime_t l_nextCycle = std::max(
					 std::max(
						  l_bankPtr->getNextCommandCycle(
										 e_BankCommandType::READ),
						  l_bankPtr->getNextCommandCycle(
										 e_BankCommandType::READA)),
					 l_time
					 + std::max(m_bankParams->at("nCCD_S"),
						    m_bankParams->at("nBL")));

	l_bankPtr->setNextCommandCycle(e_BankCommandType::READ,
				       l_nextCycle);
	l_bankPtr->setNextCommandCycle(e_BankCommandType::READA,
				       l_nextCycle);

	// timing for the next WRITE or WRITEA command
	l_nextCycle = std::max(
			       std::max(
					l_bankPtr->getNextCommandCycle(
								       e_BankCommandType::WRITE),
					l_bankPtr->getNextCommandCycle(
								       e_BankCommandType::WRITEA)),
			       l_time + m_bankParams->at("nCL")
			       + m_bankParams->at("nBL")
			       + m_bankParams->at("nRTW")
			       - m_bankParams->at("nCWL"));

	l_bankPtr->setNextCommandCycle(e_BankCommandType::WRITE,
				       l_nextCycle);
	l_bankPtr->setNextCommandCycle(e_BankCommandType::WRITEA,
				       l_nextCycle);

	break;
      }
      case e_BankCommandType::WRITE:
      case e_BankCommandType::WRITEA: {
	// timing for the next READ or READA command
	SimTime_t l_nextCycle = std::max(
					 std::max(
						  l_bankPtr->getNextCommandCycle(
										 e_BankCommandType::READ),
						  l_bankPtr->getNextCommandCycle(
										 e_BankCommandType::READA)),
					 l_time + m_bankParams->at("nCWL")
					 + m_bankParams->at("nBL")
					 + m_bankParams->at("nWTR_S"));

	l_bankPtr->setNextCommandCycle(e_BankCommandType::READ,
				       l_nextCycle);
	l_bankPtr->setNextCommandCycle(e_BankCommandType::READA,
				       l_nextCycle);

	// timing for the next WRITE or WRITEA command
	l_nextCycle = std::max(
			       std::max(
					l_bankPtr->getNextCommandCycle(
								       e_BankCommandType::WRITE),
					l_bankPtr->getNextCommandCycle(
								       e_BankCommandType::WRITEA)),
			       l_time
			       + std::max(m_bankParams->at("nCCD_S"),
					  m_bankParams->at("nBL")));
	l_bankPtr->setNextCommandCycle(e_BankCommandType::WRITE,
				       l_nextCycle);
	l_bankPtr->setNextCommandCycle(e_BankCommandType::WRITEA,
				       l_nextCycle);

	break;
      }
      default:
	break;
      }

    }
  }
  m_channelPtr->updateOtherBanksNextCommandCycles(this, x_cmdPtr, l_time);
}
