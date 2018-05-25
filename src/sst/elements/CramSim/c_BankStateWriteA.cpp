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

#include "c_Bank.hpp"
#include "c_BankState.hpp"
#include "c_BankInfo.hpp"
#include "c_BankCommand.hpp"
#include "c_Transaction.hpp"
#include "c_BankStateWriteA.hpp"
#include "c_BankStatePrecharge.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_BankStateWriteA::c_BankStateWriteA(
		std::map<std::string, unsigned>* x_bankParams) {
	// std::cout << "Entered " << __PRETTY_FUNCTION__ << std::endl;
	m_timerEnter = 0;
	m_timerExit = 0;
	m_currentState = e_BankState::WRITEA;
	m_bankParams = x_bankParams;
	m_allowedCommands.clear();
}

c_BankStateWriteA::~c_BankStateWriteA() {
}

// handle the command based state changes in function handleCommand( ... )
// handle automatic state changes in function update( ... )

void c_BankStateWriteA::handleCommand(c_BankInfo* x_bank,
		c_BankCommand* x_bankCommandPtr, SimTime_t x_cycle) {
	// std::cout << __PRETTY_FUNCTION__
	// 		<< "ERROR: Bank commands are irrelevant in the current state ... exiting simulation"
	// 		<< std::endl;
}

void c_BankStateWriteA::clockTic(c_BankInfo* x_bank, SimTime_t x_cycle) {

	if (0 < m_timerEnter) {
		--m_timerEnter;
	} else {
		if (0 == m_timerExit) {
			SimTime_t l_time = x_cycle;
			x_bank->setLastCommandCycle(e_BankCommandType::WRITEA, l_time);
			SimTime_t l_nextCycle = std::max(
					x_bank->getNextCommandCycle(e_BankCommandType::ACT)
							+ m_bankParams->at("nRAS"),
					x_bank->getLastCommandCycle(e_BankCommandType::WRITEA)
							+ m_bankParams->at("nCWL") + m_bankParams->at("nBL")
							+ m_bankParams->at("nWR"))-2;
			m_timerExit = 0;
			if (l_nextCycle > l_time){
				m_timerExit = l_nextCycle-l_time;
			}
		}

		if (1 < m_timerExit) {
			--m_timerExit;
		} else {
			if (nullptr != m_nextStatePtr)
				m_nextStatePtr->enter(x_bank, this, nullptr,x_cycle);
		}
	}
}

void c_BankStateWriteA::enter(c_BankInfo* x_bank, c_BankState* x_prevState,
		c_BankCommand* x_cmdPtr, SimTime_t x_cycle) {
//	 std::cout << "Entered " << __PRETTY_FUNCTION__ << std::endl;

	// Being in the WriteA state does not make a READA cmd response ready.
	// Therefore it is forwarded to BankStatePrecharge
	m_timerExit = 0;
	m_prevCommandPtr = x_cmdPtr;
	if (nullptr != m_prevCommandPtr) {
		m_prevCommandPtr->setResponseReady();

		switch (m_prevCommandPtr->getCommandMnemonic()) {
		case e_BankCommandType::WRITEA:
			m_timerEnter = std::max(m_bankParams->at("nCCD_L"),m_bankParams->at("nBL"))-1;
			break;
		default:
			std::cout << __PRETTY_FUNCTION__ << ": Unrecognized command"
					<< std::endl;
			exit(-1);
			break;
		}
		m_prevCommandPtr = nullptr;
	}

	SimTime_t l_time = x_cycle;


	x_bank->setLastCommandCycle(e_BankCommandType::WRITEA,l_time);

	m_allowedCommands.clear();
	x_bank->setNextCommandCycle(e_BankCommandType::READ,
			std::max(x_bank->getNextCommandCycle(e_BankCommandType::READ),
					x_bank->getLastCommandCycle(e_BankCommandType::WRITE))
					+ m_bankParams->at("nCWL") + m_bankParams->at("nBL")
					+ m_bankParams->at("nWR") - m_bankParams->at("nCL"));
	x_bank->setNextCommandCycle(e_BankCommandType::READA,
			std::max(x_bank->getNextCommandCycle(e_BankCommandType::READA),
					x_bank->getLastCommandCycle(e_BankCommandType::WRITE))
					+ m_bankParams->at("nCWL") + m_bankParams->at("nBL")
					+ m_bankParams->at("nWR") - m_bankParams->at("nCL"));
	x_bank->setNextCommandCycle(e_BankCommandType::WRITE,
			std::max(x_bank->getNextCommandCycle(e_BankCommandType::WRITE),
					l_time + m_bankParams->at("nCCD_L")));
	x_bank->setNextCommandCycle(e_BankCommandType::PRE,
			std::max(x_bank->getNextCommandCycle(e_BankCommandType::PRE),
					std::max(
							x_bank->getLastCommandCycle(e_BankCommandType::ACT)
									+ m_bankParams->at("nRAS") - 2,
							std::max(
									x_bank->getLastCommandCycle(
											e_BankCommandType::WRITE),
									x_bank->getLastCommandCycle(
											e_BankCommandType::READ)
											+ m_bankParams->at("nRTP") - 2))));
	x_bank->setNextCommandCycle(e_BankCommandType::WRITEA,
			std::max(
					x_bank->getNextCommandCycle(e_BankCommandType::ACT)
							+ m_bankParams->at("nRAS"),
					x_bank->getLastCommandCycle(e_BankCommandType::WRITEA)
							+ m_bankParams->at("nCWL") + m_bankParams->at("nBL")
							+ m_bankParams->at("nWR"))-2);

	m_nextStatePtr = new c_BankStatePrecharge(m_bankParams);

	x_bank->changeState(this);
	if (nullptr != x_prevState)
		delete x_prevState;

}

std::list<e_BankCommandType> c_BankStateWriteA::getAllowedCommands() {
	return (m_allowedCommands);
}

bool c_BankStateWriteA::isCommandAllowed(c_BankCommand* x_cmdPtr,
		c_BankInfo* x_bankPtr) {

	return false;

}
