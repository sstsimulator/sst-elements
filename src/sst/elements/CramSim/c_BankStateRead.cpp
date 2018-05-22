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
#include <algorithm>
#include <assert.h>

#include "c_BankState.hpp"
#include "c_BankCommand.hpp"
#include "c_Transaction.hpp"
#include "c_BankStateActive.hpp"
#include "c_BankStatePrecharge.hpp"
#include "c_BankStateRead.hpp"
#include "c_BankStateReadA.hpp"
#include "c_BankStateWrite.hpp"
#include "c_BankStateWriteA.hpp"
#include "c_BankStateIdle.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_BankStateRead::c_BankStateRead(std::map<std::string, unsigned>* x_bankParams) :
		m_receivedCommandPtr(nullptr) {

	m_currentState = e_BankState::READ;
	m_bankParams = x_bankParams;
}

c_BankStateRead::~c_BankStateRead() {
	// std::cout << std::endl << __PRETTY_FUNCTION__ << std::endl;

}

void c_BankStateRead::handleCommand(c_BankInfo* x_bank,
		c_BankCommand* x_bankCommandPtr, SimTime_t x_cycle) {

	SimTime_t l_time = x_cycle;


	if (nullptr == m_receivedCommandPtr) {
		m_receivedCommandPtr = x_bankCommandPtr;

		m_nextStatePtr = nullptr;
		switch (m_receivedCommandPtr->getCommandMnemonic()) {
		case e_BankCommandType::READ:
			m_nextStatePtr = new c_BankStateRead(m_bankParams);
			x_bank->setLastCommandCycle(e_BankCommandType::READ, l_time);
			break;
		case e_BankCommandType::READA:
			m_nextStatePtr = new c_BankStateReadA(m_bankParams);
			x_bank->setLastCommandCycle(e_BankCommandType::READA, l_time);
			break;
		case e_BankCommandType::WRITE:
			m_nextStatePtr = new c_BankStateWrite(m_bankParams);
			x_bank->setLastCommandCycle(e_BankCommandType::WRITE, l_time);
			break;
		case e_BankCommandType::WRITEA:
			m_nextStatePtr = new c_BankStateWriteA(m_bankParams);
			x_bank->setLastCommandCycle(e_BankCommandType::WRITEA, l_time);
			break;
		case e_BankCommandType::PRE:
			m_nextStatePtr = new c_BankStatePrecharge(m_bankParams);
			x_bank->setLastCommandCycle(e_BankCommandType::PRE, l_time);
			break;
		default:
			std::cout << "Unrecognized state" << std::endl;
			exit(-1);
			break;
		}

	}
}

void c_BankStateRead::clockTic(c_BankInfo* x_bank, SimTime_t x_cycle) {

	SimTime_t l_time = x_cycle;

	if (0 < m_timer) {
		--m_timer;
	} else {
		if (m_receivedCommandPtr) {
			if (0 == m_timerExit) {
				switch (m_receivedCommandPtr->getCommandMnemonic()) {
				case e_BankCommandType::WRITE:
					if ((l_time)
							> (x_bank->getLastCommandCycle(
									e_BankCommandType::READ))) {
						if ((l_time
								- (x_bank->getLastCommandCycle(
										e_BankCommandType::READ)))
								< m_bankParams->at("nRTW")) {
							m_timerExit = m_bankParams->at("nRTW")
									- (l_time
											- (x_bank->getLastCommandCycle(
													e_BankCommandType::READ)));
						} else {
							m_timerExit = 0;
						}
					} else {
						m_timerExit = m_bankParams->at("nRTW");
					}
					break;
				case e_BankCommandType::READ:
					m_timerExit = 0;
					break;
				case e_BankCommandType::PRE:
					if ((l_time)
							> (x_bank->getLastCommandCycle(
									e_BankCommandType::READ))) {
						if ((l_time
								- (x_bank->getLastCommandCycle(
										e_BankCommandType::READ)))
								< m_bankParams->at("nRTP")) {
							m_timerExit = m_bankParams->at("nRTP")
									- (l_time
											- (x_bank->getLastCommandCycle(
													e_BankCommandType::READ)));
						} else {
							m_timerExit = 0;
						}
					} else {
						m_timerExit = m_bankParams->at("nRTP") - 1;
					}
					break;
				default:
					std::cout << "Unrecognized state" << std::endl;
					exit(-1);
					break;
				}
			}

			if (1 < m_timerExit) {
				--m_timerExit;
			} else {
				if ((nullptr != m_nextStatePtr)
						&& (m_receivedCommandPtr != nullptr))
					m_nextStatePtr->enter(x_bank, this, m_receivedCommandPtr,x_cycle);
			}
		}
	}
}

