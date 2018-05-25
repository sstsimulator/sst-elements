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

#ifndef C_RANK_HPP_
#define C_RANK_HPP_

// global includes
#include <memory>
#include <vector>
#include <string>

// SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>

// local includes
#include "c_BankInfo.hpp"
//#include "c_BankCommand.hpp"
#include "c_Channel.hpp"

namespace SST {
namespace n_Bank {

class c_Channel;
class c_BankGroup;
class c_BankCommand;

  class c_Rank {
  public:

    friend std::ostream& operator<<(std::ostream& x_stream,
				    const c_Rank& x_rank) {
      x_stream<<"Rank:"<<std::endl;
      for(unsigned l_i=0; l_i<x_rank.m_bankGroupPtrs.size(); ++l_i) {
	x_stream<<(x_rank.m_bankGroupPtrs.at(l_i))<<std::endl;
      }

      return (x_stream);
    }

    c_Rank(std::map<std::string, unsigned>* x_bankParams);
    virtual ~c_Rank();

    void acceptBankGroup(c_BankGroup* x_bankGroupPtr);
    void acceptChannel(c_Channel* x_channelPtr);

    unsigned getNumBanks() const;
    unsigned getNumBankGroups() const;
      c_Channel* getChannelPtr() const;


    std::vector<c_BankInfo*>& getBankPtrs();

    void updateOtherBanksNextCommandCycles(c_BankGroup* x_initBankGroupPtr,
					   c_BankCommand* x_cmdPtr, SimTime_t x_cycle);

  private:
    c_Channel* m_channelPtr;
    std::vector<c_BankGroup*> m_bankGroupPtrs;
      std::vector<c_BankInfo*> m_allBankPtrs;
    std::map<std::string, unsigned>* m_bankParams;

  };

} // end n_Bank
} // end SST
#endif /* C_RANK_HPP_ */
