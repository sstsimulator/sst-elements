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

#ifndef C_CHANNEL_HPP_
#define C_CHANNEL_HPP_

// global includes
#include <memory>
#include <vector>
#include <string>

// SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>

// local includes
#include "c_BankInfo.hpp"
#include "c_BankCommand.hpp"

namespace SST {
namespace n_Bank {

class c_Rank;
class c_Channel {
public:

	friend std::ostream& operator<<(std::ostream& x_stream,
			const c_Channel& x_channel) {
		x_stream << "Rank:" << std::endl;
		for (unsigned l_i = 0; l_i < x_channel.m_rankPtrs.size(); ++l_i) {
			x_stream << (x_channel.m_rankPtrs.at(l_i)) << std::endl;
		}

		return x_stream;
	}

	c_Channel(std::map<std::string, unsigned>* x_bankParams);
	virtual ~c_Channel();

	void acceptRank(c_Rank* x_rankPtr);

	unsigned getNumBanks() const;
	unsigned getNumRanks() const;

	std::vector<c_BankInfo*> getBankPtrs() const;

	void updateOtherBanksNextCommandCycles(c_Rank* x_initRankPtr,
			c_BankCommand* x_cmdPtr);

private:
	std::vector<c_Rank*> m_rankPtrs;
	std::map<std::string, unsigned>* m_bankParams;

};

} // end n_Bank
} // end SST
#endif /* C_CHANNEL_HPP_ */
