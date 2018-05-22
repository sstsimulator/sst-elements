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
#include "c_BankGroup.hpp"
#include "c_BankCommand.hpp"
#include "c_BankInfo.hpp"
#include "c_Rank.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_BankGroup::c_BankGroup(std::map<std::string, unsigned>* x_bankParams, unsigned x_Id) {
	m_rankPtr = nullptr;
	m_bankParams = x_bankParams;
	m_bankGroupId = x_Id;
}

c_BankGroup::~c_BankGroup() {
	for (unsigned l_i = 0; l_i != m_bankPtrs.size(); ++l_i)
		if (m_bankPtrs[l_i] != nullptr)
			delete m_bankPtrs[l_i];
}

void c_BankGroup::acceptBank(c_BankInfo* x_bankPtr) {
	assert(nullptr != x_bankPtr);
	m_bankPtrs.push_back(x_bankPtr);
}

void c_BankGroup::acceptRank(c_Rank* x_rankPtr) {
	assert(nullptr != x_rankPtr);
	m_rankPtr = x_rankPtr;
}

unsigned c_BankGroup::getNumBanks() const {
	return m_bankPtrs.size();
}

unsigned c_BankGroup::getBankGroupId() const {
        return m_bankGroupId;
}

std::vector<c_BankInfo*> c_BankGroup::getBankPtrs() const {
	return (m_bankPtrs);
}

c_Rank* c_BankGroup::getRankPtr() const{
	return m_rankPtr;
}

void c_BankGroup::updateOtherBanksNextCommandCycles(c_BankInfo* x_initBankPtr,
		c_BankCommand* x_cmdPtr, SimTime_t x_cycle) {

//	std::cout << "Entered " << __PRETTY_FUNCTION__ << std::endl;

	SimTime_t l_time = x_cycle;
	for (std::vector<c_BankInfo*>::iterator l_iter = m_bankPtrs.begin();
			l_iter != m_bankPtrs.end(); ++l_iter) {

		// skip the bank given as param
		if (x_initBankPtr == (*l_iter))
			continue;

		c_BankInfo* l_bankPtr = (*l_iter); // easier reference to the bank

		switch (x_cmdPtr->getCommandMnemonic()) {
		case e_BankCommandType::ACT: {
			SimTime_t l_nextCycle = std::max(
					l_bankPtr->getNextCommandCycle(e_BankCommandType::ACT),
					l_time + m_bankParams->at("nRRD_L"));
			l_bankPtr->setNextCommandCycle(e_BankCommandType::ACT, l_nextCycle);
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
							+ std::max(m_bankParams->at("nCCD_L"),
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
					l_time
							+ m_bankParams->at("nCL") + m_bankParams->at("nBL")
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
					l_time
							+ m_bankParams->at("nCWL") + m_bankParams->at("nBL")
							+ m_bankParams->at("nWTR_L"));
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
							+ std::max(m_bankParams->at("nCCD_L"),
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
	m_rankPtr->updateOtherBanksNextCommandCycles(this, x_cmdPtr, l_time);
}
