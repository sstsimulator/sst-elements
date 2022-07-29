// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * c_TxnConverter.hpp
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
#ifndef C_TxnConverter_HPP_
#define C_TxnConverter_HPP_

#include <vector>
#include <queue>

// SST includes
#include <sst/core/subcomponent.h>
#include <sst/core/link.h>

// local includes
#include "c_BankCommand.hpp"
//#include "c_TransactionToCommands.hpp"
#include "c_DeviceDriver.hpp"
#include "c_CmdScheduler.hpp"
#include "c_Transaction.hpp"
#include "c_BankInfo.hpp"
#include "c_Controller.hpp"

namespace SST {
namespace CramSim {
	class c_CmdScheduler;
	class c_Controller;


class c_TxnConverter: public SubComponent{

public:

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::CramSim::c_TxnConverter, Output*, unsigned, c_CmdScheduler*)

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        c_TxnConverter,
        "CramSim",
        "c_TxnConverter",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Transaction Converter",
        SST::CramSim::c_TxnConverter
    )

    SST_ELI_DOCUMENT_PARAMS(
		{"relCommandWidth", "Relative width of each command", NULL},
		{"bankPolicy", "Select which bank policy to model", "CLOSE"},
		{"boolUseReadA", "Whether to use READ or READA Cmds", NULL},
		{"boolUseWriteA", "Whether to use WRITE or WRITEA Cmds", NULL},
                {"bankCloseTime", "", NULL},
    )

    SST_ELI_DOCUMENT_PORTS(
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"readTxnsRecvd", "Number of read transactions received", "reads", 1}, // Name, Desc, Units, Enable Level
        {"writeTxnsRecvd", "Number of write transactions received", "writes", 1},
        {"totalTxnsRecvd", "Number of write transactions received", "transactions", 1},
        {"reqQueueSize", "Total size of the request queue over time", "transactions", 1},
        {"resQueueSize", "Total size of the response queue over time", "transactions", 1},
    )

    c_TxnConverter(SST::ComponentId_t id, SST::Params& x_params, Output* out, unsigned banks, c_CmdScheduler* scheduler);
    void build(SST::Params& x_params, unsigned l_bankNum);
    ~c_TxnConverter();

    void run(SimTime_t simCycle);
    void push(c_Transaction* newTxn); // receive txns from txnGen into req q
    c_BankInfo* getBankInfo(unsigned x_bankId);

private:

	std::vector<c_BankCommand*> getCommands(c_Transaction* x_txn);
	void getPreCommands(std::vector<c_BankCommand*> &x_commandVec, c_Transaction* x_txn, ulong x_addr);
	void getPostCommands(std::vector<c_BankCommand*> &x_commandVec, c_Transaction* x_txn, ulong x_addr);
	void updateBankInfo(c_Transaction* x_txn);

	/// / FIXME: Remove. For testing purposes
	void printQueues();

	c_CmdScheduler *m_cmdScheduler;

	std::vector<c_BankInfo*> m_bankInfo;
	unsigned m_bankNums;
	unsigned m_cmdSeqNum;

	std::deque<c_Transaction*> m_inputQ;

	// params
	int k_relCommandWidth; // txn relative command width
	bool k_useReadA;
	bool k_useWriteA;
	int k_bankPolicy;
	SimTime_t k_bankCloseTime;


  	// Statistics
	Statistic<uint64_t>* s_readTxnsRecvd;
  	Statistic<uint64_t>* s_writeTxnsRecvd;
  	Statistic<uint64_t>* s_totalTxnsRecvd;

    //debug
	Output *output;

};
}
}

#endif /* C_TxnConverter_HPP_ */

