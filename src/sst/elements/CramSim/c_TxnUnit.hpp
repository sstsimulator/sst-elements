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
#include <sst/core/subcomponent.h>
#include <sst/core/link.h>

// local includes
#include "c_Transaction.hpp"
#include "c_BankCommand.hpp"
#include "c_TransactionToCommands.hpp"


namespace SST {
namespace n_Bank {
class c_TxnUnit: public SubComponent {
public:

	c_TxnUnit(SST::Component * comp, SST::Params& x_params);
	~c_TxnUnit();

	void pushTransaction(SST::Event *ev); // receive txns from txnGen into req q
	std::vector<c_BankCommand*> convertTransaction();

	void print() const; // print internals

private:

	void createRefreshCmds();

	// FIXME: Remove. For testing purposes
	void printQueues();

	// params for internal architecture
	int k_relCommandWidth; // txn relative command width
	bool k_useReadA;
	bool k_useWriteA;
	bool k_useRefresh;
	int k_bankPolicy;

	int m_numBanks;
	int k_REFI;
	int m_currentREFICount;
	c_TransactionToCommands *m_converter;

	std::vector<std::vector<unsigned> > m_refreshGroups;
	uint m_currentRefreshGroup;
	std::queue<c_BankCommand*> m_refreshList;

	// internal architecture
	std::queue<c_Transaction*> m_txnReqQ;
	std::queue<c_Transaction*> m_txnResQ;

	// FIXME: Delete. Used for debugging queue size issues
//	unsigned* m_statsReqQ;
//	unsigned* m_statsResQ;

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
