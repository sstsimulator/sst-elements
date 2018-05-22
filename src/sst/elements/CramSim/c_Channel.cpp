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
#include "c_Channel.hpp"
#include "c_BankCommand.hpp"
#include "c_Rank.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_Channel::c_Channel(std::map<std::string, unsigned>* x_bankParams) {
	m_bankParams = x_bankParams;
	m_rankPtrs.clear();
}

c_Channel::c_Channel(std::map<std::string, unsigned>* x_bankParams, unsigned x_chId) : c_Channel(x_bankParams) {
		m_chId = x_chId;
}


c_Channel::~c_Channel() {
	for (unsigned l_i = 0; l_i != m_rankPtrs.size(); ++l_i)
		if (m_rankPtrs.at(l_i) != nullptr)
			delete m_rankPtrs.at(l_i);
}

void c_Channel::acceptRank(c_Rank* x_rankPtr) {
	assert(nullptr != x_rankPtr);
	m_rankPtrs.push_back(x_rankPtr);
}

unsigned c_Channel::getNumBanks() const {
	unsigned l_numBanks = 0;
	for (auto& l_rankPtr : m_rankPtrs)
		l_numBanks += l_rankPtr->getNumBanks();

	return l_numBanks;
}

unsigned c_Channel::getNumRanks() const {
	return m_rankPtrs.size();
}

unsigned c_Channel::getChannelId() const {
	return m_chId;
}

std::vector<c_BankInfo*> c_Channel::getBankPtrs() const {
	std::vector<c_BankInfo*> l_allBankPtrs;
	for (auto& l_rankPtr : m_rankPtrs) {
		std::vector<c_BankInfo*>& l_entryPtrs = l_rankPtr->getBankPtrs();
		l_allBankPtrs.insert(l_allBankPtrs.end(), l_entryPtrs.begin(),
				l_entryPtrs.end());
	}

	return l_allBankPtrs;
}

void c_Channel::updateOtherBanksNextCommandCycles(c_Rank* x_initRankPtr,
		c_BankCommand* x_cmdPtr, SimTime_t x_cycle) {

	SimTime_t l_time = x_cycle;

	for (auto& l_rankPtr : m_rankPtrs) {

		// skip the bank given as param
		if (x_initRankPtr == l_rankPtr)
			continue;

		for (auto& l_bankPtr : l_rankPtr->getBankPtrs()) {


			switch (x_cmdPtr->getCommandMnemonic()) {
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
								+ std::max(m_bankParams->at("nBL"),
										std::min(m_bankParams->at("nCCD_S"),
												m_bankParams->at("nCCD_L")))
								+ m_bankParams->at("nERTR"));

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
								+ m_bankParams->at("nCL")
								+ std::max(m_bankParams->at("nBL"),
										std::min(m_bankParams->at("nCCD_S"),
												m_bankParams->at("nCCD_L")))
								+ m_bankParams->at("nERTW")
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
						l_time
								+ m_bankParams->at("nCWL")
								+ std::max(m_bankParams->at("nBL"),
										std::min(m_bankParams->at("nCCD_S"),
												m_bankParams->at("nCCD_L")))
								+ m_bankParams->at("nEWTR")
								- m_bankParams->at("nCL"));

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
								+ std::max(m_bankParams->at("nBL"),
										std::min(m_bankParams->at("nCCD_S"),
												m_bankParams->at("nCCD_L")))
								+ m_bankParams->at("nEWTW"));


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
}
