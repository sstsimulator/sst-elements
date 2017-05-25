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
#ifndef C_DIMM_HPP
#define C_DIMM_HPP

#include <vector>
#include <queue>

// SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>

// local includes
//#include "c_BankCommand.hpp"
#include "c_Bank.hpp"

namespace SST {
namespace n_Bank {

  class c_BankCommand;

  // holds all the statistics pointers for the banks
  class c_BankStatistics {
  public:
    Statistic<uint64_t>* s_bankACTsRecvd;
    Statistic<uint64_t>* s_bankREADsRecvd;
    Statistic<uint64_t>* s_bankWRITEsRecvd;
    Statistic<uint64_t>* s_bankPREsRecvd;
    
    Statistic<uint64_t>* s_bankRowHits;
    Statistic<uint64_t>* s_totalRowHits;
  };
  
class c_Dimm: public SST::Component {
public:

	c_Dimm(SST::ComponentId_t x_id, SST::Params& x_params);
	~c_Dimm();

	void finish(){
		std::cout << "Deleting DIMM" << std::endl;
		for (unsigned l_i = 0; l_i != m_banks.size(); ++l_i) {
			// m_banks.at(l_i)->finish();
		}
	}
private:
	c_Dimm(); // for serialization only
	c_Dimm(const c_Dimm&); // do not implement
	void operator=(const c_Dimm&); // do not implement

	virtual bool clockTic(SST::Cycle_t); // called every cycle

	// BankReceiver <-> CmdUnit Handlers
	void handleInCmdUnitReqPtrEvent(SST::Event *ev); // receive a cmd req from CmdUnit
	void handleOutCmdUnitResPtrEvent(SST::Event *ev); // we do not need this function for functionality

	// BankReceiver <-> CmdUnit Links
	SST::Link* m_inCmdUnitReqPtrLink; // incoming cmdunit req ptr
	SST::Link* m_outCmdUnitResPtrLink; // outgoing cmdunit res ptr

	void sendResponse();
	void sendToBank(c_BankCommand* x_bankCommandPtr);

	void printQueues();

	// internal microarcitecture params
	SimTime_t m_thisCycleReceivedCmds;

	// params for bank structure
	int k_numRanksPerChannel;
	int k_numBankGroupsPerRank;
	int k_numBanksPerBankGroup;
	int m_numBanks;


	std::vector<c_Bank*> m_banks;

	std::vector<c_BankCommand*> m_cmdResQ;
  
  // Statistics
  Statistic<uint64_t>* s_actCmdsRecvd;
  Statistic<uint64_t>* s_readCmdsRecvd;
  Statistic<uint64_t>* s_readACmdsRecvd;
  Statistic<uint64_t>* s_writeCmdsRecvd;
  Statistic<uint64_t>* s_writeACmdsRecvd;
  Statistic<uint64_t>* s_preCmdsRecvd;
  Statistic<uint64_t>* s_refCmdsRecvd;

  std::vector<c_BankStatistics*> m_bankStatsVec;
};
}
}

#endif // C_DIMM_HPP
