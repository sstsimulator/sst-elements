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

#include <memory>
#include <assert.h>

#include <sst/core/simulation.h>

#include "c_BankState.hpp"
#include "c_BankInfo.hpp"
#include "c_BankCommand.hpp"
#include "c_Transaction.hpp"
#include "c_BankStateReadA.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_BankStateReadA::c_BankStateReadA(
		std::map<std::string, unsigned>* x_bankParams) {

	// std::cout << "Entered " << __PRETTY_FUNCTION__ << std::endl;
	m_timerEnter = 0;
	m_timerExit = 0;
	m_currentState = e_BankState::READA;
	m_bankParams = x_bankParams;
	m_allowedCommands.clear();
}

c_BankStateReadA::~c_BankStateReadA() {
	// std::cout << std::endl << __PRETTY_FUNCTION__ << std::endl;

}

// handle the command based state changes in function handleCommand( ... )
// handle automatic state changes in function clockTic( ... )

void c_BankStateReadA::handleCommand(c_BankInfo* x_bank,
		c_BankCommand* x_bankCommandPtr, SimTime_t x_cycle) {
}

void c_BankStateReadA::clockTic(c_BankInfo* x_bank, SimTime_t x_cycle) {
	if (0 < m_timerEnter) {
		--m_timerEnter;
	} else {
		if (0 == m_timerExit) {
			SimTime_t l_time = x_cycle;
			m_nextStatePtr = new c_BankStatePrecharge(m_bankParams);
			x_bank->setLastCommandCycle(e_BankCommandType::PRE, l_time);
			SimTime_t l_nextCycle = std::max(
					x_bank->getNextCommandCycle(e_BankCommandType::PRE),
					std::max(
							x_bank->getLastCommandCycle(e_BankCommandType::ACT)
									+ m_bankParams->at("nRAS"),
							std::max(
									x_bank->getLastCommandCycle(
											e_BankCommandType::READA)
											+ m_bankParams->at("nRTP"),
									x_bank->getLastCommandCycle(
											e_BankCommandType::READ)
											+ m_bankParams->at("nRTP")))) - 1;
			m_timerExit = 0;
			if (l_time < l_nextCycle) {
				m_timerExit = l_nextCycle - l_time;
			}
		}

		if (1 < m_timerExit) {
			--m_timerExit;
		} else {
			if (nullptr != m_nextStatePtr)
				m_nextStatePtr->enter(x_bank, this, nullptr, x_cycle);
		}
	}

}

void c_BankStateReadA::enter(c_BankInfo* x_bank, c_BankState* x_prevState,
		c_BankCommand* x_cmdPtr, SimTime_t x_cycle) {

//	std::cout << "Entered " << __PRETTY_FUNCTION__ << std::endl;

	m_timerExit = 0;

	m_prevCommandPtr = x_cmdPtr;
	if (nullptr != m_prevCommandPtr) {
		m_prevCommandPtr->setResponseReady();

		switch (m_prevCommandPtr->getCommandMnemonic()) {
		case e_BankCommandType::READA:
			m_timerEnter = std::max(m_bankParams->at("nCCD_L"),
					m_bankParams->at("nBL")) - 1;
			break;
		default:
			std::cout << __PRETTY_FUNCTION__ << ": Unrecognized state"
					<< std::endl;
			exit(-1);
			break;
		}

		m_prevCommandPtr = nullptr;
	}

	SimTime_t l_time = x_cycle;

	x_bank->setLastCommandCycle(e_BankCommandType::READA,l_time);


	m_allowedCommands.clear();
	x_bank->setNextCommandCycle(e_BankCommandType::READ,
			(std::max(x_bank->getNextCommandCycle(e_BankCommandType::READ),
					x_bank->getLastCommandCycle(e_BankCommandType::READ)
							+ m_bankParams->at("nCCD_L"))) - 1);
	x_bank->setNextCommandCycle(e_BankCommandType::READA,
			(std::max(x_bank->getNextCommandCycle(e_BankCommandType::READA),
					x_bank->getLastCommandCycle(e_BankCommandType::READ)
							+ m_bankParams->at("nCCD_L"))) - 1);
	x_bank->setNextCommandCycle(e_BankCommandType::WRITE,
			(std::max(x_bank->getNextCommandCycle(e_BankCommandType::WRITE),
					x_bank->getLastCommandCycle(e_BankCommandType::READ)
							+ m_bankParams->at("nCWL") + m_bankParams->at("nBL")
							+ m_bankParams->at("nWTR")) - 1));
	x_bank->setNextCommandCycle(e_BankCommandType::WRITEA,
			(std::max(x_bank->getNextCommandCycle(e_BankCommandType::WRITEA),
					x_bank->getLastCommandCycle(e_BankCommandType::READ)
							+ m_bankParams->at("nCWL") + m_bankParams->at("nBL")
							+ m_bankParams->at("nWTR")) - 1));
	x_bank->setNextCommandCycle(e_BankCommandType::PRE,
			(std::max(x_bank->getNextCommandCycle(e_BankCommandType::PRE),
					std::max(
							x_bank->getLastCommandCycle(e_BankCommandType::ACT)
									+ m_bankParams->at("nRAS"),
							std::max(
									x_bank->getLastCommandCycle(
											e_BankCommandType::READA)
											+ m_bankParams->at("nRTP"),
									x_bank->getLastCommandCycle(
											e_BankCommandType::READ)
											+ m_bankParams->at("nRTP"))))) - 1);

	x_bank->changeState(this);
	if (nullptr != x_prevState)
		delete x_prevState;

}

std::list<e_BankCommandType> c_BankStateReadA::getAllowedCommands() {
	return (m_allowedCommands);
}

bool c_BankStateReadA::isCommandAllowed(c_BankCommand* x_cmdPtr,
		c_BankInfo* x_bankPtr) {
	return false;
}
