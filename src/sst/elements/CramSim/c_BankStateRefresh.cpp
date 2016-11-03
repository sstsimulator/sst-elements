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

#include "c_BankStateRefresh.hpp"
#include "c_BankState.hpp"
#include "c_BankCommand.hpp"
#include "c_BankInfo.hpp"
#include "c_BankStateIdle.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_BankStateRefresh::c_BankStateRefresh(
		std::map<std::string, unsigned>* x_bankParams) :
		m_receivedCommandPtr(nullptr), m_timer(0) {
	// std::cout << "Entered " << __PRETTY_FUNCTION__ << std::endl;
	m_bankParams = x_bankParams;
	m_currentState = e_BankState::REF;
	m_allowedCommands.clear();
}

c_BankStateRefresh::~c_BankStateRefresh() {
	// std::cout << std::endl << __PRETTY_FUNCTION__ << std::endl;

}

// this function is called by the c_Bank that contains this state
void c_BankStateRefresh::handleCommand(c_BankInfo* x_bank,
		c_BankCommand* x_bankCommandPtr) {
	std::cout << __PRETTY_FUNCTION__
			<< " ERROR: should not receive a command in this state. This is a transitory state."
			<< std::endl;
}

// returns the list of allowed commands in this state
std::list<e_BankCommandType> c_BankStateRefresh::getAllowedCommands() {
	return (m_allowedCommands);
}

// call this function every clock cycle
void c_BankStateRefresh::clockTic(c_BankInfo* x_bank) {
	if (1 < m_timer) {
		--m_timer;

		// std::cout << "@@" << std::dec
		// 		<< Simulation::getSimulation()->getCurrentSimCycle()
		// 		<< ": m_timer = " << m_timer << std::endl;
	} else {

		auto l_p = new c_BankStateIdle(m_bankParams); // create pointer to the next state
		assert(
				e_BankCommandType::REF == m_prevCommandPtr->getCommandMnemonic());
		// only cmd allowed to flow through to BankStateActive is ACT

//		std::cout << "@@" << std::dec
//				<< Simulation::getSimulation()->getCurrentSimCycle()
//				<< ": m_timer = " << m_timer << std::endl;
//		std::cout << __PRETTY_FUNCTION__ << " timer expired" << std::endl;
//		std::cout << " for command ";
//		m_prevCommandPtr->print();
//		std::cout << std::endl;

		m_prevCommandPtr->setResponseReady();

		l_p->enter(x_bank, this, m_receivedCommandPtr);
	}
}

// call this function after receiving a command
void c_BankStateRefresh::enter(c_BankInfo* x_bank, c_BankState* x_prevState,
		c_BankCommand* x_cmdPtr) {
//	 std::cout << "Entered " << __PRETTY_FUNCTION__ << std::endl;

	// Being in the refresh state does not make a REF cmd response ready.
	// Therefore it is forwarded to BankStateIdle
	m_prevCommandPtr = x_cmdPtr;
	m_receivedCommandPtr = nullptr;
	m_timer = m_bankParams->at("nRFC");

	m_allowedCommands.clear();
	// this state should not have any allowed bank commands
	// this is a transitory state
	unsigned l_time = Simulation::getSimulation()->getCurrentSimCycle();
	x_bank->setNextCommandCycle(e_BankCommandType::REF,
			std::max(
					x_bank->getNextCommandCycle(e_BankCommandType::REF)
							+ m_bankParams->at("nREFI"), l_time));

	x_bank->changeState(this);
	if (nullptr != x_prevState)
		delete x_prevState;
}

bool c_BankStateRefresh::isCommandAllowed(c_BankCommand* x_cmdPtr,
		c_BankInfo* x_bankPtr) {

	return false;

}
