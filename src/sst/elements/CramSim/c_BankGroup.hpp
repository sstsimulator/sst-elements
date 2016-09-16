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

// local includes
#include "c_Rank.hpp"
#include "c_BankInfo.hpp"
#include "c_BankCommand.hpp"

namespace SST {
namespace n_Bank {

class c_BankInfo;
class c_Rank;

class c_BankGroup {
public:

	// friend std::ostream& operator<<(std::ostream& x_stream,
	// 		const c_BankGroup& x_bankGroup) {
	// 	x_stream<<"Bank Group:"<<std::endl;
	// 	  for(unsigned l_i=0; l_i<x_bankGroup.m_bankPtrs.size(); ++l_i) {
	// 	    x_stream<<(x_bankGroup.m_bankPtrs.at(l_i))<<std::endl;
	// 	  }
	//
	// 	  return x_stream;
	// }

	c_BankGroup(std::map<std::string, unsigned>* x_bankParams);
	virtual ~c_BankGroup();

	void acceptBank(c_BankInfo* x_bankPtr);
	void acceptRank(c_Rank* x_rankPtr);

	unsigned getNumBanks() const;
	std::vector<c_BankInfo*> getBankPtrs() const;

	void updateOtherBanksNextCommandCycles(c_BankInfo* x_initBankPtr,
			c_BankCommand* x_cmdPtr);

private:
	std::vector<c_BankInfo*> m_bankPtrs;
	c_Rank* m_rankPtr;

	std::map<std::string, unsigned>* m_bankParams;

};

} // end n_Bank
} // end SST
#endif /* C_BANKGROUP_HPP_ */
