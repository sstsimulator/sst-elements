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

/*
 * c_TxnUnit.hpp
 *
 *  Created on: May 18, 2016
 *      Author: tkarkha
 */

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//   http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef C_TXNUNIT_HPP_
#define C_TXNUNIT_HPP_

#include <vector>
#include <queue>

// SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>

// local includes
#include "c_Transaction.hpp"

namespace SST {
namespace n_Bank {
class c_TxnUnit: public SST::Component {
public:
// TODO: overload << operator
// friend std::ostream& operator<< (std::ostream& x_stream, const c_Bank& x_bank);

	c_TxnUnit(SST::ComponentId_t x_id, SST::Params& x_params);
	~c_TxnUnit();

	void finish(){
		// TODO: Delete. For testing purposes
		// printf("TxnUnit Queues:\n");
		// printf("\t TxnUnit Req Q:\n");
		// for (unsigned l_i = 0; l_i != k_txnReqQEntries+1; ++l_i)
		// 	printf("%u\n", m_statsReqQ[l_i]);
		//
		// printf("\n\t TxnUnit Res Q:\n");
		// for (unsigned l_i = 0; l_i != k_txnResQEntries+1; ++l_i)
		// 	printf("%u\n", m_statsResQ[l_i]);
	}

	void print() const; // print internals

private:
	// TODO: change this to C++11 style =delete
	c_TxnUnit(); // for serialization only
	c_TxnUnit(const c_TxnUnit&); // do not implement
	void operator=(const c_TxnUnit&); // do not implement

	virtual bool clockTic(SST::Cycle_t); // called every cycle

	// TxnUnit <-> TxnGen Handlers
	void handleInTxnGenReqPtrEvent(SST::Event *ev); // receive txns from txnGen into req q
	void handleOutTxnGenReqQTokenChgEvent(SST::Event *ev); // we do not need this function for functionality
	void handleOutTxnGenResPtrEvent(SST::Event *ev); // we do not need this function for functionality
	void handleInTxnGenResQTokenChgEvent(SST::Event *ev); //receive tokens from txnGen for res q


	// TxnUnit <-> CmdUnit Handlers
	void handleOutCmdUnitReqPtrEvent(SST::Event *ev); // we do not need this function for functionality
	void handleInCmdUnitReqQTokenChgEvent(SST::Event *ev); // receive tokens from cmdUnit for req q
	void handleInCmdUnitResPtrEvent(SST::Event *ev); // receive txn from cmdUnit for res q

	// TxnUnit <-> TxnGen Links
	SST::Link* m_inTxnGenReqPtrLink; // incoming txngen req ptr
	SST::Link* m_outTxnGenReqQTokenChgLink; // outgoing txngen req q token
	SST::Link* m_outTxnGenResPtrLink; // outgoing txngen res ptr
	SST::Link* m_inTxnGenResQTokenChgLink; // incoming txngen res q token


	// TxnUnit <-> CmdUnit Links
	SST::Link* m_outCmdUnitReqPtrLink; // outgoing cmdunit req ptr
	SST::Link* m_inCmdUnitReqQTokenChgLink; // incoming cmdunit req q token
	SST::Link* m_inCmdUnitResPtrLink; // incoming cmdunit res ptr
	//outCmdUnitResQToken no longer part of model

	void sendTokenChg(); // should happen at the end of every cycle
	void sendResponse();
	void sendRequest();
	void createRefreshCmds();

	// FIXME: Remove. For testing purposes
	void printQueues();

	// params for internal architecture
	int k_txnReqQEntries;
	int k_txnResQEntries;
	int k_relCommandWidth; // txn relative command width
	bool k_useReadA;
	bool k_useWriteA;
	bool k_useRefresh;
	int k_bankPolicy;

	int m_numBanks;
	int k_REFI;
	int m_currentREFICount;
        std::vector<std::vector<unsigned> > m_refreshGroups;
        uint m_currentRefreshGroup;
	std::queue<c_BankCommand*> m_refreshList;


	// params for neighboring components
	int k_txnGenResQEntries;
	int m_txnGenResQTokens;
	int k_cmdUnitReqQEntries;
	int m_cmdUnitReqQTokens;

	int k_numBytesPerTransaction;
	int k_numChannelsPerDimm;
	int k_numRanksPerChannel;
	int k_numBankGroupsPerRank;
	int k_numBanksPerBankGroup;
	int k_numColsPerBank;
	int k_numRowsPerBank;


	// token change in this unit this cycle
	// beginning of every cycle these variables are reset
	// sendTokenChg() function sends the contents of these variables
	int m_thisCycleReqQTknChg;

	// internal architecture
	std::vector<c_Transaction*> m_txnReqQ;
	std::vector<c_Transaction*> m_txnResQ;

	// FIXME: Delete. Used for debugging queue size issues
	unsigned* m_statsReqQ;
	unsigned* m_statsResQ;

	bool m_processingRefreshCmds; // Cmd unit is processing refresh commands

  // Statistics
  Statistic<uint64_t>* s_readTxnsRecvd;
  Statistic<uint64_t>* s_writeTxnsRecvd;
  Statistic<uint64_t>* s_totalTxnsRecvd;
  Statistic<uint64_t>* s_reqQueueSize;
  Statistic<uint64_t>* s_resQueueSize;
};
}
}

#endif /* C_TXNUNIT_HPP_ */
