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

#ifndef C_BANKGROUP_HPP_
#define C_BANKGROUP_HPP_

// global includes
#include <memory>
#include <vector>
#include <string>

// SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>

namespace SST {
namespace n_Bank {

class c_BankInfo;
class c_Rank;
class c_BankCommand;

class c_BankGroup {
public:


        c_BankGroup(std::map<std::string, unsigned>* x_bankParams, unsigned x_Id);
	virtual ~c_BankGroup();

	void acceptBank(c_BankInfo* x_bankPtr);
	void acceptRank(c_Rank* x_rankPtr);

	unsigned getNumBanks() const;
        unsigned getBankGroupId() const;
	std::vector<c_BankInfo*> getBankPtrs() const;
	c_Rank* getRankPtr() const;

	void updateOtherBanksNextCommandCycles(c_BankInfo* x_initBankPtr,
			c_BankCommand* x_cmdPtr, SimTime_t x_cycle);

private:
        unsigned m_bankGroupId;
	std::vector<c_BankInfo*> m_bankPtrs;
	c_Rank* m_rankPtr;

	std::map<std::string, unsigned>* m_bankParams;

};

} // end n_Bank
} // end SST
#endif /* C_BANKGROUP_HPP_ */
