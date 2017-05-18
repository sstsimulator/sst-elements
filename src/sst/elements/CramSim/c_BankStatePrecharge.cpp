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
#include <memory>
#include <iostream>
#include <assert.h>

// CramSim includes
#include "c_BankState.hpp"
#include "c_BankCommand.hpp"
#include "c_Transaction.hpp"
#include "c_BankStateIdle.hpp"
#include "c_BankStatePrecharge.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_BankStatePrecharge::c_BankStatePrecharge(
		std::map<std::string, unsigned>* x_bankParams) :
		m_receivedCommandPtr(nullptr) {
	// std::cout << std::endl << __PRETTY_FUNCTION__ << std::endl;

	m_bankParams = x_bankParams;
	m_currentState = e_BankState::PRE;
}

c_BankStatePrecharge::~c_BankStatePrecharge() {
	// std::cout << std::endl << __PRETTY_FUNCTION__ << std::endl;

}

void c_BankStatePrecharge::handleCommand(c_BankInfo* x_bank,
		c_BankCommand* x_bankCommandPtr) {
	// std::cout << __PRETTY_FUNCTION__
	// 		<< " ERROR: should not receive a command in this state. This is a transitory state."
	// 		<< std::endl;
}

void c_BankStatePrecharge::clockTic(c_BankInfo* x_bank) {
	if (0 < m_timer) {
		--m_timer;

//		std::cout << __PRETTY_FUNCTION__ << "@@" << std::dec << "m_timer = "
//				<< m_timer << " ";
//		if (m_prevCommandPtr)
//			m_prevCommandPtr->print();

	} else {

		//FIXME: Remove. for testing only
		//assert(1 == 0);
//		std::cout << __PRETTY_FUNCTION__ << " timer expired ";
//		if (m_prevCommandPtr)
//			m_prevCommandPtr->print();

		if (m_prevCommandPtr) {
			m_prevCommandPtr->setResponseReady();
			//const unsigned l_cmdsLeft =
			//		m_prevCommandPtr->getTransaction()->getWaitingCommands()
			//				- 1;
			//m_prevCommandPtr->getTransaction()->setWaitingCommands(l_cmdsLeft);
			//if (l_cmdsLeft == 0)
			//	m_prevCommandPtr->getTransaction()->setResponseReady();
		}
		auto l_p = new c_BankStateIdle(m_bankParams);
		l_p->enter(x_bank, this, nullptr);
	}
}

void c_BankStatePrecharge::enter(c_BankInfo* x_bank, c_BankState* x_prevState,
		c_BankCommand* x_cmdPtr) {

//	std::cout << "Entered " << __PRETTY_FUNCTION__ << std::endl;
	x_bank->resetRowOpen();
	m_prevCommandPtr = x_cmdPtr;
	m_receivedCommandPtr = nullptr;
	m_timer = m_bankParams->at("nRP");
	m_allowedCommands.clear();

//	if (m_prevCommandPtr) {
//		std::cout << "Previous command = ";
//		m_prevCommandPtr->print();
//		std::cout << std::endl;
//	}

// this state should not have any allowed bank commands
// this is a transitory state

	x_bank->changeState(this);
	if (nullptr != x_prevState)
		delete x_prevState;

}

std::list<e_BankCommandType> c_BankStatePrecharge::getAllowedCommands() {
	return (m_allowedCommands);
}

bool c_BankStatePrecharge::isCommandAllowed(c_BankCommand* x_cmdPtr,
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
