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
#include <memory>
#include <assert.h>

#include "sst_config.h"

//local includes
#include "c_BankInfo.hpp"
#include "c_BankState.hpp"
#include "c_BankStateIdle.hpp"
#include "c_BankCommand.hpp"
#include "c_BankGroup.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_BankInfo::c_BankInfo() :
		m_bankState(new c_BankStateIdle(nullptr)) {

	reset();
	m_bankState->enter(this, nullptr, nullptr,0);
}

c_BankInfo::c_BankInfo(std::map<std::string, unsigned>* x_bankParams,
		unsigned x_bankId) :
		m_bankParams(x_bankParams), m_bankId(x_bankId), m_bankState(
				new c_BankStateIdle(x_bankParams)),
                m_autoPrechargeTimer(0) {

	reset();
	m_bankState->enter(this, nullptr, nullptr,0);

}
c_BankInfo::~c_BankInfo() {
	// if (nullptr != m_bankState)
	// 	delete m_bankState;
}

void c_BankInfo::print() {
	std::cout << "m_bankId = " << m_bankId << std::endl;
	printf("Current state: ");
	switch (m_bankState->getCurrentState()) {
	case e_BankState::IDLE:
		printf("IDLE\n");
		break;
	case e_BankState::ACTNG:
		printf("ACTNG\n");
		break;
	case e_BankState::ACTIVE:
		printf("ACTIVE\n");
		break;
	case e_BankState::READ:
		printf("READ\n");
		break;
	case e_BankState::READA:
		printf("READA\n");
		break;
	case e_BankState::WRITE:
		printf("WRITE\n");
		break;
	case e_BankState::WRITEA:
		printf("WRITEA\n");
		break;
	case e_BankState::PRE:
		printf("PRE\n");
		break;
	case e_BankState::REF:
		printf("REF\n");
		break;
	default:
	    break;
	}
	std::cout << "m_nextCommandCycleMap: " << std::endl;
	for (auto l_mapEntry : m_nextCommandCycleMap) {
		std::cout << m_cmdToString[l_mapEntry.first] << ":" << std::dec
				<< l_mapEntry.second << std::endl;
	}
}

void c_BankInfo::reset() {
	m_lastCommandCycleMap = { {e_BankCommandType::ACT,0},
		{	e_BankCommandType::READ,0},
		{	e_BankCommandType::READA,0},
		{	e_BankCommandType::WRITE,0},
		{	e_BankCommandType::WRITEA,0},
		{	e_BankCommandType::PRE,0},
		{	e_BankCommandType::REF,0}};

	m_nextCommandCycleMap = { {e_BankCommandType::ACT,0},
		{	e_BankCommandType::READ,0},
		{	e_BankCommandType::READA,0},
		{	e_BankCommandType::WRITE,0},
		{	e_BankCommandType::WRITEA,0},
		{	e_BankCommandType::PRE,0},
		{	e_BankCommandType::REF, 0}};

	m_cmdToString[e_BankCommandType::ERR] = "ERR";
	m_cmdToString[e_BankCommandType::ACT] = "ACT";
	m_cmdToString[e_BankCommandType::READ] = "READ";
	m_cmdToString[e_BankCommandType::READA] = "READA";
	m_cmdToString[e_BankCommandType::WRITE] = "WRITE";
	m_cmdToString[e_BankCommandType::WRITEA] = "WRITEA";
	m_cmdToString[e_BankCommandType::PRE] = "PRE";
	m_cmdToString[e_BankCommandType::PREA] = "PREA";
	m_cmdToString[e_BankCommandType::REF] = "REF";
}

void c_BankInfo::handleCommand(c_BankCommand* x_bankCommandPtr,
                               SimTime_t x_simCycle) {
	assert(
			m_nextCommandCycleMap.end() != m_nextCommandCycleMap.find(x_bankCommandPtr->getCommandMnemonic()));
	assert(
			x_simCycle >= m_nextCommandCycleMap[x_bankCommandPtr->getCommandMnemonic()]);


	m_bankState->handleCommand(this, x_bankCommandPtr,x_simCycle);
	m_bankGroupPtr->updateOtherBanksNextCommandCycles(this, x_bankCommandPtr, x_simCycle);
}

void c_BankInfo::clockTic(SimTime_t x_cycle) {
	if (0 < m_autoPrechargeTimer)
		--m_autoPrechargeTimer;

	m_bankState->clockTic(this, x_cycle);

}

std::list<e_BankCommandType> c_BankInfo::getAllowedCommands() {
	return m_bankState->getAllowedCommands();
}

bool c_BankInfo::isCommandAllowed(c_BankCommand* x_cmdPtr,
                                  SimTime_t x_simCycle) {
	bool l_canAccept = false;
	assert(nullptr != m_bankState);

	if (m_bankState->isCommandAllowed(x_cmdPtr, this)) {
		assert(m_nextCommandCycleMap.find(x_cmdPtr->getCommandMnemonic()) != m_nextCommandCycleMap.end());
		if (m_nextCommandCycleMap.find(x_cmdPtr->getCommandMnemonic())->second <= x_simCycle)
			l_canAccept = true;


	}

	return l_canAccept;
}

void c_BankInfo::changeState(c_BankState* x_newState) {
	if (nullptr != x_newState)
		m_bankState = x_newState;
}

void c_BankInfo::setNextCommandCycle(const e_BankCommandType x_cmd,
		const SimTime_t x_cycle) {
	assert(m_nextCommandCycleMap.end() != m_nextCommandCycleMap.find(x_cmd));
	m_nextCommandCycleMap[x_cmd] = x_cycle;
}

SimTime_t c_BankInfo::getNextCommandCycle(e_BankCommandType x_cmd) {
	assert(m_nextCommandCycleMap.end() != m_nextCommandCycleMap.find(x_cmd));
	return (m_nextCommandCycleMap[x_cmd]);
}

void c_BankInfo::setLastCommandCycle(e_BankCommandType x_cmd,
                                     SimTime_t x_lastCycle) {
	assert(m_lastCommandCycleMap.end() != m_lastCommandCycleMap.find(x_cmd));
	m_lastCommandCycleMap[x_cmd] = x_lastCycle;
}

SimTime_t c_BankInfo::getLastCommandCycle(e_BankCommandType x_cmd) {
	assert(m_lastCommandCycleMap.end() != m_lastCommandCycleMap.find(x_cmd));
	return m_lastCommandCycleMap[x_cmd];
}

void c_BankInfo::acceptBankGroup(c_BankGroup* x_bankGroupPtr) {
	assert(nullptr != x_bankGroupPtr);
	m_bankGroupPtr = x_bankGroupPtr;
}
