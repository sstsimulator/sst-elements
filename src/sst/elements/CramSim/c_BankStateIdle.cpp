// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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
#include <iostream>
#include <assert.h>

#include "c_BankStateIdle.hpp"
#include "c_BankState.hpp"
#include "c_BankCommand.hpp"
#include "c_BankInfo.hpp"
#include "c_BankStateActivating.hpp"
#include "c_BankStateRefresh.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_BankStateIdle::c_BankStateIdle(std::map<std::string, unsigned>* x_bankParams) :
		m_receivedCommandPtr(nullptr), m_timer(0) {
	//std::cout << "Entered " << __PRETTY_FUNCTION__ << std::endl;
	m_bankParams = x_bankParams;
	m_currentState = e_BankState::IDLE;
}

c_BankStateIdle::~c_BankStateIdle() {
	// std::cout << std::endl << __PRETTY_FUNCTION__ << std::endl;

}

// this function is called by the c_Bank that contains this state
void c_BankStateIdle::handleCommand(c_BankInfo* x_bank,
		c_BankCommand* x_bankCommandPtr) {
	// std::cout << "Entered c_BankStateIdle::handleCommand() received command "
	// 		<< std::endl;
	SimTime_t l_time =
			Simulation::getSimulation()->getCurrentSimCycle();
	switch (x_bankCommandPtr->getCommandMnemonic()) {
	case e_BankCommandType::ACT:
		x_bank->setLastCommandCycle(e_BankCommandType::ACT, l_time);
		break;
	case e_BankCommandType::REF:
		x_bank->setLastCommandCycle(e_BankCommandType::REF, l_time);
		break;
	default:
	    break;
	}
	if (nullptr == m_receivedCommandPtr) {
		m_timer = 1;
		m_receivedCommandPtr = x_bankCommandPtr;
		m_allowedCommands.clear();
	}
}

// returns the list of allowed commands in this state
std::list<e_BankCommandType> c_BankStateIdle::getAllowedCommands() {
	return (m_allowedCommands);
}

// call this function every clock cycle
void c_BankStateIdle::clockTic(c_BankInfo* x_bank) {
	if (2 < m_timer) {
		--m_timer;
	} else {
		--m_timer;
		if (1 == m_timer) {
			if (m_prevCommandPtr) {
				m_prevCommandPtr->setResponseReady();
				const unsigned l_cmdsLeft =
						m_prevCommandPtr->getTransaction()->getWaitingCommands()
								- 1;
				m_prevCommandPtr->getTransaction()->setWaitingCommands(
						l_cmdsLeft);

				if (l_cmdsLeft == 0)
					m_prevCommandPtr->getTransaction()->setResponseReady();

			}
		} else {
			if (m_receivedCommandPtr) {
				// std::cout << "@@" << std::dec
				// 		<< l_time
				// 		<< ": now can process command response in IDLE state"
				// 		<< std::endl;

				SimTime_t l_time =
						Simulation::getSimulation()->getCurrentSimCycle();
				c_BankState* l_p = nullptr;
				switch (m_receivedCommandPtr->getCommandMnemonic()) {
				case e_BankCommandType::ACT:
					l_p = new c_BankStateActivating(m_bankParams);
//					x_bank->setLastCommandCycle(e_BankCommandType::ACT, l_time);
					break;
				case e_BankCommandType::REF:
					l_p = new c_BankStateRefresh(m_bankParams);
//					x_bank->setLastCommandCycle(e_BankCommandType::REF, l_time);
					break;
				default:
				break;
				}

				assert(nullptr != m_receivedCommandPtr);
				// make sure we have a command
				l_p->enter(x_bank, this, m_receivedCommandPtr);
			}
		}
	}
}

// call this function after receiving a command
void c_BankStateIdle::enter(c_BankInfo* x_bank, c_BankState* x_prevState,
		c_BankCommand* x_cmdPtr) {

//	std::cout << "@@" << std::dec
//			<< Simulation::getSimulation()->getCurrentSimCycle()
//			<< ": m_timer = " << m_timer << std::endl;
//	std::cout << "Entering " << __PRETTY_FUNCTION__ << std::endl;

	m_prevCommandPtr = x_cmdPtr;

	m_receivedCommandPtr = nullptr;

	unsigned l_time = Simulation::getSimulation()->getCurrentSimCycle();

	m_allowedCommands.clear();
	m_allowedCommands.push_back(e_BankCommandType::ACT);
	m_allowedCommands.push_back(e_BankCommandType::REF);

	x_bank->setNextCommandCycle(e_BankCommandType::ACT,
			std::max(x_bank->getNextCommandCycle(e_BankCommandType::ACT),
					l_time));

	x_bank->setNextCommandCycle(e_BankCommandType::REF,
			std::max(x_bank->getNextCommandCycle(e_BankCommandType::REF),
					l_time));

	x_bank->changeState(this);

	if (nullptr != x_prevState)
		delete x_prevState;
}

bool c_BankStateIdle::isCommandAllowed(c_BankCommand* x_cmdPtr,
		c_BankInfo* x_bankPtr) {

// Cmd must be of an allowed type and BankState cannot already be processing another cmd
	for (std::list<e_BankCommandType>::iterator l_iter =
			m_allowedCommands.begin(); l_iter != m_allowedCommands.end();
			++l_iter) {

		if (x_cmdPtr->getCommandMnemonic() == *l_iter
				&& m_receivedCommandPtr == nullptr)
			return true;
	}
	return false;

}
