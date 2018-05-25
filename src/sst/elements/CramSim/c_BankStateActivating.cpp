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

#include "c_BankState.hpp"
#include "c_BankInfo.hpp"
#include "c_BankCommand.hpp"
#include "c_BankStateActive.hpp"
#include "c_BankStateActivating.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_BankStateActivating::c_BankStateActivating(std::map<std::string, unsigned>* x_bankParams) :
	m_receivedCommandPtr(nullptr) {

	//std::cout << "Entered " << __PRETTY_FUNCTION__ << std::endl;
	m_timer = 0;
	m_currentState = e_BankState::ACTNG;
	m_bankParams = x_bankParams;
	m_allowedCommands.clear();
}

c_BankStateActivating::~c_BankStateActivating() {
	// std::cout << std::endl << __PRETTY_FUNCTION__ << std::endl;
}

// handle the command based state changes in function handleCommand( ... )
// handle automatic state changes in function update( ... )

void c_BankStateActivating::handleCommand(c_BankInfo* x_bank, c_BankCommand* x_bankCommandPtr, SimTime_t x_cycle) {
	std::cout << __PRETTY_FUNCTION__
			<< " ERROR: should not receive a command in this state. This is a transitory state."
			<< std::endl;
}

void c_BankStateActivating::clockTic(c_BankInfo* x_bank, SimTime_t x_cycle) {

	if (0 < m_timer) {
		--m_timer;

	} else {
		auto l_p = new c_BankStateActive(m_bankParams); // create pointer to the next state
		assert(e_BankCommandType::ACT == m_prevCommandPtr->getCommandMnemonic()); // only cmd allowed to flow through to BankStateActive is ACT
		l_p->enter(x_bank, this, m_prevCommandPtr, x_cycle);
	}
}

void c_BankStateActivating::enter(c_BankInfo* x_bank,
		c_BankState* x_prevState, c_BankCommand* x_cmdPtr, SimTime_t x_cycle) {
	//std::cout << "Entered " << __PRETTY_FUNCTION__ << std::endl;

	// set timer for auto precharge countdown used in the pseudo-open page policy
	x_bank->setAutoPreTimer(m_bankParams->at("nRAS"));
	x_bank->setRowOpen();
	x_bank->setOpenRowNum(x_cmdPtr->getHashedAddress()->getRow());

	// Being in the activating state does not make an ACT cmd response ready.
	// Therefore it is forwarded to BankStateActive
	m_prevCommandPtr = x_cmdPtr;
	m_receivedCommandPtr = nullptr;
	m_timer = 0; //TODO: Ask Michael Healey about the timing here
	m_allowedCommands.clear();
	// this state should not have any allowed bank commands
	// this is a transitory state

	x_bank->changeState(this);
	if (nullptr != x_prevState)
		delete x_prevState;
}

std::list<e_BankCommandType> c_BankStateActivating::getAllowedCommands() {
	return (m_allowedCommands);
}


bool c_BankStateActivating::isCommandAllowed(c_BankCommand* x_cmdPtr,
		c_BankInfo* x_bankPtr) {

	// Cmd must be of an allowed type and BankState cannot already be processing another cmd
	for (std::list<e_BankCommandType>::iterator l_iter =
			m_allowedCommands.begin(); l_iter != m_allowedCommands.end(); ++l_iter) {

		if (x_cmdPtr->getCommandMnemonic() == *l_iter && m_receivedCommandPtr
				== nullptr)
			return true;
	}
	return false;

}