void c_BankStateRead::enter(c_BankInfo* x_bank, c_BankState* x_prevState,
		c_BankCommand* x_cmdPtr, SimTime_t x_cycle) {


	m_timerExit = 0;
	m_nextStatePtr = nullptr;

	m_prevCommandPtr = x_cmdPtr;
	if (nullptr != m_prevCommandPtr) {
		m_prevCommandPtr->setResponseReady();

		switch (m_prevCommandPtr->getCommandMnemonic()) {
		case e_BankCommandType::WRITE:
			m_timer = m_bankParams->at("nCL") - m_bankParams->at("nCWL")
					+ m_bankParams->at("nBL");
			break;
		case e_BankCommandType::READ:
			m_timer = std::max(m_bankParams->at("nCCD_L"),
					m_bankParams->at("nBL"))-1;
			break;
		case e_BankCommandType::ACT:
			m_timer = 0;
			break;
		default:
			std::cout << "Unrecognized state" << std::endl;
			exit(-1);
			break;
		}

		m_prevCommandPtr = nullptr;
	}

	m_receivedCommandPtr = nullptr;

	SimTime_t l_time = x_cycle;

	m_allowedCommands.clear();
	m_allowedCommands.push_back(e_BankCommandType::READ);
	x_bank->setNextCommandCycle(e_BankCommandType::READ,
			(std::max(x_bank->getNextCommandCycle(e_BankCommandType::READ),
					x_bank->getLastCommandCycle(e_BankCommandType::READ)
							+ m_bankParams->at("nCCD_L"))));

	m_allowedCommands.push_back(e_BankCommandType::READA);
	x_bank->setNextCommandCycle(e_BankCommandType::READA,
			(std::max(x_bank->getNextCommandCycle(e_BankCommandType::READA),
					x_bank->getLastCommandCycle(e_BankCommandType::READ)
							+ m_bankParams->at("nCCD_L"))));

//  FIXME: below for write going to the same row as the previous WRITE command
	m_allowedCommands.push_back(e_BankCommandType::WRITE);
	x_bank->setNextCommandCycle(e_BankCommandType::WRITE,
			(std::max(x_bank->getNextCommandCycle(e_BankCommandType::WRITE),
					x_bank->getLastCommandCycle(e_BankCommandType::READ)
							+ m_bankParams->at("nCWL") + m_bankParams->at("nBL")
							+ m_bankParams->at("nWTR"))));

	m_allowedCommands.push_back(e_BankCommandType::WRITEA);
	x_bank->setNextCommandCycle(e_BankCommandType::WRITEA,
			(std::max(x_bank->getNextCommandCycle(e_BankCommandType::WRITEA),
					x_bank->getLastCommandCycle(e_BankCommandType::READ)
							+ m_bankParams->at("nCWL") + m_bankParams->at("nBL")
							+ m_bankParams->at("nWTR"))));

	m_allowedCommands.push_back(e_BankCommandType::PRE);
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
											+ m_bankParams->at("nRTP"))))));
	x_bank->changeState(this);
	if (nullptr != x_prevState)
		delete x_prevState;

}

std::list<e_BankCommandType> c_BankStateRead::getAllowedCommands() {
	return (m_allowedCommands);
}

bool c_BankStateRead::isCommandAllowed(c_BankCommand* x_cmdPtr,
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
