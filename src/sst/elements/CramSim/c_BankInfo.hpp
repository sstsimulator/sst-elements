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
#ifndef C_BANKINFO_HPP
#define C_BANKINFO_HPP

// C++ includes
#include <memory>
#include <list>
#include <map>

// CramSim includes
#include "c_BankState.hpp"

namespace SST {
namespace n_Bank {

class c_BankGroup;
class c_BankCommand;
enum class e_BankCommandType;
  
class c_BankInfo {
public:

	c_BankInfo();
	c_BankInfo(std::map<std::string, unsigned>* x_bankParams,
			unsigned x_bankId);

	virtual ~c_BankInfo();

	void handleCommand(c_BankCommand* x_bankCommandPtr, SimTime_t x_simCycle);

	void clockTic(SimTime_t x_cycle);

	std::list<e_BankCommandType> getAllowedCommands();

	bool isCommandAllowed(c_BankCommand* x_cmdPtr, SimTime_t x_simCycle);

	e_BankState getCurrentState() {
		return (m_bankState->getCurrentState());
	}

	void changeState(c_BankState* x_newState);

	void setNextCommandCycle(const e_BankCommandType x_cmd,
			const SimTime_t x_cycle);
	SimTime_t getNextCommandCycle(e_BankCommandType x_cmd);

	void setLastCommandCycle(e_BankCommandType x_cmd, SimTime_t x_lastCycle);
	SimTime_t getLastCommandCycle(e_BankCommandType x_cmd);

	void acceptBankGroup(c_BankGroup* x_bankGroupPtr);

	bool isRowOpen() const {
		return (m_isRowOpen);
	}
	void setRowOpen() {
		m_isRowOpen = true;
	}
	void resetRowOpen() {
		m_isRowOpen = false;
	}
	void setOpenRowNum(const unsigned x_openRowNum) {
		m_openRowNum = x_openRowNum;
	}
	unsigned getOpenRowNum() const {
		return (m_openRowNum);
	}
	void setAutoPreTimer(SimTime_t x_timerVal) {
		m_autoPrechargeTimer = x_timerVal;
	}
	SimTime_t getAutoPreTimer() {
		return (m_autoPrechargeTimer);
	}
	void setBankId(const unsigned x_bankId) {
		m_bankId=x_bankId;
	}

	void print();
        unsigned getBankId() {
	  return m_bankId;
	}
  
	c_BankGroup *getBankGroupPtr() {
	  return m_bankGroupPtr;
	}
private:
	void reset();

	unsigned m_bankId;

	bool m_isRowOpen;
	unsigned m_openRowNum;

	c_BankState* m_bankState;
	c_BankGroup* m_bankGroupPtr;

	std::map<std::string, unsigned>* m_bankParams;
	std::map<e_BankCommandType, SimTime_t> m_lastCommandCycleMap;
	std::map<e_BankCommandType, SimTime_t> m_nextCommandCycleMap;

	//TESTING -- DELETE
	std::map<e_BankCommandType, std::string> m_cmdToString;

	SimTime_t m_autoPrechargeTimer; // used to model a pseudo-open page policy

};
}
}

#endif // C_BANKINFO_HPP
