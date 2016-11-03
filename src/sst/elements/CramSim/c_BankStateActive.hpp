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
#ifndef C_BANKSTATEACTIVE_HPP
#define C_BANKSTATEACTIVE_HPP

#include <memory>
#include <list>

#include "c_BankState.hpp"
#include "c_BankCommand.hpp"

namespace SST {
namespace n_Bank {

class c_BankStateRead;
class c_BankStateReadA;
class c_BankStateWrite;
class c_BankStateWriteA;
//class c_BankStatePrecharge;

class c_BankStateActive: public c_BankState {

public:

	c_BankStateActive(std::map<std::string, unsigned>* x_bankParams);
	~c_BankStateActive();

	virtual void handleCommand(c_BankInfo* x_bank, c_BankCommand* x_bankCommandPtr);

	virtual void clockTic(c_BankInfo* x_bank);

	virtual void enter(c_BankInfo* x_bank, c_BankState* x_prevState, c_BankCommand* x_cmdPtr);

	virtual std::list<e_BankCommandType> getAllowedCommands();

	virtual bool isCommandAllowed(c_BankCommand* x_cmdPtr,
			c_BankInfo* x_bankPtr);

private:

	std::list<e_BankCommandType> m_allowedCommands;
	c_BankCommand* m_receivedCommandPtr; //<! pointer to command received after entering this state
	c_BankCommand* m_prevCommandPtr;
	c_BankState* m_nextStatePtr;
	unsigned m_timer;

};
} // namespace n_Bank
} // namespace SST

#endif // C_BANKSTATEACTIVE_HPP
